#pragma once

namespace KnotTools
{
    template<typename Real_, typename Int_>
    class alignas( ObjectAlignment ) PolygonFolder
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;

        
        static constexpr Int AmbDim = 3;
        
        using Tree_T        = AABBTree<AmbDim,Real,Int>;
        using BContainer_T  = typename Tree_T::BContainer_T;
        using Matrix_T      = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        using Vector_T      = Tiny::Vector<AmbDim,Real,Int>;
        using PRNG_T        = std::mt19937_64;
        using PRNG_Result_T = PRNG_T::result_type;
        
        PolygonFolder() = default;
        
        template<typename ExtReal, typename ExtInt>
        PolygonFolder( const ExtInt vertex_count_, const ExtReal radius_  )
        :   n       { int_cast<Int>(vertex_count_) }
        ,   r       { static_cast<Real>(radius_)   }
        ,   r2      { r * r                        }
        ,   X       { n, AmbDim                    }
        ,   P       { n, AmbDim                    }
        ,   Q       { n, AmbDim                    }
        ,   P_tree  { n                            }
        ,   P_boxes { P_tree.AllocateBoxes()       }
        ,   Q_boxes { P_tree.AllocateBoxes()       }
        {
            P_tree = Tree_T();
            
            using RNG_T = std::random_device;
            
            constexpr Size_T state_size = PRNG_T::state_size;
            constexpr Size_T seed_size = state_size * sizeof(PRNG_T::result_type) / sizeof(RNG_T::result_type);
            
            std::array<RNG_T::result_type,seed_size> seed_array;
            
            std::generate( seed_array.begin(), seed_array.end(), RNG_T() );
            
            std::seed_seq seed ( seed_array.begin(), seed_array.end() );
            
            random_engine = PRNG_T( seed );
        }
        
        ~PolygonFolder() = default;
        
    private:
        
        Int  n  = 0;
        Real r  = 1;
        Real r2 = 1;
        
        // The whole polygon.
        Tensor2<Real,Int> X;
        
        // The whole two arcs; potentially rotated.
        Tensor2<Real,Int> P;
        Tensor2<Real,Int> Q;
        
        Int P_size = 0;
        Int Q_size = 0;
        
        Tree_T P_tree;
        Tree_T Q_tree;
        
        BContainer_T P_boxes;
        BContainer_T Q_boxes;
        
        // Pivot positions.
        Int p = 0;
        Int q = 0;
        
        // Fold angle.
        Real theta = 0;
        
        // Rotation matrix.
        Matrix_T A;
        
        // Coordinates of pivot vertices.
        Vector_T  X_p;
        Vector_T  X_q;
        
        PRNG_T random_engine;
        
    public:

        Int VertexCount() const
        {
            return X.Dimension(0);
        }
        
        Real Radius() const
        {
            return r;
        }
        
        template<typename ExtReal>
        void ReadCoordinates( cptr<ExtReal> X_ )
        {
            X.Read(X_);
        }
        
        cref<Tensor2<Real,Int>> Coordinates() const
        {
            return X;
        }

        Int Pivot_0() const
        {
            return p;
        }
        
        Int Pivot_1() const
        {
            return q;
        }
        
        cref<Matrix_T> RotationMatrix() const
        {
            return A;
        }
        
    public:
        
        Int FoldRandom( const Int step_count )
        {
            Int counter = 0;
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            for( Int step = 0; step < step_count; ++step )
            {
                const Int  i     = u_int (random_engine);
                const Int  j     = unif_int(i+1,n-1)(random_engine);
                const Real angle = u_real(random_engine);
                
                counter += Fold( i, j, angle );
            }
            
            return counter;
        }
        
        
        // Returns true when an actual change has been made.
        
        bool Fold( const Int pivot_0, const Int pivot_1, const Real angle )
        {
            ptic(ClassName()+"::OverlapQ");
            
            std::tie(p,q) = MinMax(pivot_0,pivot_1);
            
            theta = angle;
            
            P_size = q - p - 1;
            Q_size = n - q + p - 1;
            
            if( (P_size <= 0) || (Q_size <= 0) )
            {
                return false;
            }
            
            X_p.Read( X.data(p) );
            X_q.Read( X.data(q) );
            
            // Rotation axis.
            Vector_T u = X_q - X_p;
            u.Normalize();
            
            const Real c = std::cos(theta);
            const Real s = std::sin(theta);
            const Real d = Scalar::One<Real> - c;
            
            A[0][0] = u[0] * u[0] * d + c;
            A[0][1] = u[0] * u[1] * d - u[2] * s;
            A[0][2] = u[0] * u[2] * d + u[1] * s;
            
            A[1][0] = u[1] * u[0] * d + u[2] * s;
            A[1][1] = u[1] * u[1] * d + c;
            A[1][2] = u[1] * u[2] * d - u[0] * s;
            
            A[2][0] = u[2] * u[0] * d - u[1] * s;
            A[2][1] = u[2] * u[1] * d + u[0] * s;
            A[2][2] = u[2] * u[2] * d + c;
            
            Vector_T v;
            Vector_T w;
            
            if( P_size <= Q_size )
            {
                // P has vertices {p+1,...,q-1}
                for( Int i = p + 1; i < q; ++i )
                {
                    v.Read( X.data(i) );
                    v -= X_p;
                    Dot<AddTo_T::False>(A,v,w);
                    w += X_p;
                    w.Write( P.data(i-p-1) );
//                    v.Write( P.data(i-p-1) );
                }
                
                // Q has vertices {q+1,...,n-1} U {0,...,p-1}
                copy_buffer( X.data(q+1), Q.data(0       ), AmbDim * (Q_size-p) );
                copy_buffer( X.data(0  ), Q.data(Q_size-p), AmbDim * p          );
            }
            else
            {
                // P has vertices {p+1,...,q-1}
                copy_buffer( X.data(p+1), P.data(0  ), AmbDim * P_size );
                
                
                // Q has vertices {q+1,...,n-1} U {0,...,p-1}
                for( Int i = q + 1; i < n; ++i )
                {
                    v.Read( X.data(i) );
                    v -= X_p;
                    Dot<AddTo_T::False>(A,v,w);
                    w += X_p;
                    w.Write( Q.data(i-q-1) );
//                    v.Write( Q.data(i-q-1) );
                }
                
                for( Int i = 0; i < p; ++i )
                {
                    v.Read( X.data(i) );
                    v -= X_p;
                    Dot<AddTo_T::False>(A,v,w);
                    w += X_p;
                    w.Write( Q.data((Q_size - p) + i) );
//                    v.Write( Q.data((Q_size - p) + i) );
                }
            }
            
            constexpr Int test_size = 16;
            
            bool overlapQ = false;
            
            const Int P_test_size = Min(test_size,P_size);
            const Int Q_test_size = Min(test_size,Q_size);

            // Test beginning of P against end of Q.
            if( !overlapQ )
            {
                const Int P_pos = 0;
                const Int Q_pos = Q_size - Q_test_size;
                
                overlapQ = overlapQ || OverlapQ( P_pos, P_test_size, Q_pos, Q_test_size );
            }
            
            // Test end of P against beginning of Q.
            if( !overlapQ )
            {
                const Int P_pos = P_size - P_test_size;
                const Int Q_pos = 0;
                
                overlapQ = overlapQ || OverlapQ( P_pos, P_test_size, Q_pos, Q_test_size );
            }
            
            // Test P against Q.
            if( !overlapQ )
            {
                overlapQ = overlapQ || OverlapQ( 0, P_size, 0, Q_size);
            }
            
            if( !overlapQ )
            {
                // P has vertices {p+1,...,q-1}
                copy_buffer( P.data(0       ), X.data(p+1), AmbDim * P_size     );
                
                // Q has vertices {q+1,...,n-1} U {0,...,p-1}
                copy_buffer( Q.data(0       ), X.data(q+1), AmbDim * (Q_size-p) );
                copy_buffer( Q.data(Q_size-p), X.data(0  ), AmbDim * p          );
            }
            
            ptoc(ClassName()+"::OverlapQ");
            
            return (!overlapQ);
        }
        
        
        bool OverlapQ( Int P_pos, Int P_n, Int Q_pos, Int Q_n )
        {
            ptic(ClassName()+"::OverlapQ");
            
            P_tree = Tree_T( P_n );
            Q_tree = Tree_T( Q_n );
            
            P_tree.template ComputeBoundingBoxes<1,AmbDim>( P.data(P_pos), P_boxes );
            Q_tree.template ComputeBoundingBoxes<1,AmbDim>( Q.data(Q_pos), Q_boxes );
            
            constexpr Int max_depth = 128;
            
            Int stack[max_depth][2];
            Int stack_ptr = 0;
            // Push the produce of the root node of S and the root node of T onto stack.
            stack[0][0] = 0;
            stack[0][1] = 0;
            
            ptic("Traverse trees");
            
            while( (0 <= stack_ptr) && (stack_ptr < max_depth - 4) )
            {
                // Pop node i of S and node j of T from stack.
                const Int i = stack[stack_ptr][0];
                const Int j = stack[stack_ptr][1];
                stack_ptr--;
                
                const Real dist_squared = P_tree.BoxBoxSquaredDistance(P_boxes,i,Q_boxes,j);
                
                const bool overlappingQ = (dist_squared < r2);
                
                if( overlappingQ )
                {
                    // "Interior node" means "not a leaf node".
                    const bool i_interiorQ = P_tree.InteriorNodeQ(i);
                    const bool j_interiorQ = Q_tree.InteriorNodeQ(j);
                    
                    if( i_interiorQ || j_interiorQ )
                    {
                        const Int L_i = Tree_T::LeftChild(i);
                        const Int R_i = L_i+1;
                        
                        const Int L_j = Tree_T::LeftChild(j);
                        const Int R_j = L_j+1;
                        
                        if( i_interiorQ == j_interiorQ )
                        {
                            // Split both nodes.
                            
                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = R_j;
                            
                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = L_j;
                            
                            // We push the "off-diagonal" cases last so that they will be popped first.
                            // This is meaningful because the beginning of P is likely to be close to the _end_ of Q and vice versa.
                            
                            ++stack_ptr;
                            stack[stack_ptr][0] = L_i;
                            stack[stack_ptr][1] = R_j;
                            
                            ++stack_ptr;
                            stack[stack_ptr][0] = R_i;
                            stack[stack_ptr][1] = L_j;
                        }
                        else
                        {
                            // split only larger cluster
                            if ( i_interiorQ ) // !j_interiorQ follows from this.
                            {
                                // Split node i.
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = R_i;
                                stack[stack_ptr][1] = j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = L_i;
                                stack[stack_ptr][1] = j;
                            }
                            else
                            {
                                // Split node j.
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = i;
                                stack[stack_ptr][1] = R_j;
                                
                                ++stack_ptr;
                                stack[stack_ptr][0] = i;
                                stack[stack_ptr][1] = L_j;
                            }
                        }
                    }
                    else
                    {
                        // Nodes i and j are overlapping leaf nodes in P_tree and Q_tree, respectively.
                        
                        ptoc("Traverse trees");
                        
                        ptoc(ClassName()+"::OverlapQ");
                        
                        return true;
                    }
                }
            }
            
            ptoc("Traverse trees");
            
            ptoc(ClassName()+"::OverlapQ");
            
            return false;
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("PolygonFolder")+"<"+ToString(AmbDim)+","+TypeName<Real>+","+TypeName<Int>+">";
        }

    }; // PolygonFolder
    
} // namespace KnotTools
