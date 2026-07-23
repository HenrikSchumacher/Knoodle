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
    
    /*!@brief A class for computing relatively simple 3D embeddings of planar diagrams.
     */
    
    template<FloatQ Real_ = Real64, IntQ Int_ = Int64, FloatQ BReal_ = Real32>
    class Reapr
    {
    public:
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
        
        static constexpr Parallel_T parQ = Sequential;
        
        //        using Link_T              = LinkEmbedding<Real,Int,BReal>;
        using PD_T                = PlanarDiagram<Int>;
        using Point_T             = Tiny::Vector<3,Real,Int>;
        using Matrix_T            = Tiny::Matrix<3,3,Real,Int>;
        using OrthoDraw_T         = OrthoDraw<PD_T>;
        using OrthoDrawSettings_T = OrthoDraw_T::Settings_T;
        //        using Embedding_T         = RaggedList<Point_T,Int>;
        using LinkEmbedding_T     = LinkEmbedding<Real,Int,BReal>;
        
        using PRNG_T              = Knoodle::PRNG_T;
        using Flag_T              = Scalar::Flag;
        
        /*!@brief The objective function that is optimized to find the height values (i.e., the z-coordinates) of the graph embedding. */
        enum class Energy_T : Int8
        {
            TV        = 0 /**< Total variation, i.e., sum of absolute values of jump heights; use default algorithm */
            , Dirichlet = 1 /**< Discrete Dirichlet energy, i.e., sum of squared of jump heights. */
            , Bending   = 2 /**< Discrete bending energy, i.e., sum of squared second finite differences of heights. */
            , Height    = 3 /**< Total height of diagram. */
            , TV_CLP    = 4 /**< Total variation, i.e., sum of absolute values of jump heights; use CLP for optimization. */
            , TV_MCF    = 5  /**< Total variation, i.e., sum of absolute values of jump heights; use MCFClass for optimization. */
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
        
        /**@brief Control `struct` for holding settings of `Reapr`.
         *
         * Typically, one wants to change only the settings of `permute_randomQ`, `randomize_bends`, and `randomize_virtual_edgesQ` (to turn randomization on or off) or `energy` (for different ways to determine the z-coordinates of the embedding.
         */
        struct Settings_T
        {
            bool permute_randomQ       = true; /**< Randomly permute the crossings and arcs of the input diagram. Use this if you want to generate many diagrams that are as different as possible from each other. */
            Energy_T energy            = Energy_T::TV; /**< Minimize this objective function to find the z-coordinates of the graph embedding. */
            OrthoDrawSettings_T ortho_draw_settings = {
                .randomize_bends  = 4,
                .randomize_virtual_edgesQ = true
            }; /**< Use these options for `OrthoDraw` to determin the x- and y-coordinates of the graph embedding. */
            Real   scaling             = Real(1); /**< Scaling applied to z-direction _after_ the z-coordinates are scaled so that the embedding is roughly as high as its maximal extension in x- and z-direction. */
            Real   dirichlet_reg       = Real(0.00001); /**< Regularization parameter for the Dirichlet energy; only relevant if `Energy_T::Dirichlet` is used. */
            Real   bending_reg         = Real(0.00001); /**< Regularization parameter for the Dirichlet energy; only relevant if `Energy_T::Bending` is used. */
            Real   backtracking_factor = Real(0.25); /**< Scale stepsize by this in each step of the line search algorith. Only relevant if `Energy_T::Dirichlet` or `Energy_T::Bending` is used. */
            Real   armijo_slope        = Real(0.001); /**< Armijo parameter for the line search algorith. Only relevant if `Energy_T::Dirichlet` or `Energy_T::Bending` is used. */
            Real   tolerance           = Real(0.00000001); /**< Tolerance used for the stopping criterion of the semi-smooth Newton algorithm. Only relevant if `Energy_T::Dirichlet` or `Energy_T::Bending` is used. */
            Size_T SSN_max_b_iter      = 20; /**< Maximal number of backtracking steps per line search. Only relevant if `Energy_T::Dirichlet` or `Energy_T::Bending` is used. */
            Size_T SSN_max_iter        = 1000; /**< Maximal number of steps in the semi-smooth Newton algorithm. Only relevant if `Energy_T::Dirichlet` or `Energy_T::Bending` is used. */
        };
        
        friend std::string ToString( cref<Settings_T> args )
        {
            return std::string("{ ")
            +   ".permute_randomQ = " + ToString(args.permute_randomQ)
            + ", .energy = " + ToString(args.energy)
            + ", .ortho_draw_settings = " + ToString(args.ortho_draw_settings)
            + ", .scaling = " + ToString(args.scaling)
            + ", .dirichlet_reg = " + ToString(args.dirichlet_reg)
            + ", .bending_reg = " + ToString(args.bending_reg)
            + ", .backtracking_factor = " + ToString(args.backtracking_factor)
            + ", .armijo_slope = " + ToString(args.armijo_slope)
            + ", .tolerance = " + ToString(args.tolerance)
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
        
        /**@brief Initialize by an instance of `Settings_T` to configure the behavior. */
        Reapr( cref<Settings_T> settings_ = Settings_T() )
        : settings { settings_ }
        {}
        
        ~Reapr() = default;
        
        /**@brief We redefine the copy constructor because of `random_engine`; every instance of `Reapr` needs its own pseudorandom number generator. */
        Reapr( const Reapr & other )
        :   settings      { other.settings                    }
        ,   random_engine { InitializedRandomEngine<PRNG_T>() }
        {}
        
    private:
        
        /**@brief Return the flag that signals which objective function to minimize. */
        Energy_T EnergyFlag( mref<PD_T> pd ) const
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.GetCache(tag);
        }
        
        /**@brief Change the objective function. */
        void SetEnergyFlag( mref<PD_T> pd )
        {
            const std::string tag = MethodName("EnergyFlag");
            
            return pd.SetCache(tag,settings.energy);
        }
        
        /**@brief Return the number of iterations used by the semi-smooth Newton method. */
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
        
        /**@brief Return the settings currently in use. */
        mref<Settings_T> Settings()
        {
            return settings;
        }
        
        /**@brief Expose the pseudorandom number generator. */
        mref<PRNG_T> RandomEngine()
        {
            return random_engine;
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

#include "Reapr/DirichletHessian.hpp"
#include "Reapr/BendingHessian.hpp"
#include "Reapr/LevelsConstraintMatrix.hpp"
#include "Reapr/LevelsLP_MCF.hpp"
#include "Reapr/LevelsMinHeight.hpp"
        
#ifdef KNOODLE_USE_UMFPACK
    #include "Reapr/LevelsQP_SSN.hpp"
#endif
        
#ifdef KNOODLE_USE_CLP
    #include "Reapr/LevelsLP_CLP.hpp"
#endif
        
#include "Reapr/Embedding.hpp"
#include "Reapr/RandomRotation.hpp"
        
    public:
        
        template<typename R = Real, typename I = Int, typename J = Int>
        Sparse::MatrixCSR<R,I,J,Sequential> Hessian( cref<PD_T> pd ) const
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
                    
                    return Sparse::MatrixCSR<R,I,J,Sequential>();
                }
            }
        }
        
        /**@brief Compute the z-coordinates of the graph embedding and return them in a linear buffer.  */
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
            return std::string("Reapr")
            + "<" + TypeName<Real>
            + "," + TypeName<Int>
            + ">";
        }
        
    }; // class Reapr

} // namespace Knoodle
