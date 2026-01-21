#pragma once

namespace Knoodle
{
    template<
        Size_T AmbDim_,
        typename Real_, typename Int_, typename LInt_
    >
    class alignas( ObjectAlignment ) NaivePolygonFolder final
    {
        static_assert(FloatQ<Real_>,"");
//        static_assert(SignedIntQ<Int_>,"");
//        static_assert(SignedIntQ<LInt_>,"");
        static_assert( AmbDim_ == 3, "Currently only implemented in dimension 3." );
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using LInt   = LInt_;
        
        static constexpr Int AmbDim = AmbDim_;
        
        using Vector_T          = Tiny::Vector<AmbDim,Real,Int>;
        using Matrix_T          = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        using FoldFlag_T        = Int32;
        using FoldFlagCounts_T  = Tiny::Vector<5,LInt,FoldFlag_T>;
        using WitnessVector_T   = Tiny::Vector<2,Int,Int>;
        using VertexContainer_T = Tiny::VectorList_AoS<AmbDim,Real,Int>;
        
        
        using PRNG_T = pcg64;
        using unif_int  = std::uniform_int_distribution<Int>;
        using unif_real = std::uniform_real_distribution<Real>;
        
        template<typename ExtReal, typename ExtInt>
        NaivePolygonFolder(
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_
        )
        :   V_coords                 {vertex_count_                         }
        ,   hard_sphere_diam         { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam { hard_sphere_diam * hard_sphere_diam  }
        ,   random_engine            { InitializedRandomEngine<PRNG_T>()    }
        {
            SetToCircle();
        }
        
        template<typename ExtReal, typename ExtInt>
        NaivePolygonFolder(
            cptr<ExtReal> vertex_coords_,
            const ExtInt vertex_count_,
            const ExtReal hard_sphere_diam_
        )
        :   V_coords                 { vertex_coords_, vertex_count_        }
        ,   hard_sphere_diam         { static_cast<Real>(hard_sphere_diam_) }
        ,   hard_sphere_squared_diam { hard_sphere_diam * hard_sphere_diam  }
        ,   random_engine            { InitializedRandomEngine<PRNG_T>()    }
        {}
        
    private:
        
        VertexContainer_T V_coords;
        
        Real hard_sphere_diam           = 0;
        Real hard_sphere_squared_diam   = 0;
        Real prescribed_edge_length     = 1;
        
        Int p = 0;                      // Lower pivot index.
        Int q = 0;                      // Greater pivot index.
        WitnessVector_T witness {{-1,-1}};
    
        Real theta;                     // Rotation angle
        Vector_T X_p;                   // Pivot location.
        Vector_T X_q;                   // Pivot location.
        Matrix_T pivot_rotation;
        
        PRNG_T random_engine;
        
        unif_real unif_angle { -Scalar::Pi<Real>,Scalar::Pi<Real> };
        
    public:
        
        Int VertexCount() const
        {
            return V_coords.Dim(0);
        }
        
        cref<VertexContainer_T> VertexCoordinates() const
        {
            return V_coords;
        }
        
        void SetToCircle()
        {
            const Int n = VertexCount();
            
            mptr<Real> x = V_coords.data();
            
            const double delta  = Frac<double>( Scalar::TwoPi<double>, n );
            const double radius = Frac<double>( 1, 2 * std::sin( Frac<double>(delta,2) ) );
            
            Tiny::Vector<AmbDim,double,Int> v (double(0));
            
            for( Int vertex = 0; vertex < n; ++vertex )
            {
                const double angle = delta * vertex;
                
                v[0] = radius * std::cos(angle);
                v[1] = radius * std::sin(angle);
                
                v.Write(x,vertex);
            }
        }
        
        bool CollisionQ()
        {
            witness[0] = -1;
            witness[1] = -1;
            
            const Int n = VertexCount();
            
            cptr<Real> x = V_coords.data();

            for( Int i = 0; i < n; ++i )
            {
                Vector_T x_i (x,i);
                
                const Int j_begin = i + Int(2);
                const Int j_end   = (i == Int(0) ? n - Int(1) : n);
                
                for( Int j = j_begin; j < j_end; ++j )
                {
                    Vector_T x_j (x,j);
                    
                    Vector_T delta = x_i - x_j;
                    
                    if( delta.SquaredNorm() < hard_sphere_squared_diam )
                    {
                        witness[0] = i;
                        witness[1] = j;
                        return true;
                    }
                }
            }
            
            return false;
        }
        
        
        public:

        // Generates a random integer in [a,b[.
        Int RandomInteger( const Int a, const Int b )
        {
            return unif_int(a,b)(random_engine);
        }
        
        // Generates a random integer in [a,b[.
        Int RandomVertex()
        {
            return RandomInteger(Int(0),VertexCount());
        }

        Real RandomAngle()
        {
            return unif_angle(random_engine);
        }
        
        std::pair<Int,Int> RandomPivots()
        {
            const Int n = VertexCount();
            
            Int i;
            Int j;
            do
            {
                i = RandomVertex();
                j = RandomVertex();
            }
            while( ModDistance(n,i,j) <= Int(1) );

            return std::pair<Int,Int>( i, j );
        }
        
        static Matrix_T ComputePivotRotation(
            cref<Vector_T> X, cref<Vector_T> Y, Real angle
        )
        {
            // Rotation axis.
            Vector_T u = (Y - X);
            u.Normalize();
            
            const Real cos = std::cos(angle);
            const Real sin = std::sin(angle);
            
            Matrix_T A;
            
            const Real d = Real(1) - cos;
            
            A[0][0] = u[0] * u[0] * d + cos       ;
            A[0][1] = u[0] * u[1] * d - sin * u[2];
            A[0][2] = u[0] * u[2] * d + sin * u[1];
            
            A[1][0] = u[1] * u[0] * d + sin * u[2];
            A[1][1] = u[1] * u[1] * d + cos       ;
            A[1][2] = u[1] * u[2] * d - sin * u[0];
            
            A[2][0] = u[2] * u[0] * d - sin * u[1];
            A[2][1] = u[2] * u[1] * d + sin * u[0];
            A[2][2] = u[2] * u[2] * d + cos       ;
            
            // DEBUGGING
            {
                auto B = Dot(A.Transpose(),A);
                
                B[0][0] -= Real(1);
                B[1][1] -= Real(1);
                B[2][2] -= Real(1);
                
                const Real error = norm_max<9>( &B[0][0] );
                
                if( error > 0.000'000'000'1 )
                {
                    eprint(MethodName("ComputePivotRotation") + ": pivot transformation matrix is not orthogonal!");
                    TOOLS_DDUMP(error);
                }
            }
            
            return A;
        }
        
        int LoadPivots(
            cref<std::pair<Int,Int>> pivots, const Real angle_theta
        )
        {
            std::tie(p,q) = MinMax(pivots.first,pivots.second);
            theta = angle_theta;
            
            const Int n = VertexCount() ;
            const Int mid_size = q - p - Int(1);
            const Int rem_size = n - mid_size - Int(2);
            
            if( (mid_size <= Int(0)) || (rem_size <= Int(0)) ) [[unlikely]]
            {
                return 1;
            }
            
            X_p.Read(V_coords.data(),p);
            X_q.Read(V_coords.data(),q);
            pivot_rotation = ComputePivotRotation( X_p, X_q, theta );
            
            return 0;
        }
        
        void Update()
        {
//            const Int n = VertexCount();
            
            mptr<Real> x = V_coords.data();
            
            if( p + Int(1) == q )
            {
                eprint(MethodName("Update") + ": p + Int(1) == q.");
            }
            
            for( Int i = p + Int(1); i < q; ++i )
            {
                Vector_T x_i (x,i);
                x_i -= X_p;
                x_i  = Dot(pivot_rotation,x_i);
                x_i += X_p;
                x_i.Write(x,i);
            }
        }
        
        void UndoUpdate()
        {
            pivot_rotation = pivot_rotation.Transpose();
            
            Update();
        }
        
        FoldFlag_T Fold(
            cref<std::pair<Int,Int>> pivots,
            const Real theta_,
            const bool check_collisionsQ
        )
        {
            int pivot_flag = LoadPivots(pivots,theta_);
            
            if( pivot_flag != 0 )
            {
                // Folding step aborted because pivots indices are too close.
                return pivot_flag;
            }
            
            Update();

            if( check_collisionsQ && CollisionQ() )
            {
                // Folding step failed; undo the modifications.
                UndoUpdate();
                return 4;
            }
            else
            {
                // Folding step succeeded.
                return 0;
            }
        }
        
        
        FoldFlagCounts_T FoldRandom(
            const LInt accept_count,
            const bool check_collisionsQ
        )
        {
            FoldFlagCounts_T flag_ctrs ( LInt(0) );
            
            while( flag_ctrs[0] < accept_count )
            {
                FoldFlag_T flag = Fold(
                    RandomPivots(),
                    RandomAngle(),
                    check_collisionsQ
                );
                
                ++flag_ctrs[flag];
            }
            
            return flag_ctrs;
        }
        
//###########################################################
//##    Standard interface
//###########################################################
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
    
        static std::string ClassName()
        {
            return ct_string("NaivePolygonFolder")
                + "<" + Tools::ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + ">";
        }
        
    }; // NaivePolygonFolder
    
} // namespace Knoodle
