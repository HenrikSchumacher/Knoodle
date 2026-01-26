#pragma once

#ifdef KNOODLE_USE_BOOST_UNORDERED
    #include <boost/unordered/unordered_flat_map.hpp>
    #include <boost/unordered/unordered_flat_set.hpp>
#else
    #include <unordered_map>
    #include <unordered_set>
#endif


//#include <boost/unordered/unordered_set.hpp>
//#include <boost/container/flat_set.hpp>

namespace Knoodle
{
#ifdef KNOODLE_USE_BOOST_UNORDERED
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    using AssociativeContainer = boost::unordered_flat_map<Key_T,Val_T,Hash_T>;
#else
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    using AssociativeContainer = std::unordered_map<Key_T,Val_T,Hash_T>;
#endif
    
    
#ifdef KNOODLE_USE_BOOST_UNORDERED
    template<typename Key_T, typename Hash_T = Tools::hash<Key_T>>
    using SetContainer = boost::unordered_flat_set<Key_T,Hash_T>;
#else
    template<typename Key_T, typename Hash_T = Tools::hash<Key_T>>
    using SetContainer = std::unordered_set<Key_T,Hash_T>;
#endif
    
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    TOOLS_FORCE_INLINE void AddTo(
        mref<AssociativeContainer<Key_T,Val_T,Hash_T>> a, cref<Key_T> key, cref<Val_T> val
    )
    {
        static_assert(Tools::IntQ<Val_T>,"");
        if( a.contains(key) )
        {
            a[key] += val;
        }
        else
        {
            a[key]  = val;
        }
    }
    
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    TOOLS_FORCE_INLINE void Increment(
        mref<AssociativeContainer<Key_T,Val_T,Hash_T>> a, cref<Key_T> key
    )
    {
        static_assert(Tools::IntQ<Val_T>,"");
        AddTo(a,key,Val_T(1));
    }
    
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    TOOLS_FORCE_INLINE void Decrement(
        mref<AssociativeContainer<Key_T,Val_T,Hash_T>> a, cref<Key_T> key
    )
    {
        static_assert(Tools::IntQ<Val_T>,"");
        AddTo(a,key,Val_T(-1));
    }
    
    template<typename Key_T, typename Val_T, typename Hash_T = Tools::hash<Key_T>>
    std::string ToString( cref<AssociativeContainer<Key_T,Val_T,Hash_T>> a )
    {
        std::string s ("{ ");
        
        const Size_T n = a.size();
        
        Size_T iter = 0;
        
        for( auto & x : a )
        {
            ++iter;
            s += ToString(x.first) + " -> " + ToString(x.second);
            if( iter < n )
            {
                s += ", ";
            }
        }
        
        s += " }";
        
        return s;
    }
    
    template<typename Int>
    struct IndexPair_T
    {
        static_assert(IntQ<Int>,"");
        Int i;
        Int j;
        
        friend bool operator==( cref<IndexPair_T> a, cref<IndexPair_T> b )
        {
            return (a.i == b.i) && (a.j == b.j);
        }
    };
    
    template<typename Int>
    struct IndexPairHash_T
    {
        using is_avalanching [[maybe_unused]] = std::true_type; // instruct Boost.Unordered to not use post-mixing
        
        inline std::size_t operator()(const IndexPair_T<Int> & p) const
        {
            std::size_t seed = 0;
            hash_combine(seed, p.i);
            hash_combine(seed, p.j);
            return seed;
        }
    };
    
    template<typename Int, typename Scal>
    using MatrixTripleContainer_T
            = AssociativeContainer<IndexPair_T<Int>, Scal, IndexPairHash_T<Int>>;
    
    
    template<typename Key_T, typename Hash_T = Tools::hash<Key_T>>
    std::string ToString( cref<SetContainer<Key_T,Hash_T>> set )
    {
        std::string s ("{ ");
        
        const Size_T n = set.size();
        
        Size_T iter = 0;
        
        for( auto & x : set )
        {
            ++iter;
            s += ToString(x);
            if( iter < n )
            {
                s += ", ";
            }
        }
        
        s += " }";
        
        return s;
    }
    
    
    
#ifdef LTEMPLATE_H
    template<
        typename S,
        typename Hash_T = Tools::hash<S>,
        class = typename std::enable_if_t<mma::HasTypeQ<S>>
    >
    inline mma::TensorRef<mma::Type<S>> to_MTensorRef( cref<SetContainer<S,Hash_T>> source )
    {
        using T = mma::Type<S>;
        auto r = mma::makeVector<T>( int_cast<mint>(source.size()) );
        mptr<T> target = r.data();
        
        mint i = 0;
        
        for( auto & x : source )
        {
            if constexpr ( (SameQ<S,double> || SameQ<S,float>) )
            {
                if( x == Scalar::Infty<S> )
                {
                    target[i] = Scalar::Max<T>;
                }
                else if( x == -Scalar::Infty<S> )
                {
                    target[i] = -Scalar::Max<T>;
                }
                else
                {
                    target[i] = static_cast<T>(x);
                }
            }
            else
            {
                target[i] = static_cast<T>(x);
            }
            ++i;
        }
        
        return r;
    }
#endif // LTEMPLATE_H
    
    
} // namespace Knoodle
