#pragma once

namespace KnotTools
{

    // This is basically a Tensor2 whose last dimension equals 2. This way we can help the compiler to speed up the indexing operations a little.
    
    template <typename Int_>
    class ArcContainer : public Tensor2<Int_,Int_>
    {

    public:

        using Int      = Int_;
        using Tensor_T = Tensor2<Int,Int>;
        
    public:

        ArcContainer()
        :   Tensor_T()
        {}
        
        explicit ArcContainer( const Int n_ )
        :   Tensor_T(n_, 2)
        {}
        
        ArcContainer( const Int n_, const Int init )
        :   Tensor_T(n_, 2, init)
        {}
        
        ~ArcContainer() = default;
        
    protected:
        
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
            return &a[2 * i];
        }
        
        TOOLS_FORCE_INLINE cptr<Int> data( const Int i ) const
        {
            return &a[2 * i];
        }

        TOOLS_FORCE_INLINE mptr<Int> data( const Int i, const bool j )
        {
            return &a[2 * i + j];
        }
        
        TOOLS_FORCE_INLINE cptr<Int> data( const Int i, const bool j ) const
        {
            return &a[2 * i + j];
        }
        
        TOOLS_FORCE_INLINE mref<Int> operator()( const Int i, const bool j)
        {
            return a[2 * i + j];
        }
            
        TOOLS_FORCE_INLINE cref<Int> operator()( const Int i, const bool j) const
        {
            return a[2 * i + j];
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("ArcContainer") + "<" + TypeName<Int> + ">";
        }
        
    }; // class ArcContainer

        
} // namespace KnotTools
