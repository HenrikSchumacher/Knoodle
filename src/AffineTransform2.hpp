#pragma once

namespace KnotTools
{
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AffineTransform2
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int AmbDim  = AmbDim_;
        
        using Vector_T = Tiny::Vector<AmbDim_,Real,Int>;
        using Matrix_T = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        

        AffineTransform2() = default;
        
        template<typename ExtReal>
        AffineTransform2( cptr<ExtReal> f_ptr )
        {
            Read(f_ptr);
        }
        
        template<typename ExtReal>
        AffineTransform2( cptr<ExtReal> A_ptr, cptr<ExtReal> b_ptr )
        {
            A.Read(A_ptr);
            b.Read(b_ptr);
        }
        
        template<typename ExtReal, typename ExtInt>
        AffineTransform2(
            cref<Tiny::Matrix<AmbDim,AmbDim,ExtReal,ExtInt>> A_,
            cref<Tiny::Vector<AmbDim,ExtReal,ExtInt>> & b_
        )
        :   A ( A_ )
        ,   b ( b_ )
        {}
        
        AffineTransform2( Matrix_T && A_, Vector_T && b_ )
        {
            swap( A, A_ );
            swap( b, b_ );
        }
        
        ~AffineTransform2() = default;

    private:
        
        Matrix_T A;
        Vector_T b;

    public:
        
        static constexpr Int Size()
        {
            return AmbDim * AmbDim + AmbDim;
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> f )
        {
            copy_matrix<AmbDim,AmbDim>( f        , AmbDim+1, &A[0][0], AmbDim );
            copy_matrix<AmbDim,1>     (&f[AmbDim], AmbDim+1, &b[0]   , Int(1) );
//            A.Read( f );
//            b.Read( &f[AmbDim * AmbDim] );
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f ) const
        {
//            A.Write( f );
//            b.Write( &f[AmbDim * AmbDim] );
            
            copy_matrix<AmbDim,AmbDim>( &A[0][0], AmbDim, f         , AmbDim+1 );
            copy_matrix<AmbDim,1>     ( &b[0]   , Int(1), &f[AmbDim], AmbDim+1 );
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
    
        
        Vector_T operator()( cref<Vector_T> x ) const
        {
            return Dot(A,x) + b;
        }
        
        AffineTransform2 operator()( cref<AffineTransform2> f ) const
        {
            return AffineTransform2( Dot(A,f.A), Dot(A,f.b) + b );
        }
        
//        template<Int n>
//        Tiny::Matrix<AmbDim,n,Real,Int> operator()( cref<Tiny::Matrix<AmbDim,n,Real,Int>> B ) const
//        {
//            Tiny::Matrix<AmbDim,n,Real,Int> C = Dot(A,B);
//            
//            
//            
//            return AffineTransform2( Dot(A,f.A), Dot(A,f.b) + b );
//        }
        
        
        void SetIdentity()
        {
            A.SetIdentity();
            b.SetZero();
        }
        
        static constexpr AffineTransform2 IdentityTransform()
        {
            AffineTransform2 f;
            
            f.SetIdentity();
            
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString( cref<AffineTransform2> f )
        {
            std::stringstream sout;
            sout << "A = " << ToString(f.A) << "\n" ;
            sout << "b = " << ToString(f.b);
            return sout.str();
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("AffineTransform2")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // AffineTransform2
    
} // namespace KnotTools
