#pragma once

#include "../submodules/Tensors/UMFPACK.hpp"

namespace Knoodle
{
    
    // TODO: Allow use of Dirichlet energy as well as bending energy.
    // TODO: Call COIN-OR for TV regularizer -> implex? interior point?
    // TODO: Call COIN-OR for height regularizer.
    
    template<typename Int_>
    class Reapr
    {
    public:
        using Real = Real64;
        using Int  = Int_;
        using LInt = Int64;
        
        using PlanarDiagram_T = PlanarDiagram<Int>;
        using Aggregator_T    = TripleAggregator<LInt,LInt,Real,LInt>;
        using Matrix_T        = Sparse::MatrixCSR<Real,LInt,LInt>;
        using I_T             = Matrix_T::Int;
        using Values_T        = Tensor1<Real,I_T>;
        using Flag_T          = Scalar::Flag;
        
        enum class EnergyFlag_T : Int32
        {
            Dirichlet = 0,
            Bending   = 1
        };

        static constexpr Real jump = 1;

    private:
        
        Real scaling             = Real(1);
        Real dirichlet_reg       = Real(0.00001);
        Real bending_reg         = Real(0.00001);
        Real backtracking_factor = Real(0.25);
        Real armijo_slope        = Real(0.001);
        Real tolerance           = Real(0.00000001);
        Int  max_b_iter          = 20;
        Int  max_iter            = 1000;
        
        Real initial_time_step   = Real(1.0) / backtracking_factor;
        
        EnergyFlag_T en_flag     = EnergyFlag_T::Dirichlet;
        
    public:
        
        void SetEnergyFlag( EnergyFlag_T flag ){ en_flag = flag; }

        EnergyFlag_T EnergyFlag() const { return en_flag; }
        
        
        void SetScaling( Real scal ) { scaling = scal; }

        Real Scaling() const { return scaling; }

        
        Matrix_T Hessian( mref<PlanarDiagram_T> pd ) const
        {
            switch ( en_flag )
            {
                case EnergyFlag_T::Bending:
                {
                    return BendingHessian(pd);
                }
                case EnergyFlag_T::Dirichlet:
                {
                    return DirichletHessian(pd);
                }
            }
        }

        
    private:
        
        EnergyFlag_T EnergyFlag( mref<PlanarDiagram_T> pd ) const
        {
            const std::string tag = ClassName() + "EnergyFlag";
            
            return pd.GetCache(tag);
        }
        
        bool EnergyFlagOutdatedQ( mref<PlanarDiagram_T> pd ) const
        {
            return EnergyFlag(pd) != en_flag;
        }
        
        
        void SetEnergyFlag( mref<PlanarDiagram_T> pd )
        {
            const std::string tag = ClassName() + "EnergyFlag";
            
            return pd.SetCache(tag,en_flag);
        }
        
        // TODO: Do we want more get-setters?
        
    public:
        
        void WriteReaprFeasibleLevels( mref<PlanarDiagram_T> pd, mptr   <Real> x )
        {
            const I_T m        = pd.ArcCount();
            auto & C_arcs      = pd.Crossings();
            auto & A_cross     = pd.Arcs();
            cptr<Int> comp_ptr = pd.LinkComponentArcPointers().data();
            
            Tensor1<bool,I_T> visitedQ ( m, false );

            // TODO: We need a feasible initialization of x!
            
            Real level = 0;
            Int a      = 0;
            Int b      = 0;
            Real sign  = 1;
            
            for( Int comp = 0; comp < pd.LinkComponentCount(); ++comp )
            {
                const Int a_begin = comp_ptr[a  ];
                const Int a_end   = comp_ptr[a+1];
                
                a = a_begin;
                
                // TODO: This works only for knots, not for links.
                for( Int i = a_begin; i < a_end; ++i )
                {
                    Int c = A_cross(a,0);
                    
                    visitedQ[a] = true;
                    
                    Int a_0 = C_arcs(c,0,0);
                    Int a_1 = C_arcs(c,0,1);
                    
                    if( a == a_0 )
                    {
                        b = a_1;
                        sign =  Real(2);
                    }
                    else
                    {
                        b = a_0;
                        sign = -Real(2);
                    }
                    
                    if( visitedQ[b] )
                    {
                        level = x[b] + ( pd.CrossingLeftHandedQ(c) ? sign : -sign );
                    }
                    
                    x[a] = level;
                 
                    a = pd.template NextArc<1>(a);
                }
            }
            
        } // WriteReaprFeasibleLevels

#include "Reapr/DirichletHessian.hpp"
#include "Reapr/BendingHessian.hpp"
#include "Reapr/ConstraintMatrix.hpp"
#include "Reapr/SystemMatrix.hpp"
#include "Reapr/Optimize_SemiSmoothNewton.hpp"
 
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("Reapr")
                + "<" + TypeName<Int>
                + ">";
        }
        
        
    }; // class Reapr
    

} // namespace Knoodle
