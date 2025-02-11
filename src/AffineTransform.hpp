#pragma once

namespace KnotTools
{
    template<int AmbDim_, typename Real_, typename Int_>
    class alignas( ObjectAlignment ) AffineTransform
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int AmbDim  = AmbDim_;
        
        using Vector_T = Tiny::Vector<AmbDim_,Real,Int>;
        using Matrix_T = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;
        

        AffineTransform() = default;
        
        template<typename ExtReal>
        AffineTransform( cptr<ExtReal> f_ptr )
        :   A  ( f_ptr                   )
        ,   b  ( &f_ptr[AmbDim * AmbDim] )
        {}
        
        template<typename ExtReal>
        AffineTransform( cptr<ExtReal> A_ptr, cptr<ExtReal> b_ptr )
        :   A  ( A_ptr )
        ,   b  ( b_ptr )
        {}
        
        template<typename ExtReal, typename ExtInt>
        AffineTransform(
            cref<Tiny::Matrix<AmbDim,AmbDim,ExtReal,ExtInt>> A_,
            cref<Tiny::Vector<AmbDim,ExtReal,ExtInt>> & b_
        )
        :   A  ( A_ )
        ,   b  ( b_ )
        {}
        
        AffineTransform( Matrix_T && A_, Vector_T && b_ )
        {
            swap( A, A_ );
            swap( b, b_ );
        }
        
        ~AffineTransform() = default;

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
            A.Read( f );
            b.Read( &f[AmbDim * AmbDim] );
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f ) const
        {
            A.Write( f );
            b.Write( &f[AmbDim * AmbDim] );
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
            return A * x + b;
        }
        
        void TransformVector( mptr<Real> x_ptr ) const
        {
            Vector_T x (x_ptr);
            
            Vector_T y = this->operator()(x);
            
            y.Write( x_ptr );
        }
        
        AffineTransform operator()( cref<AffineTransform> f ) const
        {
            return AffineTransform( A * f.A, A * f.b + b );
        }
        
        void TransformTransform( mptr<Real> g_ptr ) const
        {
            AffineTransform g ( g_ptr );
            
            AffineTransform h = this->operator()(g);
            
            h.Write(g_ptr);
        }
        
        
        void SetIdentity()
        {
            A.SetIdentity();
            b.SetZero();
        }
        
        static constexpr AffineTransform IdentityTransform()
        {
            AffineTransform f;
            
            f.SetIdentity();
            
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString( cref<AffineTransform> f )
        {
            return std::string("A = ") + ToString(f.A) + "\n" + "b = " + ToString(f.b);
        }
        
    public:
        
        static std::string ClassName()
        {
            return std::string("AffineTransform")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // AffineTransform
    
} // namespace KnotTools
