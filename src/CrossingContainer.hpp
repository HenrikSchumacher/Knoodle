#pragma once

namespace Knoodle
{
        
    // This is basically a Tensor3 whose last two dimensions equal 2. This way we can help the compiler to speed up the indexing operations a little. (The compiler has the discretion to use fused shift-load operations.)
    
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

        TOOLS_FORCE_INLINE mptr<Int> data()
        {
            return a;
        }
        
        TOOLS_FORCE_INLINE cptr<Int> data() const
        {
            return a;
        }
        
        TOOLS_FORCE_INLINE mptr<Int> data( const Int i )
        {
            return &a[4 * i];
        }
        
        TOOLS_FORCE_INLINE cptr<Int> data( const Int i ) const
        {
            return &a[4 * i];
        }

        TOOLS_FORCE_INLINE mptr<Int> data( const Int i, const bool j )
        {
            return &a[4 * i + 2 * j];
        }
        
        TOOLS_FORCE_INLINE cptr<Int> data( const Int i, const bool j ) const
        {
            return &a[4 * i + 2 * j];
        }
        
        TOOLS_FORCE_INLINE mptr<Int> data( const Int i, const bool j, const bool k)
        {
            return &a[4 * i + 2 * j + k];
        }
        
        TOOLS_FORCE_INLINE mptr<Int> data( const Int i, const bool j, const bool k) const
        {
            return &a[4 * i + 2 * j + k];
        }
        
        TOOLS_FORCE_INLINE mref<Int> operator()( const Int i, const bool j, const bool k)
        {
            return a[4 * i + 2 * j + k];
        }
            
        TOOLS_FORCE_INLINE cref<Int> operator()( const Int i, const bool j, const bool k) const
        {
            return a[4 * i + 2 * j + k];
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("CrossingContainer") + "<" + TypeName<Int> + ">";
        }
        
    }; // class CrossingContainer

        
} // namespace Knoodle
