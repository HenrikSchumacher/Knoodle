#pragma once

// TODO: wrap_aroundQ and centralizeQ

namespace Knoodle
{
    using namespace Tools;
    using namespace Tensors;
    
    /*!
     * @brief Implements the _(Progressive) Action-Angle Method_. The main routines are `CreateRandomClosedPolygon` and `CreateRandomClosedPolygons` to generate one or many closed polygons with unit edge lengths.
     *
     * The class's only purpose is to initial the random number generator and to keep it alive during calls to `CreateRandomClosedPolygon`.
     *
     * @tparam Real_ A real floating point type used for corordinates.
     *
     * @tparam Int_  An integer type used for indices.
     *
     * @tparam Prng_T_ A class of a pseudorandom number generator.
     */
    
    template<
        Size_T  AmbDim_   = 3,
        FloatQ Real_      = double,
        IntQ Int_         = Int64,
        typename Prng_T_  = Knoodle::PRNG_T
    >
    class ConformalBarycenterSampler
    {
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using Prng_T = Prng_T_;
        
        static constexpr Int  AmbDim        = static_cast<Int>(AmbDim_);
        static constexpr bool vectorizeQ    = true;
        static constexpr bool zerofy_firstQ = true;
        
        using Vector_T              = Tiny::Vector<AmbDim,Real,Int>;
        using SquareMatrix_T        = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        using SymmetricMatrix_T     = Tiny::SelfAdjointMatrix<AmbDim,Real,Int>;
        using Weights_T             = Tensor1<Real,Int>;
        
        using VectorList_T          = Tiny::VectorList<AmbDim,Real,Int>;
        using Matrix_T              = Tensor2<Real,Int>;
        using VectorContainer_T     = std::conditional_t<vectorizeQ, VectorList_T, Matrix_T>;

        
        /*!@brief A struct to carry the options for `CoBarS::SamplerBase` and `CoBarS::Sampler`.
         */
        
        struct Settings_T
        {
            Real tolerance            = std::pow(Scalar::eps<Real>,Frac<Real>(7,8));
            Real give_up_tolerance    = 128 * Scalar::eps<Real>;
            Real regularization       = static_cast<Real>(0.0078125);
            Int  max_iter             = 1000;
            
            Real Armijo_slope_factor  = static_cast<Real>(0.0078125);
            Real Armijo_shrink_factor = static_cast<Real>(0.5);
            Int  max_backtrackings    = 20;
            
            bool use_linesearch       = true;
            
            Settings_T()  = default;
            
            ~Settings_T() = default;
            
            void PrintStats() const
            {
                valprint( "tolerance           ", tolerance           );
                valprint( "give_up_tolerance   ", give_up_tolerance   );
                valprint( "regularization      ", regularization      );
                valprint( "max_iter            ", max_iter            );
                valprint( "Armijo_slope_factor ", Armijo_slope_factor );
                valprint( "Armijo_shrink_factor", Armijo_shrink_factor);
                valprint( "max_backtrackings   ", max_backtrackings   );
                valprint( "use_linesearch      ", use_linesearch      );
            }
        };
        
        /*!@brief This struct is used to control how the vertex coordinates output are centralized and in which why they are.
         */
        
        enum struct CentralizationMode_T : UInt8
        {
            None            = 0,    // No centralization at all.
            UnitOnPoints    = 1,    // Unit masses at the vertex positions.
            UnitOnMidpoints = 2,    // Unit masses at edge midpoints.
            UniformOnEdges  = 3     // Mass is smeared uniformly along polygon (normalized line element).
        };
        using M_T = CentralizationMode_T;
                
        ConformalBarycenterSampler() = default;
        
        ~ConformalBarycenterSampler() = default;
        
        explicit ConformalBarycenterSampler(
            const Int edge_count,
            const Settings_T settings = Settings_T()
        )
        :   settings_       { settings                          }
        ,   edge_count_     { edge_count                        }
        ,   random_engine   { InitializedRandomEngine<Prng_T>() }
        ,   r_              { edge_count_, one                  }
        ,   rho_            { edge_count_, one                  }
        ,   total_r_inv     { Frac<Real>(one, edge_count_ )     }
        {
            PrintWarnings();
            ComputeEdgeSpaceSamplingHelper();
            ComputeEdgeQuotientSpaceSamplingHelper();
            
            if constexpr ( vectorizeQ )
            {
                x_ = VectorList_T( edge_count_ );
                y_ = VectorList_T( edge_count_ );
            }
            else
            {
                x_ = Matrix_T( edge_count_, AmbDim );
                y_ = Matrix_T( edge_count_, AmbDim );
            }
        }
        
        explicit ConformalBarycenterSampler(
            const Real * restrict const r,
            const Real * restrict const rho,
            const Int edge_count,
            const Settings_T settings = Settings_T()
        )
        :   settings_       { settings                          }
        ,   edge_count_     { edge_count                        }
        ,   random_engine   { InitializedRandomEngine<Prng_T>() }
        ,   r_              { edge_count_                       }
        ,   rho_            { rho, edge_count_                  }
        {
            PrintWarnings();
            ComputeEdgeSpaceSamplingHelper();
            ComputeEdgeQuotientSpaceSamplingHelper();
            
            if constexpr ( vectorizeQ )
            {
                x_ = VectorList_T( edge_count_ );
                y_ = VectorList_T( edge_count_ );
            }
            else
            {
                x_ = Matrix_T( edge_count_, AmbDim );
                y_ = Matrix_T( edge_count_, AmbDim );
            }
            
            ReadEdgeLengths(r);
        }
        
    private:
        
        Settings_T settings_;
        
        /*!@brief Number of edges in the represented polygon.
         */
        Int edge_count_ = 0;
        
        /*!@brief The instance's own pseudorandom number generator.
         */
        
        mutable Prng_T random_engine;

        
        mutable std::normal_distribution<Real> normal_dist {zero,one};
        
        mutable std::uniform_real_distribution<Real> phi_dist {0,Scalar::TwoPi<Real>};

        /*!@brief The open polyline's unit edge vectors.
         */

        VectorContainer_T x_;
        
        /*!@brief The shifted polyline's unit edge vectors. (After the optimization has succeeded: the closed polygon's unit edge vectors.)
         */
        
        VectorContainer_T y_;
        
//        /*!@brief The closed polyline's vertex coordinates.
//         */
//        VectorContainer_T p_;
                
        
        /*!@brief The edge lengths of the represented polygon.
         */
        
        Weights_T r_ {0};
        
        /*!@brief The weights of the Riemannian metric used on the Cartesian product of unit spheres.
         */
        
        Weights_T rho_ {0};
        
        /*!@brief Inverse of the total arc length of the represented polygon.
         */
        
        Real total_r_inv = one;
        
        /*!@brief The current shift vector in hyperbolic space. (After the optimization has succeeded: the conformal barycenter.)
         */
        
        Vector_T w_;
        
        /*!@brief Right hand side of Newton iteration.
         */
        
        Vector_T F_;
        
        /*!@brief Nabla F(0) with respect to probability measure defined by point cloud `y_` and weights `r_`.
         */
        
        SymmetricMatrix_T DF_;
        
        /*!@brief An auxiliary buffer to store Cholesky factors.
         */
        SymmetricMatrix_T L;
        
        /*!@brief Update direction.
         */
        Vector_T u_;
        
        
        /*!@brief Multiple purpose buffer.
         */
        Vector_T z_;
        
        /*!@brief Number of optimization iterations used so far.
         */
        
        Int iter = 0;
        
        Real squared_residual = 1;
        Real         residual = 1;

        Real edge_space_sampling_helper                  = 1;
        Real edge_quotient_space_sampling_helper         = 1;
        
        mutable Real edge_space_sampling_weight          = -1;
        mutable Real edge_quotient_space_sampling_weight = -1;
        
        Real lambda_min = eps;
        Real q_Newton = one;
        Real error_estimator = infty;
        
        bool linesearchQ = true;    // Toggle line search.
        bool succeededQ  = false;   // Whether algorithm has succeded.
        bool continueQ   = true;    // Whether to continue with the main loop.
        bool ArmijoQ     = false;   // Whether Armijo condition was met last time we checked.
        
    private:
        
        static constexpr Real zero              = 0;
        static constexpr Real half              = 0.5;
        static constexpr Real one               = 1;
        static constexpr Real two               = 2;
        static constexpr Real three             = 3;
        static constexpr Real four              = 4;
        static constexpr Real eps               = std::numeric_limits<Real>::min();
        static constexpr Real infty             = std::numeric_limits<Real>::max();
        static constexpr Real small_one         = 1 - 16 * eps;
        static constexpr Real big_one           = 1 + 16 * eps;
        static constexpr Real g_factor          = 4;
        static constexpr Real g_factor_inv      = one/g_factor;
        static constexpr Real norm_threshold    = 0.99 * 0.99 + 16 * eps;
        static constexpr Real two_pi            = Scalar::TwoPi<Real>;
        
    public:
        
#include "ConformalBarycenterSampler/ClosedPolygon.hpp"
#include "ConformalBarycenterSampler/Compute.hpp"
#include "ConformalBarycenterSampler/Methods.hpp"
#include "ConformalBarycenterSampler/OpenPolygon.hpp"
#include "ConformalBarycenterSampler/Optimization.hpp"
#include "ConformalBarycenterSampler/Reweighting.hpp"
        
    public:
        
        /*!@brief Returns the current setting of the `CoBarS::SamplerBase` instance.
         */
        const Settings_T & Settings() const { return settings_; }
        
        /*!@brief Returns the dimension of the ambient space.
         */
        static constexpr Int AmbientDimension() { return AmbDim; }
        
        /*!@brief Returns the number of edges.
         */
        Int EdgeCount() const { return edge_count_; }

        /*!@brief Returns the list of edge lengths.
         */
        const Weights_T & EdgeLengths() const { return r_; }

        /*!@brief Reads a new list of edge lengths from buffer `r`.
         *
         * @param r Buffer containing the new edge lengths; assumed to have length `this->EdgeCount()`.
         */
        void ReadEdgeLengths( const Real * const r )
        {
            r_.Read(r);
            total_r_inv = Inv( r_.Total() );
        }

        /*!@brief Returns the list of weights for the Riemannian metrics on the product of unit spheres.
         */
        const Weights_T & MetricWeights() const { return rho_; }

        /*!@brief Reads a new list of edge weights for the Riemannian metric from buffer `rho`.
         *
         * @param rho Buffer containing the new weights; assumed to have length `this->EdgeCount()`.
         */
        void ReadMetricWeights( const Real * const rho ) { rho_.Read(rho); }
        

        /*!@brief Generates a random open polygon and writes its vertex positions to the buffer `p`.
         *
         * Let `n = this->EdgeCount()` and `d = this->AmbientDimension()`.
         *
         * @param p The output array for the open polygons; it is assumed to have size at least `(n + 1) * d`. The `j`-th coordinate of the `i`-vertex in the `k`-th polygon is stored in `p[d * i + j]`. The first vertex is duplicated and appended.
         *
         * @param mode Specify whether the output polygon `p` will be be centered to its center of mass and in which sense "mass" is operationalized.
         */
        void WriteRandomOpenPolygon(
            Real * restrict const p,
            CentralizationMode_T mode
        )
        {
            TOOLS_PTIMER(timer,MethodName("WriteRandomOpenPolygon"));
//            RandomizeInitialEdgeVectors();
//            WriteVertexCoordinates(p,mode);
            
            Real dummy_0;
            Real dummy_1;
            
            Compute<0,0,0,1,0,0,0,0,0>(
                nullptr, nullptr, nullptr, p, nullptr, nullptr, nullptr, dummy_0, dummy_1,
                mode, M_T::None, false
            );
        }
    
        /*!@brief Generates a random open polygon, closes it, and writes the relevant information to the supplied buffers.
         *
         * Let `n = this->EdgeCount()` and `d = this->AmbientDimension()`.
         *
         * @param q The output array for the _closed_ polygon; it is assumed to have size at least `(n + 1) * d`. The first vertex position is duplicated and appended to at the end of each polygon. The `j`-th coordinate of the `i`-vertex is stored in `q[d * i + j]`.
         *
         * @param K Where to store the sampling weight. The type of sampling weights is determined by the value of `quotient_space_Q` (see below).
         *
         * @param wrap_aroundQ If set to yes, then the output polygon `q` will have `EdgeCount()` + 1 vertex position, somewhat repeating the first one. (The difference between first and last vertex positions indicates the numerical error.)
         *
         * @param mode Specify whether the output polygon `q` will be be centered to its center of mass and in which sense "mass" is operationalized.
         *
         *  @param quotient_space_weightQ If set to true (default), then the sampling weights of the polygon space modulo rotation group are returned; otherwise the sampling weights of the polygon space (rotation group not modded out) are returned.
         */

        void WriteRandomClosedPolygon(
            Real * restrict const q,
            Real & restrict       K,
            CentralizationMode_T  mode,
            const bool            wrap_aroundQ,
            const bool            quotient_space_weightQ = true
        )
        {
            TOOLS_PTIMER(timer,MethodName("WriteRandomClosedPolygon"));
            
            Real dummy;
            
            if( quotient_space_weightQ )
            {
                Compute<0,0,0,0,0,0,1,0,1>(
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, q, dummy, K,
                    M_T::None, mode, wrap_aroundQ
                );
            }
            else
            {
                Compute<0,0,0,0,0,0,1,1,0>(
                    nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, q, K, dummy,
                    M_T::None, mode, wrap_aroundQ
                );
            }
        }
        
        
        template<FloatQ Real2 = Real, IntQ Int2 = Int, FloatQ BReal2 = float>
        std::pair<LinkEmbedding<Real2,Int2,BReal2>,Real2> RandomEquilateralLink(
            cptr<Real2> component_centers,
            const Int2  component_count,
            const bool  quotient_space_weightQ = true
        )
        {
            Tensor1<Int2,Int2> colors ( component_count );
            Tensor1<Int2,Int2> component_ptr( component_count + Int(1) );
            component_ptr[0] = 0;
            
            Real K_prod = 1;
            Real K;
            
            for( Int2 lc = 0; lc < component_count; ++lc )
            {
                colors[lc] = lc;
                component_ptr[lc+1] = component_ptr[lc] + edge_count_;
            }
            
            Tensor2<Real2,Int2> q ( component_ptr[component_count], Int2(3) );
            
            for( Int2 lc = 0; lc < component_count; ++lc )
            {
                const Int i_begin = component_ptr[lc    ];
                const Int i_end   = component_ptr[lc + 1];
                
                WriteRandomClosedPolygon(
                    q.data(i_begin), K, M_T::UniformOnEdges, false, quotient_space_weightQ
                );
                
                K_prod *= K;
                
                if( component_centers != nullptr )
                {
                    Tiny::Vector<3,Real2,Int2> center ( &component_centers[Int2(3) * lc] );
                    
                    // TODO: We could merge this into WriteRandomEquilateralPolygon, I think.
                    for( Int i = i_begin; i < i_end; ++i )
                    {
                        q(i,0) += center[0];
                        q(i,1) += center[1];
                        q(i,2) += center[2];
                    }
                }
            }

            LinkEmbedding<Real2,Int2,BReal2> link ( std::move(component_ptr), std::move(colors) );
            
            link.ReadVertexCoordinates(q.data());
            
            return {link,static_cast<Real2>(K_prod)};
        }
        
        template<FloatQ Real2 = Real, IntQ Int2 = Int, FloatQ BReal2 = float>
        std::pair<LinkEmbedding<Real2,Int2,BReal2>,Real2> RandomEquilateralLink(
            const Int2  component_count,
            const bool  quotient_space_weightQ = true
        )
        {
            return RandomEquilateralLink<Real2,Int2,BReal2>( nullptr, component_count, quotient_space_weightQ);
        }
        
        template<FloatQ Real2 = Real, IntQ Int2 = Int, FloatQ BReal2 = float>
        std::pair<LinkEmbedding<Real2,Int2,BReal2>,Real2> RandomEquilateralKnot(
            const Int component_count
        )
        {
            // We add zero vectors here. There should be more efficient ways to do this.
            return RandomEquilateralLink<Real2,Int2,BReal2>(component_count,component_count);
        }
        
    
        /*!@brief Reads a point cloud on the unit sphere from buffer `x`, computes its conformal centralization, and writes the relevant information to the supplied buffers (see below).
         *
         * Let `n = this->EdgeCount()` and `d = this->AmbientDimension()`.
         *
         * This routine interprets `n` as the number of unit vectors per point cloud.
         *
         * @param x The input array for the unit vectors of the uncentered point cloud; it is assumed to have size at least `n * d`. The `j`-coordinate of the `i`-th unit vector of the `k`-th polygon is stored in `x[d * i + j].`
         *
         * @param w The output array for the conformal barycenter of the input point cloud; it is assumed to have size at least `d`.
         *
         * @param y The output array for the unit vectors of the centered point cloud; it is assumed to have size at least `n * d`. The `j`-coordinate of the `i`-th unit vector is stored in `y[d * i + j].`
         *
         * @param K_edge_space The output for the reweighting factors of the point space (rotation group not modded out); it is assumed to have size at least `sample_count`.
         *
         * @param K_quot_space The output array for the reweighting factors of the point space modulo rotation group; it is assumed to have size at least `sample_count`.
         */
        
        void ComputeConformalCentralization(
            const Real * restrict const x,
                  Real * restrict const w,
                  Real * restrict const y,
                  Real & restrict       K_edge_space,
                  Real & restrict       K_quot_space
        )
        {
            TOOLS_PTIMER(timer,MethodName("ComputeConformalCentralization"));
            
            Compute<1,0,0,0,1,1,0,1,1>(
                x, nullptr, nullptr, nullptr, w, y, nullptr, K_edge_space, K_quot_space,
                M_T::None, M_T::None, false
            );
        }
    
    
        /*!@brief Reads an (open) polygon from buffer `p`, computes its conformal closure, and writes the relevant information to the supplied buffers.
         *
         * Let `n = this->EdgeCount()` and `d = this->AmbientDimension()`.
         *
         * This routine interprets `n` as the number of unit vectors of the point cloud.
         *
         * @param p Source array; assumed to be of size of at least `(n + 1) * d`. The coordinates are stored in interleaved form, i.e., the `j`-th coordinate of the `i`-th edgeis stored in `p[d * i + j]`.
         *
         * @param w The output array for the conformal barycenters of the input unit edge vectors of the input polygons; it is assumed to have size at least `sample_count * d`.
         *
         * @param q Target array; assumed to be of size of at least `(n + 1) * d`. The `j`-th coordinate of the `i`-th vertex of the `k`-th polygon is stored in `q[d * i + j]`.
         *
         * @param K_edge_space The output for the reweighting factors of the point space (rotation group not modded out).
         *
         * @param K_quot_space The output for the reweighting factors of the point space modulo rotation group.
         *
         * @param wrap_aroundQ If set to yes, then the output polygon `q` will have `EdgeCount()` + 1 vertex position, somewhat repeating the first one. (The difference between first and last vertex positions indicates the numerical error.)
         *
         * @param mode Specify whether the output polygon `q` will be be centered to its center of mass and in which sense "mass" is operationalized.
         */
        
    public:

        void ComputeConformalClosure(
            const Real * restrict const p,
                  Real * restrict const w,
                  Real * restrict const q,
                  Real & restrict       K_edge_space,
                  Real & restrict       K_quot_space,
            CentralizationMode_T        mode,
            const bool                  wrap_aroundQ
        ) const
        
        {
            TOOLS_PTIMER(timer,MethodName("ComputeConformalClosure"));

            // TODO: It would be meaningful to move the "center" of q to the "center" of p.
            // TODO: If mode == None, then the first vertex of q should equal the first vertex of p.
            
            Compute<0,0,1,0,1,0,1,1,1>(
                nullptr, nullptr, p, nullptr, w, nullptr, q, K_edge_space, K_quot_space,
                M_T::None, mode, wrap_aroundQ
            );
        }
    
        
    public:
        /*! @brief Returns a string that identifies the class's method as specified by `tag`. */
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*! @brief Returns a string that identifies the class. Good for debugging and printing messages. */
        
        static std::string ClassName()
        {
            return std::string("ConformalBarycenterSampler<") + TypeName<Real> + "," + TypeName<Int>  +  ">";
        }
        
    }; // ConformalBarycenterSampler
    
} // namespace Knoodle
