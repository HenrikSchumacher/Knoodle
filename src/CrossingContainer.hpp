#pragma once

namespace KnotTools
{
        
    // This is basically a Tensor3 whose last two dimensions equal 2. This way we can help the compiler to speed up the indexing operations a little.
    
    template <typename Int_>
    class CrossingContainer : public Tensor3<Int_,Int_>
    {

    public:

        using Int      = Int_;
        using Tensor_T = Tensor3<Int,Int>;
        
    public:

        CrossingContainer()
        :   Tensor_T()
        {}
        
        CrossingContainer( const Int n_ )
        :   Tensor_T(n_, 2, 2)
        {}
        
        CrossingContainer( const Int n_, const Int init )
        :   Tensor_T(n_, 2, 2, init)
        {}

        ~CrossingContainer() = default;

        
        // Copy constructor
        CrossingContainer(const CrossingContainer & other ) = default;

        // Copy assignment
        CrossingContainer & operator=(CrossingContainer other) noexcept
        {
            swap(*this, other);
            return *this;
        }
        
        // Move constructor
        CrossingContainer( CrossingContainer && other) noexcept
        :   CrossingContainer()
        {
            swap(*this, other);
        }
        
        // Move assignment
        CrossingContainer & operator=(CrossingContainer && other) noexcept
        {
            swap(*this, other);
            return *this;
        }
        
        friend void swap(CrossingContainer & A, CrossingContainer & B ) noexcept
        {
            using std::swap;
            
            swap(static_cast<Tensor_T & >(A), static_cast<Tensor_T &>(B));
        }
        
    private:
        
        using Tensor_T::a;

    public:

        force_inline mptr<Int> data()
        {
            return a;
        }
        
        force_inline cptr<Int> data() const
        {
            return a;
        }
        
        force_inline mptr<Int> data( const Int i )
        {
            return &a[4 * i];
        }
        
        force_inline cptr<Int> data( const Int i ) const
        {
            return &a[4 * i];
        }

        force_inline mptr<Int> data( const Int i, const bool j )
        {
            return &a[4 * i + 2 * j];
        }
        
        force_inline cptr<Int> data( const Int i, const bool j ) const
        {
            return &a[4 * i + 2 * j];
        }
        
        force_inline mptr<Int> data( const Int i, const bool j, const bool k)
        {
            return &a[4 * i + 2 * j + k];
        }
        
        force_inline mptr<Int> data( const Int i, const bool j, const bool k) const
        {
            return &a[4 * i + 2 * j + k];
        }
        
        force_inline mref<Int> operator()( const Int i, const bool j, const bool k)
        {
            return a[4 * i + 2 * j + k];
        }
            
        force_inline cref<Int> operator()( const Int i, const bool j, const bool k) const
        {
            return a[4 * i + 2 * j + k];
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("CrossingContainer") + "<" + TypeName<Int> + ">";
        }
        
    }; // class CrossingContainer

        
} // namespace KnotTools
