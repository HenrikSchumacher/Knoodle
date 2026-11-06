#pragma once

namespace Knoodle
{
    
    /*!@brief **Experimental** Data type for storing a Laurent polynomial.
     *
     * This is implementation uses a hash map to store the map from the exponents to the coefficients.
     *
     * @tparam I_ Type for the exponents.
     *
     * @tparam T_ Type for the coefficients.*/
    
    template<SignedIntQ I_, typename T_>
    class LaurentPolynomial final
    {
    public:
        
        using I       = I_;
        using T       = T_;
        using Class_T = LaurentPolynomial;
        using LP_T    = LaurentPolynomial;
        
    private:
        
        AssociativeContainer<I,T> coeff;
        bool validQ = true;
        
    public:
        
        LaurentPolynomial() = default;

        LaurentPolynomial( const T c )
        {
            coeff[I(0)] = c;
        }
        
        LaurentPolynomial( const I i, const T a )
        {
            coeff[i] = a;
        }
        
        LaurentPolynomial( cptr<I> i, cptr<T> a, const I n )
        {
            for( I k = 0; k < n; ++k )
            {
                coeff[i[k]] += a[k];
            }
            
            Compress();
        }
        
        ~LaurentPolynomial() = default;
        
//        template<IntQ Int>
//        T & operator[]( const Int i )
//        {
//            return coeff[int_cast<I>(i) - shift];
//        }
//        
//        template<IntQ Int>
//        T operator[]( const Int i ) const
//        {
//            return coeff[int_cast<I>(i) - shift];
//        }
        
        AssociativeContainer<I,T> & Coefficients()
        {
            return coeff;
        }
        
        const AssociativeContainer<I,T> & Coefficients() const
        {
            return coeff;
        }
        
        template<IntQ Int>
        T Get( const Int i ) const
        {
            if( coeff.contains(i) )
            {
                return coeff[i];
            }
            else
            {
                return T{0};
            }
        }
        
        template<IntQ Int, typename S>
        LP_T & Set( const Int i, const S a )
        {
            coeff[int_cast<I>(i)] = static_cast<T>(a);
            
            return *this;
        }
        
        template<IntQ Int, typename S>
        LP_T AddTo( const Int i, const S a )
        {
            const I idx = int_cast<I>(i);
            
            if( coeff.contains(idx) )
            {
                coeff[idx] += static_cast<T>(a);
            }
            else
            {
                coeff[idx]  = static_cast<T>(a);
            }
            
            return *this;
        }
        
        bool ValidQ() const
        {
            return validQ;
        }
        
        static LP_T InvalidPolynomial()
        {
            LP_T p;
            p.validQ = false;
            return p;
        }
    
        void Compress()
        {
            if( !ValidQ() ) { return; }
            
            for( const auto & [i,a] : coeff )
            {
                if( a == T{0} ) { coeff.erase(i); }
            }
        }
        
        friend LP_T operator+( const LP_T & p, const LP_T & q )
        {
            if( !p.ValidQ() || !q.ValidQ() ) { return InvalidPolynomial(); }
            
            LP_T r;
            
            for( const auto & [i,a] : p.coeff ) { r.AddTo(i,a); }
            for( const auto & [i,a] : q.coeff ) { r.AddTo(i,a); }
            
            r.Compress();
            
            return r;
        }
        
        friend LP_T operator*( const LP_T & p, const LP_T & q )
        {
            if( !p.ValidQ() || !q.ValidQ() ) { return InvalidPolynomial(); }
            
            LP_T r;
                        
            for( const auto & [i,a] : p.coeff )
            {
                for( const auto & [j,b] : q.coeff )
                {
                    r.AddTo(i + j,a * b);
                }
            }
            
            r.Compress();
            
            return r;
        }
        
        template<IntQ Int>
        friend LP_T operator<<( const LP_T & p, const Int shift )
        {
            if( !p.ValidQ() ) { return InvalidPolynomial(); }
            
            LP_T r;
            
            for( const auto & [i,a] : p.coeff )
            {
                r.Set(i + shift,a);
            }
            
            return r;
        }
        
        template<IntQ Int>
        friend LP_T operator>>( const LP_T & p, const Int shift )
        {
            if( !p.ValidQ() ) { return InvalidPolynomial(); }

            LP_T r;
            
            for( const auto & [i,a] : p.coeff )
            {
                r.Set(i - shift,a);
            }
            
            return r;
        }
        
        
        
        LP_T & operator*= ( const LP_T & p )
        {
            *this = *this * p;
            return *this;
        }
        
        LP_T & operator+= ( const LP_T & p )
        {
            *this = *this + p;
            return *this;
        }
        
        template<IntQ Int>
        LP_T & operator<<= ( const Int shift )
        {
            *this = *this << shift;
            return *this;
        }
        
        template<IntQ Int>
        LP_T & operator>>= ( const Int shift )
        {
            *this = *this >> shift;
            return *this;
        }
        
        I MonomialCount() const
        {
            return int_cast<I>(coeff.size());
        }
        
        friend std::string ToString( const LP_T & p )
        {
            if( !p.ValidQ() ) { return "InvalidPolynomial"; }
            
            return ToString(p.coeff);
        }
        
    public:
        
        using Msgr = Tools::Messenger<Class_T>;
        
        template<typename A>
        static consteval auto MethodName( const A & tag )
        {
            return Msgr::MethodName(tag);
        }
        
        static consteval auto ClassName()
        {
            return ct_string("LaurentPolynomial")
                + "<" + TypeName<I>
                + "," + TypeName<T>
                + ">";
        }
        
    }; // class LaurentPolynomial
    
} // namespace Knoodle
