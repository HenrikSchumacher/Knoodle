#pragma once

namespace KnotTools
{
    template<typename Real_, typename Int_, typename LInt_>
    class alignas( ObjectAlignment ) PolygonFolder
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(SignedIntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        using LInt = LInt_;

        
        static constexpr Int AmbDim = 3;
        
        using Tree_T         = AABBTree<AmbDim,Real,Int>;
        using BContainer_T   = typename Tree_T::BContainer_T;
        using Matrix_T       = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        using Vector_T       = Tiny::Vector<AmbDim,Real,Int>;
        using PRNG_T         = std::mt19937_64;
        using PRNG_Result_T  = PRNG_T::result_type;

        using Flag_T         = UInt32;
        using FlagCountVec_T = Tiny::Vector<5,LInt,Flag_T>;

        
        PolygonFolder() = default;
        
        template<typename ExtReal, typename ExtInt>
        PolygonFolder( const ExtInt vertex_count_, const ExtReal hard_sphere_diam_  )
        :   n       { int_cast<Int>(vertex_count_) }
        ,   hard_sphere_diam { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam { hard_sphere_diam * hard_sphere_diam }
        ,   X       { n, AmbDim                    }
        ,   Y       { n, AmbDim                    }
        ,   P_tree  { n                            } // for allocating P_boxes and Q_boxes.
        ,   P_boxes { P_tree.AllocateBoxes()       }
        ,   Q_boxes { P_tree.AllocateBoxes()       }
        {
            P_tree = Tree_T();
            
            InitializePRNG();
            
            SetToCircle();
            
            SetTailSize(128);
        }
        
        ~PolygonFolder() = default;
        
    private:
        
        Int  n         = 0;
        Int  tail_size = 0;
        Real hard_sphere_diam         = 1;
        Real hard_sphere_squared_diam = 1;
         
        // Buffer for the polygons's coordiantes.
        Tensor2<Real,Int> X;
        // The working buffer.
        Tensor2<Real,Int> Y;
        
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
        Vector_T X_p;
        Vector_T X_q;
        
        // shift vector
        Vector_T s;
        
        PRNG_T random_engine;
        
        bool tail_checkQ = false;
        
    public:

        Int VertexCount() const
        {
            return X.Dimension(0);
        }
        
        Real HardSphereDiameter() const
        {
            return hard_sphere_diam;
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
        
        Int TailSize() const
        {
            return tail_size;
        }
        
        void SetTailSize( const Int tail_size_ )
        {
            tail_size = Ramp(tail_size_);
            
            
            // Do the tail checks only of polygon is long enough
            tail_checkQ = (tail_size > 0) && (n > 8 * tail_size);
        }
        

        void SetToCircle()
        {
            setToCircle( X.data() );
        }
        
        
    private:
        
        void setToCircle( mptr<Real> x )
        {
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( 1, 2 * std::sin( Frac<double>(delta,2) ) );
            
            for( Int i = 0; i < n; ++i )
            {
                const double angle = delta * i;
                x[3 * i + 0] = radius * std::cos( angle );
                x[3 * i + 1] = radius * std::sin( angle );
                x[3 * i + 2] = 0;
            }
        }
        
        void InitializePRNG()
        {
            using RNG_T = std::random_device;
            
            constexpr Size_T state_size = PRNG_T::state_size;
            constexpr Size_T seed_size = state_size * sizeof(PRNG_T::result_type) / sizeof(RNG_T::result_type);
            
            std::array<RNG_T::result_type,seed_size> seed_array;
            
            std::generate( seed_array.begin(), seed_array.end(), RNG_T() );
            
            std::seed_seq seed ( seed_array.begin(), seed_array.end() );
            
            random_engine = PRNG_T( seed );
            
        }
        
        void ComputeRotationTransform()
        {
            // Rotation axis.
            Vector_T u = X_q - X_p;
            u.Normalize();
            
            const Real cos = std::cos(theta);
            const Real sin = std::sin(theta);
            const Real d = Scalar::One<Real> - cos;
            
            A[0][0] = u[0] * u[0] * d + cos;
            A[0][1] = u[0] * u[1] * d - sin * u[2];
            A[0][2] = u[0] * u[2] * d + sin * u[1];
            
            A[1][0] = u[1] * u[0] * d + sin * u[2];
            A[1][1] = u[1] * u[1] * d + cos;
            A[1][2] = u[1] * u[2] * d - sin * u[0];
            
            A[2][0] = u[2] * u[0] * d - sin * u[1];
            A[2][1] = u[2] * u[1] * d + sin * u[0];
            A[2][2] = u[2] * u[2] * d + cos;
            
            // Compute shift s = X_p - A.X_p.
            s = X_p - Dot(A,X_p);
        }

        void Rotate_Naive( cptr<Real> from, mptr<Real> to, const Int count )
        {
            Vector_T v;
            Vector_T w;

            for( Int i = 0; i < count; ++i )
            {
                v.Read( &from[AmbDim * i] );
                v -= X_p;
                Dot<Overwrite>(A,v,w);
                w += X_p;
                w.Write( &to[AmbDim * i] );
            }
        }
        
        void Rotate_Naive_Shift( cptr<Real> from, mptr<Real> to, const Int count )
        {
            Vector_T v;
            Vector_T w;

            for( Int i = 0; i < count; ++i )
            {
                v.Read( &from[AmbDim * i] );
                w = s;
                Dot<AddTo>(A,v,w);
                w.Write(  &to[AmbDim * i] );
            }
        }
        
        // Definitely slow.
        template<Int chunk_step>
        void Rotate_Shift( cptr<Real> from, mptr<Real> to, const Int count )
        {
            constexpr Int chunk_size = chunk_step * AmbDim;

            const Int chunk_count = FloorDivide(count,chunk_step);

            using M_T = Tiny::Matrix<chunk_step,AmbDim,Real,Int>;
            
            auto B = A.Transpose();

            M_T S;
            for( Int i = 0; i < chunk_step; ++i )
            {
                for( Int j = 0; j < AmbDim; ++j )
                {
                    S(i,j) = s[j];
                }
            }

            for( Int chunk = 0; chunk < chunk_count; ++chunk )
            {
                S.Write(&to[chunk_size * chunk]);

                Tiny::fixed_dot_mm<chunk_step,AmbDim,AmbDim,AddTo>(
                    &from[chunk_size * chunk], &B[0][0], &to[chunk_size * chunk]
                );
            }

            for( Int i = chunk_count * chunk_step; i < count; ++i )
            {
                s.Write(&to[AmbDim * i]);

                Tiny::fixed_dot_mm<1,AmbDim,AmbDim,AddTo>(
                    &from[AmbDim * i], &B[0][0], &to[AmbDim * i]
                );
            }
        }
        
        // Definitely slow.
//        void Rotate_gemm( cptr<Real> from, mptr<Real> to, const Int count )
//        {
//            constexpr Int chunk_step = 64;
//            
//            const Int chunk_count = FloorDivide(count,chunk_step);
//            const Int chunk_size  = chunk_step * AmbDim;
//            
//            // Compute shift s = X_p - A.X_p.
//            Vector_T s = Dot(A,X_p);
//            s -= X_p;
//            s *= (-1);
//            
//            Tensor2<Real,Int> S ( chunk_step, AmbDim );
//            
//            for( Int i = 0; i < chunk_step; ++i )
//            {
//                s.Write( S.data(i) );
//            }
//            
//            auto B = A.Transpose();
//            
//            for( Int chunk = 0; chunk < chunk_count; ++chunk )
//            {
//                S.Write( &to[ chunk_size * chunk ] );
//                
//                BLAS::gemm<Layout::RowMajor,Op::Id,Op::Id>(
//                    chunk_step, AmbDim, AmbDim,
//                    Scalar::One<Real>, &from[ chunk_size * chunk ], AmbDim,
//                                       B.data()                   , AmbDim,
//                    Scalar::One<Real>, &to  [ chunk_size * chunk ], AmbDim
//                );
//            }
//            
//
//            Vector_T v;
//            Vector_T w;
//
//            for( Int i = chunk_count * chunk_step; i < count; ++i )
//            {
//                v.Read( &from[AmbDim * i] );
//                w = s;
//                Dot<AddTo>(A,v,w);
//                w.Write( &to[AmbDim * i] );
//            }
//        }
        
        
        void Rotate_Homogeneous( cptr<Real> from, mptr<Real> to, const Int count )
        {
            // Definitely slow.
            
            constexpr Int chunk_step = 4;
            constexpr Int chunk_size = chunk_step * AmbDim;

            const Int chunk_count = FloorDivide(count,chunk_step);

            using T_T = Tiny::Matrix<4,3,Real,Int>;
            using V_T = Tiny::Matrix<4,4,Real,Int>;
            using W_T = Tiny::Matrix<4,3,Real,Int>;
            
            T_T T;
            
            T[0][0] = A[0][0]; T[0][1] = A[1][0]; T[0][2] = A[2][0]; // T[0][3] = 0;
            T[1][0] = A[0][1]; T[1][1] = A[1][1]; T[1][2] = A[2][1]; // T[1][3] = 0;
            T[2][0] = A[0][2]; T[2][1] = A[1][2]; T[2][2] = A[2][2]; // T[2][3] = 0;
            T[3][0] =    s[0]; T[3][1] =    s[1]; T[3][2] =    s[2]; // T[3][3] = 1;
        

            V_T V;
            W_T W;
            
            V[0][3] = 1;
            V[1][3] = 1;
            V[2][3] = 1;
            V[3][3] = 1;
            
            for( Int chunk = 0; chunk < chunk_count; ++chunk )
            {
                copy_matrix<4,3,Sequential>(
                    &from[ chunk_size * chunk ], 3,
                    &V[0][0],                    4,
                    4, 3, 1
                );

                Dot<Overwrite>(V,T,W);
                
                copy_buffer<12>( &W[0][0], &to[ chunk_size * chunk ] );
            }

            Vector_T v;
            Vector_T w;

            for( Int i = chunk_count * chunk_step; i < count; ++i )
            {
                v.Read( &from[AmbDim * i] );
                w = s;
                Dot<AddTo>(A,v,w);
                w.Write( &to[AmbDim * i] );
            }
        }
        
        template<Int chunk_step>
        void Rotate_Tiny_Mat( cptr<Real> from, mptr<Real> to, const Int count )
        {
            constexpr Int chunk_size = chunk_step * AmbDim;

            const Int chunk_count = FloorDivide(count,chunk_step);

            using M_T = Tiny::Matrix<chunk_step,AmbDim,Real,Int>;

            M_T S;

            for( Int i = 0; i < chunk_step; ++i )
            {
                s.Write( &S[i][0]);
            }

            auto B = A.Transpose();
            
            M_T V;
            M_T W;

            for( Int chunk = 0; chunk < chunk_count; ++chunk )
            {
                V.template Read <Op::Id>( &from[chunk_size * chunk] );
                W = S;
                Dot<AddTo>(V,B,W);
                W.template Write<Op::Id>( &to  [chunk_size * chunk] );
            }

            Vector_T v;
            Vector_T w;

            for( Int i = chunk_count * chunk_step; i < count; ++i )
            {
                v.Read( &from[AmbDim * i] );
                w = s;
                Dot<AddTo>(A,v,w);
                w.Write( &to[AmbDim * i] );
            }
        }
        
        
        void LoadArc( cptr<Real> from, mptr<Real> to, const Int count, const bool rotateQ )
        {
            if ( rotateQ )
            {
//                Rotate_Naive( from, to, count );
                
                Rotate_Naive_Shift( from, to, count );
                
//                Rotate_Shift<4>( from, to, count );

//                  Rotate_Homogeneous( from, to, count );
                
//                Rotate_Homogeneous_Mat( from, to, count );
                
//                Rotate_gemm( from, to, count );
                
//                Rotate_Tiny_Mat<4>( from, to, count );
            }
            else
            {
                copy_buffer( from, to, AmbDim * count );
            }
        }
        
    public:
        
        
        
        FlagCountVec_T FoldRandom_Reference( const LInt step_count )
        {
            FlagCountVec_T counters {0};
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real( -Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            for( Int step = 0; step < step_count; ++step )
            {
                const Int  i     = u_int                   (random_engine);
                const Int  j     = unif_int(i+2,n-1-(i==0))(random_engine);
                const Real angle = u_real                  (random_engine);
                
                Flag_T flag = Fold_Reference( i, j, angle );
                
                ++counters[flag];
            }
            
            return counters;
        }
        
        // Returns 0 when an actual change has been made.
        Flag_T Fold_Reference( const Int i, const Int j, const Real angle )
        {
            TOOLS_PTIC(ClassName()+"::Fold_Reference");
            
            std::tie(p,q) = MinMax(i,j);
            
            theta = angle;
            
            P_size = q - p - 1;
            Q_size = n - q + p - 1;

            if( (P_size <= 0) || (Q_size <= 0) )
            {
                TOOLS_PTOC(ClassName()+"::Fold_Reference");
                return 1;
            }
            
            Flag_T flag = 0;
            
            X_p.Read( X.data(p) );
            X_q.Read( X.data(q) );

            ComputeRotationTransform();
                        
            const bool Q_shorterQ  = (P_size > Q_size);

            const Int  P_ts        = Min(tail_size,P_size);
            const Int  Q_ts        = Min(tail_size,Q_size);
            
            const Int P_begin = 1;
            const Int P_end   = P_begin + P_size;
            const Int Q_begin = P_end + 1;
            const Int Q_end   = n;
            
            X_p.Write( Y.data(0) );
            
            // Load all of P.
            // P has vertices {p+1,...,q-1}
            LoadArc( X.data(p+1), Y.data(P_begin), P_size  , !Q_shorterQ );
            
            X_q.Write( Y.data(P_end) );
            
            // Load all of Q.
            // Q has vertices {q+1,...,n-1} U {0,...,p-1}
            LoadArc( X.data(q+1), Y.data(Q_begin), Q_size-p,  Q_shorterQ );
            LoadArc( X.data(0  ), Y.data(Q_end-p), p       ,  Q_shorterQ );
            
            // Test end of P against front of Q.
            if( tail_checkQ && OverlapQ( P_end - P_ts, P_ts, Q_begin, Q_ts ) )
            {
                flag = 2;
                goto exit;
            }
            
            // Test front of P against end of Q.
            if( tail_checkQ && OverlapQ( P_begin, P_ts, Q_end - Q_ts, Q_ts ) )
            {
                flag = 3;
                goto exit;
            }

            // Test P against Q.
            if( OverlapQ( P_begin, P_size, Q_begin, Q_size ) )
            {
                flag = 4;
                goto exit;
            }
            
            swap(X,Y);
            
        exit:
            
            TOOLS_PTOC(ClassName()+"::Fold_Reference");
            
            return flag;
        }
        
        
        
        FlagCountVec_T FoldRandom( const LInt step_count )
        {
            FlagCountVec_T counters {0};
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            for( Int step = 0; step < step_count; ++step )
            {
                const Int  i     = u_int                   (random_engine);
                const Int  j     = unif_int(i+2,n-1-(i==0))(random_engine);
                const Real angle = u_real                  (random_engine);

                Flag_T flag = Fold( i, j, angle );
                
                ++counters[flag];
            }
            
            return counters;
        }
        
        
        // Returns 0 when an actual change has been made.
        UInt32 Fold( const Int pivot_0, const Int pivot_1, const Real angle )
        {
            TOOLS_PTIC(ClassName()+"::Fold");
            
            std::tie(p,q) = MinMax(pivot_0,pivot_1);
            
            theta = angle;
            
            P_size = q - p - 1;
            Q_size = n - q + p - 1;
            
            if( (P_size <= 0) || (Q_size <= 0) )
            {
                TOOLS_PTOC(ClassName()+"::Fold");
                return 1;
            }
            
            UInt32 flag = 0;
            
            X_p.Read( X.data(p) );
            X_q.Read( X.data(q) );
            
            ComputeRotationTransform();
            
            const Int P_ts = Min(tail_size,P_size);
            const Int Q_ts = Min(tail_size,Q_size);
            
            const bool Q_shorterQ = (P_size > Q_size);
            
            const Int P_begin = 1;
            const Int P_end   = P_begin + P_size;
            const Int Q_begin = P_end + 1;
            const Int Q_end   = n;
            
            const Int k = Min( Q_ts, n - q - 1 );
            const Int l = Ramp( Q_ts - p );
            
            // Test end of P against front of Q.
            if( tail_checkQ )
            {
                // Load end of P and load front of Q..
                // P has vertices {p+1,...,q-1}
                // Q has vertices {q+1,...,n-1} U {0,...,p-1}
                
                LoadArc( X.data(q-P_ts), Y.data(P_end-P_ts), P_ts    , !Q_shorterQ );
                LoadArc( X.data(q+1   ), Y.data(Q_begin   ), k       ,  Q_shorterQ );
                LoadArc( X.data(Int(0)), Y.data(Q_begin+k ), Q_ts - k,  Q_shorterQ );
                
                if( OverlapQ( P_end - P_ts, P_ts, Q_begin, Q_ts ) )
                {
                    flag = 2;
                    goto exit;
                }
            }
            
            // Test front of P against end of Q.
            if( tail_checkQ )
            {
                // Load front of P and end of Q.
                // P has vertices {p+1,...,q-1}
                // Q has vertices {q+1,...,n-1} U {0,...,p-1}
                
                LoadArc( X.data(p+1     ), Y.data(P_begin     ), P_ts  , !Q_shorterQ );
                LoadArc( X.data(n-l     ), Y.data(Q_end-Q_ts  ), l     ,  Q_shorterQ );
                LoadArc( X.data(p-Q_ts+l), Y.data(Q_end-Q_ts+l), Q_ts-l,  Q_shorterQ );
                
                if( OverlapQ( P_begin, P_ts, Q_end - Q_ts, Q_ts ) )
                {
                    flag = 3;
                    goto exit;
                }
            }
            
            // Test P against Q.
            
            X_p.Write( Y.data(0) );
            
            // Load all of P.
            // P has vertices {p+1,...,q-1}
            LoadArc( X.data(p+1), Y.data(P_begin), P_size  , !Q_shorterQ );
            
            X_q.Write( Y.data(P_end) );
            
            // Load all of Q.
            // Q has vertices {q+1,...,n-1} U {0,...,p-1}
            LoadArc( X.data(q+1), Y.data(Q_begin), Q_size-p,  Q_shorterQ );
            LoadArc( X.data(0  ), Y.data(Q_end-p), p       ,  Q_shorterQ );
            
            if( OverlapQ( P_begin, P_size, Q_begin, Q_size) )
            {
                flag = 4;
                goto exit;
            }
            
            swap(X,Y);
            
        exit:
            
            TOOLS_PTOC(ClassName()+"::Fold");
            
            return flag;
        }
        
        
        
        
        // Only for debugging and performance measurement.
        FlagCountVec_T FoldRandom_WithoutChecks( const LInt step_count )
        {
            FlagCountVec_T counters {0};
            
            using unif_int  = std::uniform_int_distribution<Int>;
            using unif_real = std::uniform_real_distribution<Real>;
            
            unif_int  u_int ( Int(0), n-3 );
            unif_real u_real (- Scalar::Pi<Real>,Scalar::Pi<Real> );
            
            for( Int step = 0; step < step_count; ++step )
            {
                const Int  i     = u_int                   (random_engine);
                const Int  j     = unif_int(i+2,n-1-(i==0))(random_engine);
                const Real angle = u_real                  (random_engine);

                
                Flag_T flag = Fold_WithoutCheck( i, j, angle );
                
                ++counters[flag];
            }
            
            return counters;
        }
        
        // Only for debugging and performance measurement.
        // Returns 0 when an actual change has been made.
        UInt32 Fold_WithoutCheck( const Int pivot_0, const Int pivot_1, const Real angle )
        {
            TOOLS_PTIC(ClassName()+"::Fold_WithoutCheck");
            
            std::tie(p,q) = MinMax(pivot_0,pivot_1);
            
            theta = angle;
            
            P_size = q - p - 1;
            Q_size = n - q + p - 1;
            
            if( (P_size <= 0) || (Q_size <= 0) )
            {
                TOOLS_PTOC(ClassName()+"::Fold_WithoutCheck");
                return 1;
            }
            
            X_p.Read( X.data(p) );
            X_q.Read( X.data(q) );
            
            ComputeRotationTransform();
            
            const bool Q_shorterQ = (P_size > Q_size);
            
            const Int P_begin = 1;
            const Int P_end   = P_begin + P_size;
            const Int Q_begin = P_end + 1;
            const Int Q_end   = n;
            
            // Test P against Q.
            
            X_p.Write( Y.data(0) );
            
            // Load all of P.
            // P has vertices {p+1,...,q-1}
            LoadArc( X.data(p+1), Y.data(P_begin), P_size  , !Q_shorterQ );
            
            X_q.Write( Y.data(P_end) );
            
            // Load all of Q.
            // Q has vertices {q+1,...,n-1} U {0,...,p-1}
            LoadArc( X.data(q+1), Y.data(Q_begin), Q_size-p,  Q_shorterQ );
            LoadArc( X.data(0  ), Y.data(Q_end-p), p       ,  Q_shorterQ );
            
            swap(X,Y);
                        
            TOOLS_PTOC(ClassName()+"::Fold_WithoutCheck");
            
            return 0;
        }
        
        
        bool OverlapQ( const Int P_begin, const Int P_n, const Int Q_begin, const Int Q_n )
        {
            TOOLS_PTIC(ClassName()+"::OverlapQ");
            
            bool result = OverlapQ_implementation(P_begin, P_n, Q_begin, Q_n);
            
            TOOLS_PTOC(ClassName()+"::OverlapQ");
            
            return result;
        }
        
    private:
        
        bool OverlapQ_implementation( const Int P_begin, const Int P_n, const Int Q_begin, const Int Q_n )
        {
            P_tree = Tree_T( P_n );
            Q_tree = Tree_T( Q_n );
            
            P_tree.template ComputeBoundingBoxes<1,AmbDim>( Y.data(P_begin), P_boxes );
            Q_tree.template ComputeBoundingBoxes<1,AmbDim>( Y.data(Q_begin), Q_boxes );
            
            constexpr Int max_depth = Tree_T::max_depth;
            
            Int stack [4 * max_depth][2];
            Int stack_ptr = -1;
            
            // Helper routine to manage the stack.
            auto push = [&stack,&stack_ptr]( const Int i, const Int j )
            {
                ++stack_ptr;
                stack[stack_ptr][0] = i;
                stack[stack_ptr][1] = j;
            };
            
            // Helper routine to manage the stack.
            auto conditional_push = [this, push]( const Int i, const Int j )
            {
                const Real dist_squared = Tree_T::BoxBoxSquaredDistance(this->P_boxes,i,this->Q_boxes,j);
                
                const bool overlappingQ = (dist_squared < hard_sphere_squared_diam);
                
                if( overlappingQ )
                {
                    push(i,j);
                }
            };
            
            // Helper routine to manage the stack.
            auto pop = [&stack,&stack_ptr]()
            {
                auto result = std::pair( stack[stack_ptr][0], stack[stack_ptr][1] );
                stack_ptr--;
                return result;
            };
            
            auto continueQ = [&stack_ptr,this]()
            {
                const bool overflowQ = (stack_ptr >= 4 * max_depth - 4);
                
                if( (0 <= stack_ptr) && (!overflowQ) ) [[likely]]
                {
                    return true;
                }
                else
                {
                    if ( overflowQ ) [[unlikely]]
                    {
                        eprint(this->ClassName()+"::OverlapQ_implementation: Stack overflow.");
                    }
                    return false;
                }
            };
            
            push(P_tree.Root(),Q_tree.Root());
            
            
            while( continueQ() )
            {
                auto [i,j] = pop();
                
                // "Interior node" means "not a leaf node".
                const bool i_interiorQ = P_tree.InteriorNodeQ(i);
                const bool j_interiorQ = Q_tree.InteriorNodeQ(j);
                
                if( i_interiorQ || j_interiorQ )
                {
                    if( i_interiorQ == j_interiorQ )
                    {
                        // Split both nodes.
                        auto [L_i,R_i] = Tree_T::Children(i);
                        auto [L_j,R_j] = Tree_T::Children(j);

                        conditional_push(R_i,R_j);
                        conditional_push(L_i,L_j);
                        
                        // We push the "off-diagonal" cases last so that they will be popped first.
                        // This is meaningful because the beginning of P is likely to be close to the _end_ of Q and vice versa.
                        
                        conditional_push(L_i,R_j);
                        conditional_push(R_i,L_j);
                    }
                    else
                    {
                        // split only larger cluster
                        if ( i_interiorQ ) // !j_interiorQ follows from this.
                        {
                            // Split node i.
                            
                            auto [L_i,R_i] = Tree_T::Children(i);
                            conditional_push(R_i,j);
                            conditional_push(L_i,j);
                        }
                        else
                        {
                            // Split node j.
                            auto [L_j,R_j] = Tree_T::Children(j);
                            conditional_push(i,R_j);
                            conditional_push(i,L_j);
                        }
                    }
                }
                else
                {
                    // Nodes i and j are overlapping leaf nodes in P_tree and Q_tree, respectively.
                    return true;
                }
            }
            return false;
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("PolygonFolder")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }

    }; // PolygonFolder
    
} // namespace KnotTools
