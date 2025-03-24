#pragma once

namespace Knoodle
{
    template<Size_T M_, Size_T N_, typename Real_, typename Int_>
    class ClangMatrix
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real = Real_;
        using Int  = Int_;
        
        static constexpr Int M = M_;
        static constexpr Int N = N_;
        
        using M_T = mat_T<M,N,Real>;
       
        ClangMatrix() = default;
        
        template<typename ExtReal>
        ClangMatrix( cptr<ExtReal> A_ptr )
        {
            Read(A_ptr);
        }
        
        ClangMatrix( cref<ClangMatrix> A_ )
        :   A (A_.A)
        {}

        ClangMatrix( const Real init )
        {
            fill_buffer<M*N>( get_ptr(A), init );
        }
        
        ClangMatrix( cref<M_T> B )
        :   A ( B )
        {}
        
        ~ClangMatrix() = default;

    private:
        
        M_T A;

    public:
        
        static constexpr Int Size()
        {
            return M * N;
        }
        
        static constexpr Int RowCount()
        {
            return M;
        }
        
        static constexpr Int ColCount()
        {
            return M;
        }
        
        
        Real operator()( const Int i, const Int j ) const
        {
            return A[ToSize_T(i)][ToSize_T(j)];
        }
        
        Real operator[]( const Int i ) const
        {
            return A[ToSize_T(i)][0];
        }
        
        void Set( const Int i, const Int j, const Real val )
        {
            A[ToSize_T(i)][ToSize_T(j)] = val;
        }
        
        template<typename ExtReal>
        void Read( cptr<ExtReal> A_ptr )
        {
            A = __builtin_matrix_column_major_load( A_ptr, M, N, M );
        }
        
        template<typename ExtReal>
        void Write( mptr<ExtReal> A_ptr ) const
        {
            __builtin_matrix_column_major_store( A, A_ptr, M );
        }
        
        cref<M_T> Matrix() const
        {
            return A;
        }
        
        mref<M_T> Matrix()
        {
            return A;
        }
        
        
//        template<Int K>
//        ClangMatrix<M,K,Real,Int> operator()( cref<ClangMatrix<N,K,Real,Int>> B ) const
//        {
//            return ClangMatrix<M,K,Real,Int>( A * B.Matrix() );
//        }
        
        template<Int K>
        void TransformMatrix( const Real * B_ptr, Real * C_ptr ) const
        {
            mat_T<N,K,Real> B = __builtin_matrix_column_major_load(B_ptr, N, K, N );
            
            mat_T<M,K,Real> C = A * B;
            
            __builtin_matrix_column_major_store( C, C_ptr, M );
        }
        
        void SetIdentity()
        {
            for( Size_T j = 0; j < ToSize_T(N); ++ j )
            {
                for( Size_T i = 0; i < ToSize_T(M); ++ i )
                {
                    A[i][j] = KroneckerDelta<Real>(i,j);
                }
            }
        }
        
        void SetZero()
        {
            for( Size_T j = 0; j < ToSize_T(N); ++ j )
            {
                for( Size_T i = 0; i < ToSize_T(M); ++ i )
                {
                    A[i][j] = Real(0);
                }
            }
        }
        
        ClangMatrix & operator*=( cref<Real> a )
        {
            A *= a;
            
            return *this;
        }
        
        Real SquaredNorm() const
        {
            return norm_2_squared<M*N>( get_ptr(A) );
        }
        
        Real Norm() const
        {
            return Sqrt(SquaredNorm());
        }
        
        friend Real SquaredDistance ( cref<ClangMatrix> & a, cref<ClangMatrix> & b )
        {
            ClangMatrix c = a - b;
            
            return c.SquaredNorm();
        }
        
        friend Real Distance ( cref<ClangMatrix> & a, cref<ClangMatrix> & b )
        {
            ClangMatrix c = a - b;
            
            return c.Norm();
        }
        
        ClangMatrix & Normalize()
        {
            ((*this) *= Frac<Real>(1,Norm()));
            
            return *this;
        }
        
        
        friend ClangMatrix operator+(
            cref<ClangMatrix> A_, const Real lambda
        )
        {
            ClangMatrix D ( A_.Matrix() + lambda );
            return D;
        }
        
        friend ClangMatrix operator+(
            const Real lambda, cref<ClangMatrix>A_
        )
        {
            ClangMatrix D ( lambda + A_.Matrix() );
            return D;
        }
        
        
        friend ClangMatrix operator-(
            cref<ClangMatrix> A_, const Real lambda
        )
        {
            ClangMatrix D ( A_.Matrix() - lambda );
            return D;
        }
        
        friend ClangMatrix operator-(
            const Real lambda, cref<ClangMatrix> A_
        )
        {
            ClangMatrix D ( lambda - A_.Matrix() );
            return D;
        }

        friend ClangMatrix operator*(
            cref<ClangMatrix> A_, const Real lambda
        )
        {
            ClangMatrix D ( A_.Matrix() * lambda );
            return D;
        }
        
        friend ClangMatrix operator*(
            const Real lambda, cref<ClangMatrix> A_
        )
        {
            ClangMatrix D ( lambda * A_.Matrix() );
            return D;
        }


        
        friend ClangMatrix operator+( cref<ClangMatrix> B, cref<ClangMatrix> C )
        {
            ClangMatrix D ( B.Matrix() + C.Matrix() );
            return D;
        }
        
        friend ClangMatrix operator-( cref<ClangMatrix> B, cref<ClangMatrix> C )
        {
            ClangMatrix D ( B.Matrix() - C.Matrix() );
            return D;
        }
        
        template<Size_T K>
        friend ClangMatrix<M,K,Real,Int> operator*(
            cref<ClangMatrix> B, cref<ClangMatrix<N,K,Real,Int>> C
        )
        {
            ClangMatrix<M,K,Real,Int> D ( B.Matrix() * C.Matrix() );
            return D;
        }
        
        Real Det() const
        {
            if constexpr ( M == N )
            {
                
                Tiny::Matrix<M,N,Real,int> matrix ( reinterpret_cast<const Real *>( &A ) );
                
                return matrix.Det();
            }
            else
            {
                return 0;
            }
        }
        
        [[nodiscard]] friend std::string ToString(
            cref<ClangMatrix> B,
            std::string line_prefix = std::string("")
        )
        {
            std::string s = "{ ";
            
            for( Size_T i = 0; i < ToSize_T(M); ++ i )
            {
                if( i != 0 )
                {
                    s += " },";
                }
                s += "\n" + line_prefix;

                s += "\t{ " + ToString(B.A[i][0]);
                
                for( Size_T j = 1; j < ToSize_T(N); ++ j )
                {
                    s += ", " + ToString(B.A[i][j]);
                }
            }
            
            s += " }\n";
            s += line_prefix + "}";
            
            return s;
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("ClangMatrix")
                + "<" + ToString(M)
                + "," + ToString(N)
                + "," + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // ClangMatrix
    
} // namespace Knoodle
