#pragma once

#include "../deps/pcg-cpp/include/pcg_random.hpp"
#include "../submodules/Tensors/UMFPACK.hpp"

#include "../submodules/Tensors/Clp.hpp"
#include "OrthoDraw.hpp"

namespace Knoodle
{
    // TODO: Only process _active_ crossings and _active_ arcs!
    // TODO: Add type checks everywhere.
    // TODO: Call COIN-OR for height regularizer.
    
    class Reapr
    {
    public:
        using Real      = Real64;

        using UMF_Int   = Int64;

        using COIN_Real = double;
        using COIN_Int  = int;
        using COIN_LInt = CoinBigIndex;
        
        static constexpr bool CLP_enabledQ = true;

        using PRNG_T    = pcg64;
        using Flag_T    = Scalar::Flag;
        
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
        int  SSN_max_b_iter      = 20;
        int  SSN_max_iter        = 1000;
        int  SSN_iter            = 0;
                
        Real initial_time_step   = Real(1.0) / backtracking_factor;
        
        EnergyFlag_T en_flag     = CLP_enabledQ ? EnergyFlag_T::TV: EnergyFlag_T::Dirichlet;
        
        int  rattle_counter      = 0;
        Real rattle_timing       = 0;

        
        PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };
        
    public:
        
        void SetEnergyFlag( EnergyFlag_T flag )
        {
            if ( (flag == EnergyFlag_T::TV) && !CLP_enabledQ )
            {
                eprint(MethodName("SetEnergyFlag")+": Energy flag is set to " + ToString(EnergyFlag_T::TV) + " (TV energy), but linear programming features are deactivated. Setting to fallback value " + ToString(EnergyFlag_T::Dirichlet) + " (Dirichlet energy).");
                
                en_flag = EnergyFlag_T::Dirichlet;
            }
            else
            {
                en_flag = flag;
            }
        }

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
            
            const auto & lc_arcs  = pd.LinkComponentArcs();
            const Int lc_count  = lc_arcs.SublistCount();
            cptr<Int> lc_arc_ptr = lc_arcs.Pointers().data();
//
            Tensor1<bool,Int> visitedQ ( m, false );

            // TODO: We need a feasible initialization of x!
            
            Real level = 0;
            Int a      = 0;
            Int b      = 0;
            Real sign  = 1;
            
            for( Int lc = 0; lc < lc_count; ++lc )
            {
                const Int a_begin = lc_arc_ptr[lc         ];
                const Int a_end   = lc_arc_ptr[lc + Int(1)];
                
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
#include "Reapr/LevelsByLP.hpp"
#include "Reapr/Embedding.hpp"
#include "Reapr/Rattle.hpp"
        
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
                    wprint(MethodName("Hessian")+": Energy flag " + ToString(en_flag) + " is unknown or invalid. Using default flag = " + ToString(EnergyFlag_T::Dirichlet) + " (Dirichlet enery) instead.");
                    
                    return this->DirichletHessian<I,J>(pd);
                }
            }
        }
        
        template<typename Int>
        Tensor1<Real,Int> Levels( mref<PlanarDiagram<Int>> pd )
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
                        // We should never reach this piece of code.
                        eprint(MethodName("Levels")+": Energy flag is set to " + ToString(EnergyFlag_T::TV) + " (TV energy), but linear programming features are deactivated. Returning levels computed by Dirichlet energy (energy flag = " + ToString(EnergyFlag_T::Dirichlet) + ") as fallback.");
                        en_flag = EnergyFlag_T::Dirichlet;
                        return LevelsBySSN(pd);
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
