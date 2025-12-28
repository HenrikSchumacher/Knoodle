#pragma once

#include "../deps/pcg-cpp/include/pcg_random.hpp"
#include "../submodules/Tensors/UMFPACK.hpp"

#include "../submodules/Tensors/Clp.hpp"
#include "OrthoDraw.hpp"

#include <boost/unordered/unordered_flat_set.hpp>

namespace Knoodle
{
    // TODO: Only process _active_ crossings and _active_ arcs!
    // TODO: Add type checks everywhere.
    // TODO: Call COIN-OR for height regularizer.
    
    template<typename Real_, typename Int_>
    class Reapr
    {
    public:
        static_assert(FloatQ<Real_>,"");
        static_assert(Scalar::RealQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
        using Real                = Real_;
        using Int                 = Int_;

        using UMF_Int             = Int64;

        using COIN_Real           = double;
        using COIN_Int            = int;
        using COIN_LInt           = CoinBigIndex;
        
        using Link_T              = Link_2D<Real,Int>;
        using PD_T                = PlanarDiagram<Int>;
        using Point_T             = std::array<Real,3>;
        using OrthoDraw_T         = OrthoDraw<Int>;
        using OrthoDrawSettings_T = OrthoDraw_T::Settings_T;
        using Embedding_T         = RaggedList<Point_T,Int>;

        
        using PRNG_T              = pcg64;
        using Flag_T              = Scalar::Flag;

        static constexpr bool CLP_enabledQ = true;

        enum class EnergyFlag_T : Int32
        {
            TV        = 0,
            Dirichlet = 1,
            Bending   = 2,
            Height    = 3,
            TV_CLP    = 4,
            TV_MCF    = 5,
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
        
        
        bool permute_randomQ     = true;
        OrthoDrawSettings_T ortho_draw_settings = {
            .randomize_virtual_edgesQ = true,
            .randomize_bends  = 4
        };
        
        int  rattle_counter      = 0;
        Real rattle_timing       = 0;
        
        mutable PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };
        
    public:
        
        void Reseed()
        {
            random_engine = InitializedRandomEngine<PRNG_T>();
        }
        
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
        
        
        Real Scaling() const { return scaling; }
        
        void SetScaling( Real val ) { scaling = val; }
        
        
        bool PermuteRandomQ() const { return permute_randomQ; }
        
        void SetPermuteRandomQ( bool val ) { permute_randomQ = val; }

        mref<OrthoDrawSettings_T> OrthoDrawSettings()
        {
            return ortho_draw_settings;
        }
        
        cref<OrthoDrawSettings_T> OrthoDrawSettings() const
        {
            return ortho_draw_settings;
        }
        
        void SetOrthoDrawSettings( OrthoDrawSettings_T && val )
        {
            ortho_draw_settings = val;
        }
        void SetOrthoDrawSettings( cref<OrthoDrawSettings_T> val )
        {
            ortho_draw_settings = val;
        }
        
        
    private:
        
        EnergyFlag_T EnergyFlag( mref<PlanarDiagram<Int>> pd ) const
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.GetCache(tag);
        }
        
        bool EnergyFlagOutdatedQ( cref<PlanarDiagram<Int>> pd ) const
        {
            return EnergyFlag(pd) != en_flag;
        }
        
        void SetEnergyFlag( mref<PlanarDiagram<Int>> pd )
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.SetCache(tag,en_flag);
        }
        
        // TODO: Do we want more get-setters?
        
    public:
        
        void WriteReaprFeasibleLevels( cref<PlanarDiagram<Int>> pd, mptr<Real> x )
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
#include "Reapr/LevelsQP_SSN.hpp"
#include "Reapr/LevelsLP_CLP.hpp"
#include "Reapr/LevelsLP_MCF.hpp"
#include "Reapr/LevelsMinHeight.hpp"
#include "Reapr/Embedding.hpp"
#include "Reapr/RandomRotation.hpp"
#include "Reapr/Rattle.hpp"
#include "Reapr/Generate.hpp"
        
    public:
        
        template<typename R = Real, typename I = Int, typename J = Int>
        Sparse::MatrixCSR<R,I,J> Hessian( cref<PlanarDiagram<Int>> pd ) const
        {
            switch ( en_flag )
            {
                case EnergyFlag_T::Bending:
                {
                    return this->BendingHessian<R,I,J>(pd);
                }
                case EnergyFlag_T::Dirichlet:
                {
                    return this->DirichletHessian<R,I,J>(pd);
                }
                default:
                {
                    wprint(MethodName("Hessian")+": Energy flag " + ToString(en_flag) + " is unknown or invalid for Hessian. Returning empty matrix");
                    
                    return Sparse::MatrixCSR<R,I,J>();
                }
            }
        }
        
        Tensor1<Real,Int> Levels( cref<PD_T> pd )
        {
            switch ( en_flag )
            {
                case EnergyFlag_T::TV:
                {
                    return LevelsLP_MCF(pd);
                }
                case EnergyFlag_T::TV_CLP:
                {
                    if constexpr( CLP_enabledQ )
                    {
                        return LevelsLP_CLP(pd);
                    }
                    else
                    {
                        // We should never reach this piece of code.
                        eprint(MethodName("Levels")+": Energy flag is set to " + ToString(EnergyFlag_T::TV) + " (TV energy), but linear programming features are deactivated. Returning levels computed by Dirichlet energy (energy flag = " + ToString(EnergyFlag_T::Dirichlet) + ") as fallback.");
                        en_flag = EnergyFlag_T::Dirichlet;
                        return LevelsBySSN(pd);
                    }
                }
                case EnergyFlag_T::TV_MCF:
                {
                    return LevelsLP_MCF(pd);
                }
                case EnergyFlag_T::Bending:
                {
                    return LevelsQP_SSN(pd);
                }
                case EnergyFlag_T::Dirichlet:
                {
                    return LevelsQP_SSN(pd);
                }
                case EnergyFlag_T::Height:
                {
                    return LevelsMinHeight(pd);
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
            return std::string("Reapr")
            + "<" + TypeName<Real>
            + "," + TypeName<Int>
            + "," + TypeName<CodeInt>
            + ">";
        }
        
    }; // class Reapr
    

} // namespace Knoodle
