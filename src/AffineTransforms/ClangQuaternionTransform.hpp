#pragma once

namespace Knoodle
{
    template<typename Real_, typename Int_>
    class ClangQuaternionTransform
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int AmbDim = 2;
        
        using Flag_T      = AffineTransformFlag_T;
        using Vector_T    = ClangMatrix<3,1,Real,Int>;
        using Vector4_T   = ClangMatrix<4,1,Real,Int>;
        using Matrix_T    = ClangMatrix<3,3,Real,Int>;
        using Matrix4x4_T = ClangMatrix<4,4,Real,Int>;
        
        static constexpr Flag_T Id    = Flag_T::Id;
        static constexpr Flag_T NonId = Flag_T::NonId;
       
        ClangQuaternionTransform() = default;
        
        template<typename ExtReal>
        ClangQuaternionTransform(
            cptr<ExtReal> f_ptr,
            const Flag_T  flag_
        )
        {
            Read( f_ptr, flag_ );
        }
        
        template<typename ExtReal>
        ClangQuaternionTransform(
            cptr<ExtReal> q_ptr,
            cptr<ExtReal> b_ptr,
            const Flag_T  flag_
        )
        {
            Read( q_ptr, b_ptr, flag_ );
        }
        
        ~ClangQuaternionTransform() = default;

    private:
        
        Matrix_T A;
        Matrix4x4_T q;
        Vector_T b;
        Flag_T flag = Id;

    public:
        
        static constexpr Int Size()
        {
            return 7;
        }
        
        bool NontrivialQ() const
        {
            return flag == NonId;
        }
        
        bool IdentityQ() const
        {
            return flag == Id;
        }
        
    private:
        
        // Caution: This does not set the flag!
        template<typename ExtReal>
        void ForceRead_A( cptr<ExtReal> f )
        {
            const Real x0 = Scalar::Two<Real> * f[0];
            const Real x1 = Scalar::Two<Real> * f[1];
            const Real x2 = Scalar::Two<Real> * f[2];
            
            const Real q00 = f[0] * f[0];
            const Real q01 = x0   * f[1];
            const Real q02 = x0   * f[2];
            const Real q03 = x0   * f[3];
            const Real q11 = f[1] * f[1];
            const Real q12 = x1   * f[2];
            const Real q13 = x1   * f[3];
            const Real q22 = f[2] * f[2];
            const Real q23 = x2   * f[3];
            const Real q33 = f[3] * f[3];
            
            A.Set( 0, 0, q00 + q11 - (q22 + q33) );
            A.Set( 0, 1, q12 - q03 );
            A.Set( 0, 2, q13 + q02 );
            
            A.Set( 1, 0, q12 + q03 );
            A.Set( 1, 1, q00 + q22 - (q11 + q33) );
            A.Set( 1, 2, q23 - q01 );
            
            A.Set( 2, 0, q13 - q02 );
            A.Set( 2, 1, q23 + q01 );
            A.Set( 2, 2, q00 + q33 - (q11 + q22) );
        }
        
        // Caution: This does not set the flag!
        template<typename ExtReal>
        void ForceRead_q( cptr<ExtReal> f )
        {
            q.Set(0,0, f[0]); q.Set(0,1,-f[1]); q.Set(0,2,-f[2]); q.Set(0,3,-f[3]);
            q.Set(1,0, f[1]); q.Set(1,1, f[0]); q.Set(1,2,-f[3]); q.Set(1,3, f[2]);
            q.Set(2,0, f[2]); q.Set(2,1, f[3]); q.Set(2,2, f[0]); q.Set(2,3,-f[1]);
            q.Set(3,0, f[3]); q.Set(3,1,-f[2]); q.Set(3,2, f[1]); q.Set(3,3, f[0]);
        }
        
    public:
        
        // Caution: This does not set the flag!
        template<typename ExtReal>
        void ForceReadQuaternion( cptr<ExtReal> f )
        {
            ForceRead_A(f);
            ForceRead_q(f);
        }
        
        // Caution: This does not set the flag!
        void ForceReadQuaternion( cref<Vector4_T> q_vec )
        {
            ForceReadQuaternion( reinterpret_cast<const Real *>(&q_vec) );
        }
        
        template<typename ExtReal>
        void ForceReadVector( cptr<ExtReal> b_ptr )
        {
            b.Read(b_ptr);
        }
        
        void ForceReadVector( cref<Vector_T> b_vec )
        {
            b = b_vec;
        }

        template<typename ExtReal>
        void ForceRead( cptr<ExtReal> f_ptr, const Flag_T f_flag )
        {
            flag = f_flag;
            ForceReadQuaternion(f_ptr);
            ForceReadVector(&f_ptr[4]);
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> f_ptr, const Flag_T f_flag )
        {
            if( f_flag == NonId )
            {
                ForceRead( f_ptr, f_flag );
            }
            else
            {
                flag = f_flag;
            }
        }

        // Caution: This does not store the flag!
        template<typename ExtReal>
        void ForceWriteQuaternion( mptr<ExtReal> q_ptr ) const
        {
            q_ptr[0] = q(0,0);
            q_ptr[1] = q(1,0);
            q_ptr[2] = q(2,0);
            q_ptr[3] = q(3,0);
        }
        
        // Caution: This does not store the flag!
        void ForceWriteQuaternion( mref<Vector4_T> q_vec ) const
        {
            ForceWriteQuaternion( reinterpret_cast<Real *>(&q_vec) );
        }
        
        // Caution: This does not store the flag!
        template<typename ExtReal>
        void ForceWriteVector( mptr<ExtReal> b_ptr ) const
        {
            b.Write(b_ptr);
        }
        
        void ForceWriteVector( mref<Vector_T> b_vec ) const
        {
            b_vec = b;
        }

        template<typename ExtReal>
        void ForceWrite( mptr<ExtReal> f_ptr, mref<Flag_T> f_flag ) const
        {
            f_flag = flag;
            ForceWriteQuaternion(f_ptr);
            ForceWriteVector(&f_ptr[4]);
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f_ptr, mref<Flag_T> f_flag ) const
        {
            if( NontrivialQ() )
            {
                ForceWrite( f_ptr, f_flag );
            }
            else
            {
                f_flag = Id;
            }
        }
        
        cref<Matrix_T> Matrix() const
        {
            return A;
        }
        
        cref<Matrix4x4_T> Matrix4x4() const
        {
            return q;
        }
        
        cref<Vector_T> Vector() const
        {
            return b;
        }
        
        // In-place transformation of vector.
        bool TransformVector( mptr<Real> x_ptr ) const
        {
            if( flag == NonId )
            {
                Vector_T x ( x_ptr );
                Vector_T y = A * x + b;
                y.Write(x_ptr);
                
                return true;
            }
            else
            {
                return false;
            }
        }
        
        Vector_T operator()( cref<Vector_T> x ) const
        {
            if( NontrivialQ() )
            {
                return A * x + b;
            }
            else
            {
                return x;
            }
        }
        
        // In-place transformation of transform. Taking advantage of the fact that the quaternion is stored as 4-vector.
        bool TransformTransform( mptr<Real> f_ptr, mref<Flag_T> f_flag ) const
        {
            if( NontrivialQ() )
            {
                if( f_flag == NonId )
                {
                    const Vector4_T p (&f_ptr[0]);
                    const Vector4_T r = q * p;
                    r.Write(&f_ptr[0]);
                    
                    const Vector_T x (&f_ptr[4]);
                    const Vector_T c = A * x + b;
    
                    c.Write(&f_ptr[4]);
                    
                    return 1;
                }
                else
                {
                    Write( f_ptr, f_flag );
                    return 0;
                }
            }
            else
            {
                // Do nothing.
                return 0;
            }
        }
        
        ClangQuaternionTransform operator()( cref<ClangQuaternionTransform> f ) const
        {
            if( flag == NonId)
            {
                if( f.flag == NonId )
                {
                    ClangQuaternionTransform g;
     
                    // When the transformation is loaded already, we unload it first because this allows us to reduce this to a 4x4-matrix x 4-vector multiplication.
                    Vector4_T p;
                    f.ForceWriteQuaternion(p);
                    const Vector4_T r = q * p;
                    f.ForceReadQuaternion(r);
                    
                    const Vector_T c = A * f.Vector() + b;
                    f.b = c;
                    
                    return g;
                }
                else
                {
                    return *this;
                }
                
            }
            else
            {
                return f;
            }
            
        }
        
        static Vector_T Transform(
            cptr<Real> f_ptr, Flag_T f_flag, cref<Vector_T> x
        )
        {
            if( f_flag == NonId )
            {
                ClangQuaternionTransform f;
                // This saves us the construction of q.
                f.ForceRead_A(f_ptr);
                f.ForceReadVector(&f_ptr[4]);
                f.flag = f_flag;
                
                return f(x);
            }
            else
            {
                return x;
            }
        }
        
        void Invert()
        {
            if( flag == NonId )
            {
                Vector4_T q_vec;
                
                ForceWriteQuaternion(q_vec);
                
                // Conjugate quaternion.
                for( Int i = 1; i < 4; ++i )
                {
                    q_vec.Set(i,0,-q_vec(i,0));
                }
                ForceReadQuaternion(q_vec);
                
                b = Real(-1) * (A * b);
            }
        }

//        static ClangQuaternionTransform Transform(
//            cptr<Real> f_ptr, Flag_T f_flag, cref<ClangQuaternionTransform> g
//        )
//        {
//            if( f_flag == NonId )
//            {
//                // TODO: We should not load f at all; instead we should somehow use  g.q and f's 4-vector.
//                ClangQuaternionTransform f;
//                f.ForceRead_q(f_ptr);
//                f.ForceReadVector(&f_ptr[4]);
//                f.flag = f_flag;
//                
//                return f(g);
//            }
//            else
//            {
//                return g;
//            }
//        }

        void Reset()
        {
            flag = Id;
        }
        
        void SetIdentity()
        {
            A.SetIdentity();
            q.SetIdentity();
            b.SetZero();
            flag = Id;
        }
        
        static constexpr ClangQuaternionTransform IdentityTransform()
        {
            ClangQuaternionTransform f;
            f.SetIdentity();
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString(
            cref<ClangQuaternionTransform> f,
            std::string line_prefix = std::string("")
        )
        {
            std::string line_prefix_2 = line_prefix + "\t";
            std::string line_prefix_3 = line_prefix + "\t\t";
            return std::string("<| ")
                +  "\n" + line_prefix_2 + "\"A\""    + " -> " + ToString(f.A, line_prefix_3)
                + ",\n" + line_prefix_2 + "\"b\""    + " -> " + ToString(f.b, line_prefix_3)
                + ",\n" + line_prefix_2 + "\"Flag\"" + " -> " + ToString(ToUnderlying(f.flag))
                +  "\n" + line_prefix + "|>";
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("ClangQuaternionTransform")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // ClangQuaternionTransform
    
} // namespace Knoodle
