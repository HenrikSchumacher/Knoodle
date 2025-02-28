#pragma once

namespace KnotTools
{
    template<int AmbDim_, typename Real_, typename Int_>
    class ClangAffineTransform
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int AmbDim  = AmbDim_;
        
        using Vector_T = ClangMatrix<AmbDim,1       ,Real,Int>;
        using Matrix_T = ClangMatrix<AmbDim,AmbDim  ,Real,Int>;
        using M_T      = ClangMatrix<AmbDim,AmbDim+1,Real,Int>;
       
        ClangAffineTransform() = default;
        
        template<typename ExtReal>
        ClangAffineTransform( cptr<ExtReal> f_ptr )
        {
            Read(f_ptr);
        }
        
        template<typename ExtReal>
        ClangAffineTransform( cptr<ExtReal> A_ptr, cptr<ExtReal> b_ptr )
        {
            A.Read(A_ptr);
            b.Read(b_ptr);
        }
        
        ClangAffineTransform( cref<Matrix_T> A_, cref<Vector_T> b_ )
        :   A ( A_ )
        ,   b ( b_ )
        {}
        
        ~ClangAffineTransform() = default;

    private:
        
        Matrix_T A;
        Vector_T b;

    public:
        
        static constexpr Int Size()
        {
            return AmbDim * AmbDim + AmbDim;
        }
        
        
        template<typename ExtReal>
        void ReadMatrix( cptr<ExtReal> A_ptr )
        {
            A.Read(A_ptr);
        }
        
        template<typename ExtReal>
        void ReadVector( cptr<ExtReal> b_ptr )
        {
            b.Read(b_ptr);
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> f_ptr )
        {
            ReadMatrix(f_ptr);
            ReadVector(&f_ptr[AmbDim*AmbDim]);
        }

        
        template<typename ExtReal>
        void WriteMatrix( mptr<ExtReal> A_ptr ) const
        {
            A.Write(A_ptr);
        }
        
        template<typename ExtReal>
        void WriteVector( mptr<ExtReal> b_ptr ) const
        {
            b.Write(b_ptr);
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f_ptr ) const
        {
            WriteMatrix(f_ptr);
            WriteVector(&f_ptr[AmbDim*AmbDim]);
        }
        
        cref<Matrix_T> Matrix() const
        {
            return A;
        }
        
        mref<Matrix_T> Matrix()
        {
            return A;
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
            M_T B ( f_ptr );

            M_T C = A * B;
            
            for( Int i = 0; i < AmbDim; ++i )
            {
                C.Set( i, AmbDim, C(i,AmbDim) + b(i,0) );
            }
            
            C.Write(f_ptr);
        }
        
        
        void SetIdentity()
        {
            for( Int j = 0; j < AmbDim; ++ j )
            {
                for( Int i = 0; i < AmbDim; ++ i )
                {
                    A.Set( i, j, KroneckerDelta<Real>(i,j) );
                }
                
                b.Set( j, Int(0), Real(0) );
            }
        }
        
        static constexpr ClangAffineTransform IdentityTransform()
        {
            ClangAffineTransform f;
            
            f.SetIdentity();
            
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString( cref<ClangAffineTransform> f )
        {
            return std::string("A = ") + ToString(f.A) + "\n" + "b = " + ToString(f.b);
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("ClangAffineTransform")
                + "<" + to_ct_string(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // ClangAffineTransform
    
} // namespace KnotTools
