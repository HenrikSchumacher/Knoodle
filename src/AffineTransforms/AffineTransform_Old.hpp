#pragma once

namespace Tensors
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
        
        using Flag_T   = AffineTransformFlag_T;
        using Vector_T = Tiny::Vector<AmbDim_,Real,Int>;
        using Matrix_T = Tiny::Matrix<AmbDim,AmbDim,Real,Int>;

        static constexpr Flag_T Id    = Flag_T::Id;
        static constexpr Flag_T NonId = Flag_T::NonId;

        AffineTransform() = default;
        
        template<typename ExtReal>
        AffineTransform(
            cptr<ExtReal> f_ptr,
            Flag_T        f_flag
        )
        :   A    ( f_ptr                   )
        ,   b    ( &f_ptr[AmbDim * AmbDim] )
        ,   flag ( f_flag                  )
        {}
        
        template<typename ExtReal>
        AffineTransform(
            cptr<ExtReal> A_ptr,
            cptr<ExtReal> b_ptr,
            const  Flag_T flag_
        )
        :   A    ( A_ptr )
        ,   b    ( b_ptr )
        ,   flag ( flag_ )
        {}
        
        template<typename ExtReal, typename ExtInt>
        AffineTransform(
            cref<Tiny::Matrix<AmbDim,AmbDim,ExtReal,ExtInt>> A_,
            cref<Tiny::Vector<AmbDim,ExtReal,ExtInt>> & b_,
            const Flag_T flag_
        )
        :   A    ( A_    )
        ,   b    ( b_    )
        ,   flag ( flag_ )
        {}
        
        // Move-like constructor
        AffineTransform(
            Matrix_T && A_,
            Vector_T && b_,
            const Flag_T flag_
        )
        {
            swap( A,    A_    );
            swap( b,    b_    );
            swap( flag, flag_ );
        }
        
        ~AffineTransform() = default;

    private:
        
        Matrix_T A;
        Vector_T b;
        Flag_T   flag;

    public:
        
        static constexpr Int Size()
        {
            return AmbDim * AmbDim + AmbDim;
        }
        
        template<typename ExtReal>
        void ForceReadMatrix( cptr<ExtReal> A_ptr )
        {
            A.Read(A_ptr);
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
            ForceReadMatrix( f_ptr );
            ForceReadVector( &f_ptr[AmbDim * AmbDim] );
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> f_ptr, const Flag_T f_flag )
        {
            flag = f_flag;
            
            if( NontrivialQ() )
            {
                ForceReadMatrix( f_ptr );
                ForceReadVector( &f_ptr[AmbDim * AmbDim] );
            }
        }
        
        template<typename ExtReal>
        void ForceWriteMatrix( mptr<ExtReal> A_ptr ) const
        {
            A.Write(A_ptr);
        }
        
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
            A.Write( f_ptr );
            b.Write( &f_ptr[AmbDim * AmbDim] );
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> f_ptr, mref<Flag_T> f_flag ) const
        {
            f_flag = flag;

            if( NontrivialQ() )
            {
                A.Write( f_ptr );
                b.Write( &f_ptr[AmbDim * AmbDim] );
            }
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
        
        bool NontrivialQ() const
        {
            return flag == NonId;
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
        
        bool TransformVector( mptr<Real> x_ptr ) const
        {
            if( NontrivialQ() )
            {
                Vector_T x (x_ptr);
                
                Vector_T y = this->operator()(x);
                
                y.Write( x_ptr );
                
                return 1;
            }
            else
            {
                return 0;
            }
        }
        
        AffineTransform operator()( cref<AffineTransform> f ) const
        {
            if( NontrivialQ() )
            {
                if( f.NontrivialQ() )
                {
                    return AffineTransform( A * f.A, A * f.b + b, NonId );
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
        
        bool TransformTransform( mptr<Real> f_ptr, cref<Flag_T> f_flag ) const
        {
            if( NontrivialQ() )
            {
                if( f_flag == NonId )
                {
                    AffineTransform f ( f_ptr, f_flag );
                    AffineTransform g ( A * f.A, A * f.b + b, NonId );
                    
                    // DEBUGGING CANDIDATE
//                    dump(g);
                    
                    g.Write(f_ptr,f_flag);
                    return 1;
                }
                else
                {
                    Write(f_ptr,f_flag);
                    return 0;
                }
            }
            else
            {
                return 0;
            }
        }

        void Reset()
        {
            flag = Id;
        }
        
        void SetIdentity()
        {
            A.SetIdentity();
            b.SetZero();
            flag = Id;
        }
        
        static constexpr AffineTransform IdentityTransform()
        {
            AffineTransform f;
            
            f.SetIdentity();
            
            return f;
        }
        
        
        [[nodiscard]] friend std::string ToString( cref<AffineTransform> f )
        {
            return std::string("A = ") + ToString(f.A) + "\n" + "b = " + ToString(f.b) + " flag = " + ToString(f.flag);
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("AffineTransform")
                + "<" + ToString(AmbDim)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // AffineTransform
    
} // namespace Tensors
