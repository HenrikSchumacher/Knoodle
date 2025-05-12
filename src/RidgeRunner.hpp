#pragma once

namespace Knoodle
{
    // Very rough port of Keenan Crane's experimental implementation.
    // Many magic numbers for which I do not know how to adjust them.
    
    template<
        typename Real_ = double, typename Int_ = std::int64_t, typename LInt_ = std::int64_t
    >
    class RidgeRunner
    {
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        using LInt = LInt_;
        using VertexContainer_T = Tensor2<Real,Int>;
        using PRNG_T = pcg64;
        using Link_T = Link_3D<Real,Int,Real>;
        
        static constexpr Real node_diam = 2.;
        static constexpr Int AmbDim = 3;
        
        static_assert(SignedIntQ<Int>,"");
        static_assert(IntQ<LInt>,"");
        static_assert(FloatQ<Real>,"");
        
        RidgeRunner() = default;
        
        ~RidgeRunner() = default;
        
//        explicit RidgeRunner( cptr<Real> vertex_coords_, const Int vertex_count_ )
//        :   vertex_count  ( vertex_count_ )
//        ,   vertex_coords ( vertex_coords_, vertex_count, AmbDim )
//        ,   vertex_buffer ( vertex_count, AmbDim )
//        {}
        
    private:
        
        Int vertex_count = 0;
        
        VertexContainer_T vertex_coords;
        VertexContainer_T vertex_buffer;
        
        
        Tensor1<Real,Int> rhs_buffer;
        Tensor1<Real,Int> sol_buffer;
        
        Sparse::MatrixCSR<Real,Int,LInt> A;


    public:
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        cref<VertexContainer_T> VertexCoordinates() const
        {
            return vertex_coords;
        }

        mref<VertexContainer_T> VertexCoordinates()
        {
            return vertex_coords;
        }
        
        Real CurveLength() const
        {
            return SONO::CurveLength( vertex_coords.data(), vertex_count );
        }
        
        Real StepSize() const
        {
            return step_size;
        }
        
        void SetStepSize( const Real value )
        {
            step_size = value;
        }
        
        
    public:
        
        
        
        void Step( mref<Link_T> L, const Int step_count )
        {
            if( L.VertexCount() <= Int(1) )
            {
                return;
            }
            
            vertex_coords.template RequireSize<false>( L.EdgeCount() );
            vertex_buffer.template RequireSize<false>( L.EdgeCount() );
            
        }
        
        
//        static Real CurveLength( cptr<Real> x, const Int n )
//        {
//            Real L  = 0;
//
//            for( Int j = 1; j < n; ++j )
//            {
//                const Int i = j - 1;
//                
//                // get the current distance
//                    const Real uij [3] = {
//                    x[3 * j + 0] - x[3 * i + 0],
//                    x[3 * j + 1] - x[3 * i + 1],
//                    x[3 * j + 2] - x[3 * i + 2]
//                };
//
//                L += std::sqrt(uij[0] * uij[0] + uij[1] * uij[1] + uij[2] * uij[2]);
//            }
//            
//            // Handle wrap-around.
//            {
//                Int i = n - 1;
//                Int j = 0;
//                
//                // get the current distance
//                const Real uij [3] = {
//                    x[3 * j + 0] - x[3 * i + 0],
//                    x[3 * j + 1] - x[3 * i + 1],
//                    x[3 * j + 2] - x[3 * i + 2]
//                };
//
//                L += std::sqrt(uij[0] * uij[0] + uij[1] * uij[1] + uij[2] * uij[2]);
//            }
//            
//            return L;
//        }
        
        

        
//        // Classical axpby:
//        // Compute y = alpha * x + beta * y.
//        static void LinearCombine(
//            const Real alpha, cptr<Real> x, const Real beta, mptr<Real> y, const Int n
//        )
//        {
//            for( Int i = 0; i < n; ++i )
//            {
//                y[3 * i + 0] = alpha * x[3 * i + 0] + beta * y[3 * i + 0];
//                y[3 * i + 1] = alpha * x[3 * i + 1] + beta * y[3 * i + 1];
//                y[3 * i + 2] = alpha * x[3 * i + 2] + beta * y[3 * i + 2];
//            }
//        }
        
//        void UpdateEdgeLengths( mptr<Real> x, const Int n )
//        {
//            std::uniform_int_distribution<Int> uniform_dist( Int(0), n - Int(1) );
//            std::uniform_int_distribution<Int> coin_flip   ( Int(0), Int(1) );
//            
//            Int i = uniform_dist( random_engine ); // starting node
//            const Int dir = (coin_flip( random_engine ) ? Int(1) : n - Int(1) ); // direction of traversal
//            
//            const Real curve_length = CurveLength(x,n);
//
//            const Real target_length = curve_length / static_cast<Real>(n);
//
//            // iterate over curve, projecting each edge onto the correct length
//            // (essentially via nonlinear Gauss-Seidel)
//            for( Int k = 0; k < n; ++k )
//            {
//                // Get the next neighbor.
////                Int j = (i + 1) % n;
//                Int j = i + dir;
//                if( j >= n ) { j -= n; }
//
//                // move the vertices to the target length
//
//                // midpoint
//                const Real m [3] = {
//                    Real(0.5) * x[3 * i + 0] + Real(0.5) * x[3 * j + 0],
//                    Real(0.5) * x[3 * i + 1] + Real(0.5) * x[3 * j + 1],
//                    Real(0.5) * x[3 * i + 2] + Real(0.5) * x[3 * j + 2]
//                };
//
//                // get the current distance
//                const Real u [3] = {
//                    x[3 * j + 0] - x[3 * i + 0],
//                    x[3 * j + 1] - x[3 * i + 1],
//                    x[3 * j + 2] - x[3 * i + 2]
//                };
//
//                const Real d = std::sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
//
//                const Real scale = (Real(0.5) * target_length) / d;
//
//                x[3 * i + 0] = m[0] - scale * u[0];
//                x[3 * i + 1] = m[1] - scale * u[1];
//                x[3 * i + 2] = m[2] - scale * u[2];
//
//                x[3 * j + 0] = m[0] + scale * u[0];
//                x[3 * j + 1] = m[1] + scale * u[1];
//                x[3 * j + 2] = m[2] + scale * u[2];
//
//                // go to the next node around the curve
//                i = j;
//            }
//        }
        
//        void UpdateCollisions( mptr<Real> x, const Int n )
//        {
//            Real alpha = Real(1.) - collision_damping;
//            Real beta  = collision_damping;
//            
//            constexpr Real delta   = 0.001; // "wiggle factor" between hard spheres
//            constexpr Real epsilon = 0.0;
//            
//            constexpr Real D_eps = node_diam + epsilon;
//            
//            const Real curve_length = CurveLength(x,n);
//            
//            // Determine how many neighboring nodes to skip.
//            const Real l = curve_length / static_cast<Real>(n);
//            const Real D = node_diam;
//            
//            // TODO: Using the average edge length is a bit fishy as the polygona is never really equilateral.
//            // TODO: One should rather employ the true arc distance here, which can be done efficiently, at least if we "freeze" the edge lengths before the loop.
//            const Int skip = static_cast<Int>( std::ceil( M_PI * D / (Real(2) * l) ) );
//            
//            std::uniform_int_distribution<Int> uniform_dist( Int(0), n - Int(1) );
//            Int i = uniform_dist( random_engine ); // starting node
//
//            // Keep track of the average amount by which nodes overlap;
//            // if no nodes overlap, this amount will default to zero.
//            
//            average_overlap = 0.;
//            Int overlap_count = 0;
//            
//            // Iterate over curve, projecting each pair of nodes onto a non-intersecting configuration (essentially via nonlinear Gauss-Seidel).
//            for( Int k = 0; k < n; ++k )
//            {
//                // TODO: skipping too much? too little? off by 1 on either end?
//                for( Int m = skip + 1; m < n - skip; ++m )
//                {
////                    Int j = (i+m) % n;
//                    Int j = i + m;
//                    if( j >= n ) { j-= n; };
//                    
//                    if( j == i ) continue;
//                    
//                    // distance vector u = x[j] - x[i]
//                    const Real u [3] = {
//                        x[3 * j + 0] - x[3 * i + 0],
//                        x[3 * j + 1] - x[3 * i + 1],
//                        x[3 * j + 2] - x[3 * i + 2]
//                    };
//                    
//                    // current distance
//                    const Real d = std::sqrt( u[0] * u[0] + u[1] * u[1] + u[2] * u[2] );
//                    
//                    if( d < D_eps )
//                    {
//                        average_overlap += std::max(Real(0),D - d);
//                        ++overlap_count;
//                        
//                        // move the vertices to the target length
//                        
//                        // midpoint
//                        const Real m [3] = {
//                            Real(0.5) * x[3 * i + 0] + Real(0.5) * x[3 * j + 0],
//                            Real(0.5) * x[3 * i + 1] + Real(0.5) * x[3 * j + 1],
//                            Real(0.5) * x[3 * i + 2] + Real(0.5) * x[3 * j + 2]
//                        };
//                        
//                        const Real scale = (0.5*(D+delta)) / d;
//                        
//                        const Real c_i [3] = {
//                            m[0] - scale * u[0],
//                            m[1] - scale * u[1],
//                            m[2] - scale * u[2]
//                        };
//                        
//                        // Applying some damping here (if damping > 0).
//                        // The original SONO algorithm does not do it.
//                        x[3 * i + 0] = alpha * c_i[0] + beta * x[3 * i + 0];
//                        x[3 * i + 1] = alpha * c_i[1] + beta * x[3 * i + 1];
//                        x[3 * i + 2] = alpha * c_i[2] + beta * x[3 * i + 2];
//
//                        const Real c_j [3] = {
//                            m[0] + scale * u[0],
//                            m[1] + scale * u[1],
//                            m[2] + scale * u[2]
//                        };
//                        
//                        // Applying some damping here (if damping > 0).
//                        // The original SONO algorithm does not do it.
//                        x[3 * j + 0] = alpha * c_j[0] + beta * x[3 * j + 0];
//                        x[3 * j + 1] = alpha * c_j[1] + beta * x[3 * j + 1];
//                        x[3 * j + 2] = alpha * c_j[2] + beta * x[3 * j + 2];
//                    }
//                }
//                
//                // Go to the next node around the curve.
//                // i = (i+1) % n;
//                ++i;
//                if( i >= n ) { i -= n; }
//            }
//            
//            if( overlap_count > 0 )
//            {
//                average_overlap /= static_cast<Real>(overlap_count);
//            }
//        }
        
        
//        void Laplacian_CollectTriples(
//            mref<Aggregator_T> agg,
//            const Int row_offset,
//            const Int col_offset
//        ) const
//        {
//            // Caution: This creates only the triples for the essentially upper triangle.
//            // ("Essentially", because few off-diagonal triples lie in the lower triangle.)
//
//            const Int comp_count   = pd.LinkComponentCount();
//            cptr<Int> comp_arc_ptr = pd.LinkComponentArcPointers().data();
//            cptr<Int> comp_arc_idx = pd.LinkComponentArcIndices().data();
//            cptr<Int> next_arc     = pd.ArcNextArc().data();
//            
//            const Real val_0 = Real(2) + bending_reg;
//            const Real val_1 = Real(-1);
//
//            for( Int comp = 0; comp < comp_count; ++comp )
//            {
//                const Int k_begin   = comp_arc_ptr[comp    ];
//                const Int k_end     = comp_arc_ptr[comp + 1];
//                
//                Int a   = comp_arc_idx[k_begin];
//                Int ap1 = next_arc[a  ];
//                
//                for( Int k = k_begin; k < k_end; ++k )
//                {
//                    const Int row = a + row_offset;
//                    
//                    agg.Push( row, a   + col_offset, val_0 );
//                    agg.Push( row, ap1 + col_offset, val_1 );
//
//                    a   = ap1;
//                    ap1 = next_arc[a];
//                }
//            }
//
//        } // DirichletHessian_CollectTriples
    
    public:
        
        static std::string RidgeRunner()
        {
            return ct_string("RidgeRunner")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }
        
    }; // class RidgeRunner
}
