#pragma once

namespace KnotTools
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
        
        using Vector_T    = ClangMatrix<3,1,Real,Int>;
        using Vector4_T   = ClangMatrix<4,1,Real,Int>;
        using Matrix_T    = ClangMatrix<3,3,Real,Int>;
        using Matrix4x4_T = ClangMatrix<4,4,Real,Int>;
       
        ClangQuaternionTransform() = default;
        
        template<typename ExtReal>
        ClangQuaternionTransform( cptr<ExtReal> f_ptr )
        {
            Read(f_ptr);
        }
        
        template<typename ExtReal>
        ClangQuaternionTransform( cptr<ExtReal> q_ptr, cptr<ExtReal> b_ptr )
        {
            ReadQuaternion(q_ptr);
            b.Read(b_ptr);
        }
        
        ~ClangQuaternionTransform() = default;

    private:
        
        Matrix_T A;
        Matrix4x4_T q;
        Vector_T b;

    public:
        
        static constexpr Int Size()
        {
            return 7;
        }
        
        template<typename ExtReal>
        void ReadQuaternion( cptr<ExtReal> f )
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
            
            q.Set(0,0, f[0]); q.Set(0,1,-f[1]); q.Set(0,2,-f[2]); q.Set(0,3,-f[3]);
            q.Set(1,0, f[1]); q.Set(1,1, f[0]); q.Set(1,2,-f[3]); q.Set(1,3, f[2]);
            q.Set(2,0, f[2]); q.Set(2,1, f[3]); q.Set(2,2, f[0]); q.Set(2,3,-f[1]);
            q.Set(3,0, f[3]); q.Set(3,1,-f[2]); q.Set(3,2, f[1]); q.Set(3,3, f[0]);
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> f )
        {
            ReadQuaternion(f);
            b.Read(&f[4]);
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f ) const
        {
            f[0] = q(0,0);
            f[1] = q(1,0);
            f[2] = q(2,0);
            f[3] = q(3,0);
            
            b.Write(&f[4]);
        }
        
        cref<Matrix_T> Matrix() const
        {
            return A;
        }
        
        mref<Matrix_T> Matrix()
        {
            return A;
        }
        
        cref<Matrix4x4_T> Matrix4x4() const
        {
            return q;
        }
        
        mref<Matrix4x4_T> Matrix4x4()
        {
            return q;
        }
        
        cref<Vector_T> Vector() const
        {
            return b;
        }
        
        mref<Vector_T> Vector()
        {
            return b;
        }
        
        void TransformVector( mptr<Real> x_ptr ) const
        {
            Vector_T x ( x_ptr );
            Vector_T y = A * x + b;
            y.Write(x_ptr);
        }
        
        Vector_T operator()( cref<Vector_T> x ) const
        {
            return A * x + b;
        }
        
        void TransformTransform( mptr<Real> f_ptr ) const
        {
            Vector4_T p (&f_ptr[0]);
            Vector4_T r = q * p;
            r.Write(&f_ptr[0]);
            
            Vector_T x (&f_ptr[4]);
            Vector_T c = A * x + b;
            c.Write(&f_ptr[4]);
        }
        
        
        void SetIdentity()
        {
            for( Int j = 0; j < AmbDim; ++j )
            {
                for( Int i = 0; i < AmbDim; ++i )
                {
                    A.Set( i, j, KroneckerDelta<Real>(i,j) );
                }
                
                b.Set( j, Int(0), Real(0) );
            }
            
            for( Int j = 0; j < 4; ++j )
            {
                for( Int i = 0; i < 4; ++i )
                {
                    q.Set( i, j, KroneckerDelta<Real>(i,j) );
                }
            }
        }
        
        static constexpr ClangQuaternionTransform IdentityTransform()
        {
            ClangQuaternionTransform f;
            
            f.SetIdentity();
            
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString( cref<ClangQuaternionTransform> f )
        {
            return std::string("A = ") + ToString(f.A) + "\n" + "b = " + ToString(f.b);
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
    
} // namespace KnotTools
