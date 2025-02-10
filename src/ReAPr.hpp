#pragma once

#include "../submodules/Tensors/UMFPACK.hpp"

namespace KnotTools
{
 
    template<typename Int_>
    class ReAPr
    {
        using Real = Real64;
        using Int  = Int_;
        using LInt = Int64;
        
        using PlanarDiagram_T = PlanarDiagram<Int>;
        using Aggregator_T    = TripleAggregator<LInt,LInt,Real,LInt>;
        using Matrix_T        = Sparse::MatrixCSR<Real,LInt,LInt>;
        using I_T             = Matrix_T::Int;
        using Values_T        = Tensor1<Real,I_T>;
        using Flag_T          = Scalar::Flag;
        
        
        static constexpr bool Head  = PlanarDiagram_T::Head;
        static constexpr bool Tail  = PlanarDiagram_T::Tail;
        static constexpr bool Left  = PlanarDiagram_T::Left;
        static constexpr bool Right = PlanarDiagram_T::Right;
        static constexpr bool Out   = PlanarDiagram_T::Out;
        static constexpr bool In    = PlanarDiagram_T::In;
        
    private:
        
        Real laplace_reg         = 0.01;
        Real backtracking_factor = Real(0.25);
        Real armijo_slope        = Real(0.001);
        Real tolerance           = Real(0.00000001);
        Int  max_b_iter          = 20;
        Int  max_iter            = 1000;
        
        Real initial_time_step   = Real(1.0) / backtracking_factor;
        
    public:
        
#include "ReAPr/Hessian.hpp"
#include "ReAPr/ConstraintMatrix.hpp"
#include "ReAPr/SystemMatrix.hpp"
  
//        void Optimize_InteriorPointMethod()
//        {
//        }
        
        Tensor1<Real,I_T> Optimize_SemiSmoothNewton( mref<PlanarDiagram_T> pd )
        {
            const I_T n = pd.CrossingCount();
            const I_T m = pd.ArcCount();
            
            // x[0,...,m[ is the levels.
            // x[m+1,...,m_n[ is the Lagrange multipliers.
            Tensor1<Real,I_T> x     (m+n,Real(0));
            Tensor1<Real,I_T> x_tau (m+n);
            
            // Stores the result y = L.x.
            Tensor1<Real,I_T> y     (m+n,Real(0));
            
            // Stores the result z = F(x).
            Tensor1<Real,I_T> z     (m+n,Real(0));
            fill_buffer(&z[m], Real(2), n );
            
            // Update direction.
            Tensor1<Real,I_T> u     (m+n);
            
            cref<Matrix_T> L = SystemMatrixPattern(pd);
            
            UMFPACK<Real,I_T> umfpack(
                L.RowCount(), L.ColCount(), L.Outer().data(), L.Inner().data()
            );

            // TODO: Is this a good value for the tolerance?
            const Real threshold = Power(m * tolerance,2);
            
            Int  iter          = 0;
            Real max_step_size = 0;

//            L.Dot( Scalar::One<Real>, x, Scalar::Zero<Real>, y );
            // In first iteration both x and y are zero at this point.
            
            Real phi_0 = Real(4) * n;
            
            do
            {
                ++iter;
                
                umfpack.NumericFactorization( SystemMatrixValues( pd, y.data() ).data() );

                // u = - L^{-1} . z = -DF(x)^{-1} . F(x);
                umfpack.Solve<Op::Id,Flag_T::Minus,Flag_T::Zero>(
                    -Scalar::One<Real>, z.data(), Scalar::Zero<Real>, u.data()
                );
                
                Real phi_tau;
                Real tau    = initial_time_step;
                Int  b_iter = 0;
                
                // Armijo line search.
                do{
                    ++b_iter;
                    
                    tau = backtracking_factor * tau;
                    
                    // x_tau = x + tau * u;
                    combine_buffers3<Flag_T::Plus,Flag_T::Generic>(
                        Scalar::One<Real>, x.data(), tau, u.data(), x_tau.data(), m + n
                    );
                    
                    // y = L.x_tau
                    L.Dot( Scalar::One<Real>, x_tau, Scalar::Zero<Real>, y );
                    
                    phi_tau = 0;

                    for( I_T i = 0; i < m; ++i )
                    {
                        z[i] = y[i];
                        
                        phi_tau += z[i] * z[i];
                    }
                    
                    for( I_T i = m; i < m + n; ++i )
                    {
                        z[i] = Ramp( y[i] + 2 ) - x_tau[i];
                        
                        phi_tau += z[i] * z[i];
                    }
                    
                    // Now z = F(x_tau).
                }
                while(
                    (phi_tau > (Real(1) - armijo_slope * tau) * phi_0)
                    &&
                    (b_iter < max_b_iter)
                );
                
                if( (phi_tau > (Real(1) - armijo_slope * tau) * phi_0) && (b_iter >= max_b_iter) )
                {
                    wprint(ClassName() + "::OptimizeSemiSmoothNewton: Maximal number of backtrackings reached.");
                    
                    dump( u.FrobeniusNorm() );
                }
                
                phi_0 = phi_tau;
                
                max_step_size = Max( tau, max_step_size );
                
                swap( x, x_tau );
            }
            while(
                (phi_0 > threshold) && (iter < max_iter)
            );
            
            if( (phi_0 > threshold) && (iter >= max_iter))
            {
                wprint(ClassName() + "::OptimizeSemiSmoothNewton: Maximal number of iterations reached without reaching the stopping criterion/");
            }
            
            return x;
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("ReAPr") + "<" + TypeName<Int> + ">";
        }
        
        
    }; // class ReAPr
    

} // namespace KnotTools
