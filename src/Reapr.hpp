#pragma once

#include "../submodules/Tensors/UMFPACK.hpp"

//#ifdef REAPR_USE_CLP
//    #include "../submodules/Tensors/Clp.hpp"
//#endif

#include "../submodules/Tensors/Clp.hpp"
#include "OrthoDrawing.hpp"

namespace Knoodle
{
    // TODO: Only process _active_ crossings and _active_ arcs!
    // TODO: Add type checks everywhere.
    // TODO: Call COIN-OR for height regularizer.
    
    
    
    
    
    // DONE: Allow use of Dirichlet energy as well as bending energy.
    // DONE: Call COIN-OR for TV regularizer -> simplex? interior point?
//    template<typename Int_>
    class Reapr
    {
    public:
        using Real      = Real64;

        using UMF_Int   = Int64;
        
//#ifdef REAPR_USE_CLP
        using COIN_Int  = int;
        using COIN_LInt = CoinBigIndex;
        
        static constexpr bool CLP_enabledQ = true;
//#else
//        static constexpr bool CLP_enabledQ = false;
//#endif

        using Flag_T = Scalar::Flag;
        
        enum class EnergyFlag_T : Int32
        {
            TV        = 0,
            Dirichlet = 1,
            Bending   = 2
        };

        static constexpr Real jump = 1;

    private:
        
        Real scaling             = Real(1);
        Real dirichlet_reg       = Real(0.00001);
        Real bending_reg         = Real(0.00001);
        Real backtracking_factor = Real(0.25);
        Real armijo_slope        = Real(0.001);
        Real tolerance           = Real(0.00000001);
        int  max_b_iter          = 20;
        int  max_iter            = 1000;
        
        int  iter                = 0;
        
        Real initial_time_step   = Real(1.0) / backtracking_factor;
        
        EnergyFlag_T en_flag     = CLP_enabledQ ? EnergyFlag_T::TV: EnergyFlag_T::Dirichlet;
        
    public:
        
        void SetEnergyFlag( EnergyFlag_T flag ){ en_flag = flag; }

        EnergyFlag_T EnergyFlag() const { return en_flag; }
        
        
        void SetScaling( Real scal ) { scaling = scal; }

        Real Scaling() const { return scaling; }

        

        
    private:
        
        template<typename Int>
        EnergyFlag_T EnergyFlag( mref<PlanarDiagram<Int>> pd ) const
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.GetCache(tag);
        }
        
        template<typename Int>
        bool EnergyFlagOutdatedQ( mref<PlanarDiagram<Int>> pd ) const
        {
            return EnergyFlag(pd) != en_flag;
        }
        
        
        template<typename Int>
        void SetEnergyFlag( mref<PlanarDiagram<Int>> pd )
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.SetCache(tag,en_flag);
        }
        
        // TODO: Do we want more get-setters?
        
    public:
        
        template<typename Int>
        void WriteReaprFeasibleLevels(
            mref<PlanarDiagram<Int>> pd,
            mptr<Real> x )
        {
            
            constexpr bool Tail  = PlanarDiagram<Int>::Tail;
//            constexpr bool Head  = PlanarDiagram<Int>::Head;

            constexpr bool Out   = PlanarDiagram<Int>::Out;
//            constexpr bool In    = PlanarDiagram<Int>::In;
            
            constexpr bool Left  = PlanarDiagram<Int>::Left;
            constexpr bool Right = PlanarDiagram<Int>::Right;
            
            const Int m        = pd.ArcCount();
            auto & C_arcs      = pd.Crossings();
            auto & A_cross     = pd.Arcs();
            cptr<Int> comp_ptr = pd.LinkComponentArcPointers().data();
            
            Tensor1<bool,Int> visitedQ ( m, false );

            // TODO: We need a feasible initialization of x!
            
            Real level = 0;
            Int a      = 0;
            Int b      = 0;
            Real sign  = 1;
            
            const Int comp_count = pd.LinkComponentCount();
            
            for( Int comp = 0; comp < comp_count; ++comp )
            {
                const Int a_begin = comp_ptr[a  ];
                const Int a_end   = comp_ptr[a+1];
                
                a = a_begin;
                
                // TODO: This might not work for multi-component links.
                for( Int i = a_begin; i < a_end; ++i )
                {
                    Int c = A_cross(a,Tail);
                    
                    visitedQ[a] = true;
                    
                    Int a_0 = C_arcs(c,Out,Left );
                    Int a_1 = C_arcs(c,Out,Right);
                    
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
        }

#include "Reapr/DirichletHessian.hpp"
#include "Reapr/BendingHessian.hpp"
#include "Reapr/LevelsConstraintMatrix.hpp"
#include "Reapr/LevelsSystemSSN.hpp"
#include "Reapr/LevelsBySSN.hpp"
        
#include "Reapr/LevelsSystemLP.hpp"
#include "Reapr/BendsSystemLP.hpp"
        
//#ifdef REAPR_USE_CLP
    #include "Reapr/LevelsByLP.hpp"
    #include "Reapr/BendsByLP.hpp"
//#endif
        
    public:
        
        template<typename I, typename J, typename Int>
        Sparse::MatrixCSR<Real,I,J> Hessian( mref<PlanarDiagram<Int>> pd ) const
        {
            switch ( en_flag )
            {
                case EnergyFlag_T::Bending:
                {
                    return this->BendingHessian<I,J>(pd);
                }
                case EnergyFlag_T::Dirichlet:
                {
                    return this->DirichletHessian<I,J>(pd);
                }
                default:
                {
                    wprint(MethodName("Hessian")+": Unknown or invalid energy flag. Returning empty matrix" );
                    
                    return Sparse::MatrixCSR<Real,I,J>();
                }
            }
        }
        
        
        template<typename Int>
        Tensor1<Real,Int> Levels( mref<PlanarDiagram<Int>> pd ) const
        {
            switch ( en_flag )
            {
                case EnergyFlag_T::TV:
                {
                    if constexpr( CLP_enabledQ )
                    {
                        return LevelsByLP(pd);
                    }
                    else
                    {
                        eprint(MethodName("Levels")+": Energy flag is set to TV, but linear programming features are deactivated. Returning empty vector.");
                        return Tensor1<Real,Int>();
                    }
                }
                case EnergyFlag_T::Bending:
                {
                    return LevelsBySSN(pd);
                }
                case EnergyFlag_T::Dirichlet:
                {
                    return LevelsBySSN(pd);
                }
            }
        }
        
        template<typename Int>
    std::pair<Tensor1<Int,Int>,Tensor1<Real,Int>> Embedding(
            mref<PlanarDiagram<Int>> pd
        ) const
        {
            OrthoDrawing<Int> H (pd);
            Tensor1<Real,Int> L = Levels(pd);
            
            const Int comp_count = pd.ComponentCount();
            Tensor1<Int,Int> ptr (comp_count+Int(0),Int(0));

            
            pd.Components()
            
            for( Int comp = 0; comp < comp_count; ++comp )
            {
            }
        }
//        
//        template<typename Int>
//        std::vector<PlanarDiagram<Int>> Rattle(
//            mref<PlanarDiagram<Int>> pd, Int iter_
//        ) const
//        {
//            std::vector<PlanarDiagram<Int>> result;
//            
//            rattle( pd, result, iter_ );
//            
//            return result;
//        }
//        
//        template<typename Int>
//        std::vector<PlanarDiagram<Int>> Rattle(
//            mref<std::vector<PlanarDiagram<Int>>> pd_list, Int iter_
//        ) const
//        {
//            std::vector<PlanarDiagram<Int>> result;
//            
//            for( auto & pd : pd_list )
//            {
//                rattle( pd, result, iter_ );
//            }
//            
//            return result;
//        }
//        
//        template<typename Int>
//        void rattle(
//            mref<std::vector<PlanarDiagram<Int>>> input,
//            mref<std::vector<std::pair<PlanarDiagram<Int>,Int>>> stack,
//            mref<std::vector<PlanarDiagram<Int>>> output,
//            Int iter_
//        ) const
//        {
//            using PD_T = PlanarDiagram<Int>;
//            
//            if( iter_ <= Int(0) )
//            {
//                return;
//            }
//            
//            for( auto & pd_0 : input )
//            {
//                PD_T pd_1 ( Embedding(pd_0) );
//                
//                // TODO: Rotate!
//                
//                std::vector<PD_T> summands;
//                
//                pd_1.Simplify5(summands);
//                
//                for( auto & pd_2 : summands )
//                {
//                    stack.push_back( std::move(pd_2) );
//                }
//                
//                stack.push_back( std::move(pd_1) );
//                
//                rattle( stack, output, iter_-1 );
//            }
//        }
        
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return "Reapr";
        }
        
    }; // class Reapr
    

} // namespace Knoodle
