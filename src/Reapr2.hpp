#pragma once

#ifdef KNOODLE_USE_UMFPACK
#include "../submodules/Tensors/UMFPACK.hpp"
#endif

#ifdef KNOODLE_USE_CLP
#include "../submodules/Tensors/Clp.hpp"
#endif

namespace Knoodle
{
    // TODO: Only process _active_ crossings and _active_ arcs!
    // TODO: Add type checks everywhere.
    
    template<typename Real_ = Real64, typename Int_ = Int64, typename BReal_ = Real32>
    class Reapr2
    {
    public:
        static_assert(FloatQ<Real_>,"");
        static_assert(Scalar::RealQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
        using Real                = Real_;
        using Int                 = Int_;
        using BReal               = BReal_;

#ifdef KNOODLE_USE_UMFPACK
        using UMF_Int             = Int64;
#endif
        
#ifdef KNOODLE_USE_CLP
        using COIN_Real           = double;
        using COIN_Int            = int;
        using COIN_LInt           = CoinBigIndex;
        static constexpr bool CLP_enabledQ = true;
#else
        static constexpr bool CLP_enabledQ = false;
#endif
        
//        using Link_T              = LinkEmbedding<Real,Int,BReal>;
        using PD_T                = PlanarDiagram2<Int>;
        using Point_T             = Tiny::Vector<3,Real,Int>;
        using OrthoDraw_T         = OrthoDraw<PD_T>;
        using OrthoDrawSettings_T = OrthoDraw_T::Settings_T;
//        using Embedding_T         = RaggedList<Point_T,Int>;
        using LinkEmbedding_T     = LinkEmbedding<Real,Int,BReal>;
        
        using PRNG_T              = Knoodle::PRNG_T;
        using Flag_T              = Scalar::Flag;
        

        enum class Energy_T : Int8
        {
            TV        = 0,
            Dirichlet = 1,
            Bending   = 2,
            Height    = 3,
            TV_CLP    = 4,
            TV_MCF    = 5
        };
        
        friend std::string ToString( Energy_T e )
        {
            switch(e)
            {
                case Energy_T::TV:        return "TV";
                case Energy_T::Dirichlet: return "Dirichlet";
                case Energy_T::Bending:   return "Bending";
                case Energy_T::Height:    return "Height";
                case Energy_T::TV_CLP:    return "TV_CLP";
                case Energy_T::TV_MCF:    return "TV_MCF";
                default:                  return "Unknown";
            }
        }

        struct Settings_T
        {
            bool permute_randomQ       = true;
            Energy_T energy            = Energy_T::TV;
            
            OrthoDrawSettings_T ortho_draw_settings = {
                .randomize_bends  = 4,
                .randomize_virtual_edgesQ = true
            };
            
            Real   scaling             = Real(1);
            Real   dirichlet_reg       = Real(0.00001);
            Real   bending_reg         = Real(0.00001);
            Real   backtracking_factor = Real(0.25);
            Real   armijo_slope        = Real(0.001);
            Real   tolerance           = Real(0.00000001);
            Size_T SSN_max_b_iter      = 20;
            Size_T SSN_max_iter        = 1000;
        };
        
        friend std::string ToString( cref<Settings_T> args )
        {
            return std::string("{ ")
                    +   ".permute_randomQ = " + ToString(args.permute_randomQ)
                    + ", .energy = " + ToString(args.energy)
                    + ", .ortho_draw_settings = " + ToString(args.ortho_draw_settings)
                    + ", .scaling = " + ToStringFPGeneral(args.scaling)
                    + ", .dirichlet_reg = " + ToStringFPGeneral(args.dirichlet_reg)
                    + ", .bending_reg = " + ToStringFPGeneral(args.bending_reg)
                    + ", .backtracking_factor = " + ToStringFPGeneral(args.backtracking_factor)
                    + ", .armijo_slope = " + ToStringFPGeneral(args.armijo_slope)
                    + ", .tolerance = " + ToStringFPGeneral(args.tolerance)
                    + ", .SSN_max_b_iter = " + ToString(args.SSN_max_b_iter)
                    + ", .SSN_max_iter = " + ToString(args.SSN_max_iter)
            + " }";
        }
        
        
        static constexpr Real jump = 1;

        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        
    private:
        
        Settings_T settings;
        
        mutable PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };
        
    public:
        
        Reapr2( cref<Settings_T> settings_ = Settings_T() )
        : settings { settings_ }
        {}
        
        ~Reapr2() = default;
        
        // We redefine the copy constructor because of random_engine.
        Reapr2( const Reapr2 & other )
        :   settings          { other.settings                }
        ,   random_engine { InitializedRandomEngine<PRNG_T>() }
        {}
        
    private:

        
        Energy_T EnergyFlag( mref<PD_T> pd ) const
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.GetCache(tag);
        }
        
        void SetEnergyFlag( mref<PD_T> pd )
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.SetCache(tag,settings.energy);
        }
        
        Size_T Iteration( mref<PD_T> pd ) const
        {
            const std::string tag = MethodName("Iteration");
            
            if( !pd.InCacheQ(tag) ) { SetIteration(pd,Size_T(0));
            }
            return pd.GetCache(tag);
        }
        
        void SetIteration( mref<PD_T> pd, const Size_T iter )
        {
            const std::string tag = MethodName("Iteration");
            
            return pd.template SetCache<false>(tag,iter);
        }
        
    public:
        
        cref<Settings_T> Settings() const
        {
            return settings;
        }
        
        void WriteFeasibleLevels( cref<PD_T> pd, mptr<Real> x )
        {
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
                 
                    a = pd.NextArc(a,PD_T::Head);
                }
            }
        }

#include "Reapr2/DirichletHessian.hpp"
#include "Reapr2/BendingHessian.hpp"
#include "Reapr2/LevelsConstraintMatrix.hpp"
        
#ifdef KNOODLE_USE_UMFPACK
    #include "Reapr2/LevelsQP_SSN.hpp"
#endif
        
#ifdef KNOODLE_USE_CLP
    #include "Reapr2/LevelsLP_CLP.hpp"
#endif
        
#include "Reapr2/LevelsLP_MCF.hpp"
#include "Reapr2/LevelsMinHeight.hpp"
#include "Reapr2/Embedding.hpp"
#include "Reapr2/RandomRotation.hpp"
//#include "Reapr/Rattle.hpp"
//#include "Reapr/Generate.hpp"
        
    public:
        
        template<typename R = Real, typename I = Int, typename J = Int>
        Sparse::MatrixCSR<R,I,J> Hessian( cref<PD_T> pd ) const
        {
            switch ( settings.energy )
            {
                case Energy_T::Bending:
                {
                    return this->BendingHessian<R,I,J>(pd);
                }
                case Energy_T::Dirichlet:
                {
                    return this->DirichletHessian<R,I,J>(pd);
                }
                default:
                {
                    wprint(MethodName("Hessian")+": Energy flag " + ToString(settings.energy) + " is unknown or invalid for Hessian. Returning empty matrix");
                    
                    return Sparse::MatrixCSR<R,I,J>();
                }
            }
        }
        
        Tensor1<Real,Int> Levels( cref<PD_T> pd )
        {
            switch ( settings.energy )
            {
                case Energy_T::TV:
                {
                    return LevelsLP_MCF(pd);
                }
                case Energy_T::TV_CLP:
                {
#ifdef KNOODLE_USE_CLP
                    return LevelsLP_CLP(pd);
#else
                    wprint(ClassName() + "(): Energy_T::TV_CLP is only supported if compiled with Coin-or CLP support, which is deactivated. Using default (Energy_T::TV) instead. (No worries, it solves the same problem, but faster.)");
                    return LevelsLP_MCF(pd);
#endif // KNOODLE_USE_CLP
                }
                case Energy_T::TV_MCF:
                {
                    return LevelsLP_MCF(pd);
                }
                case Energy_T::Bending:
                {
#ifdef KNOODLE_USE_UMFPACK
                    return LevelsQP_SSN(pd);
#else
                    wprint(ClassName() + "(): Energy_T::Bending is only supported if compiled with UMFPACK support, which is deactivated. Using default (Energy_T::TV) instead.");
                    return LevelsLP_MCF(pd);
#endif // KNOODLE_USE_UMFPACK
                }
                case Energy_T::Dirichlet:
                {
#ifdef KNOODLE_USE_UMFPACK
                    return LevelsQP_SSN(pd);
#else
                    wprint(ClassName() + "(): Energy_T::Dirichlet is only supported if compiled with UMFPACK support, which is deactivated. Using default (Energy_T::TV) instead.");
                    return LevelsLP_MCF(pd);
#endif // KNOODLE_USE_UMFPACK
                }
                case Energy_T::Height:
                {
                    return LevelsMinHeight(pd);
                }
                default:
                {
                    wprint(ClassName() + "(): Unknown or unsupported energy flag " + ToString(settings.energy) + ". Using default (Energy_T::TV) instead.");
                    return LevelsLP_MCF(pd);
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
            return std::string("Reapr2")
            + "<" + TypeName<Real>
            + "," + TypeName<Int>
            + ">";
        }
        
    }; // class Reapr

} // namespace Knoodle
