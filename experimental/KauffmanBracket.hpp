#pragma once

namespace Knoodle
{
    template<SignedIntQ I_ = Int64, typename T_ = Int64>
    class KauffmanBracket final
    {
    public:
        
        using T       = T_;
        using I       = I_;
        using Class_T = KauffmanBracket;
        using LP_T    = LaurentPolynomial<I,T>;
        
    private:
        
        bool compressQ = true;
        
    public:
        
        KauffmanBracket() = default;
        
        KauffmanBracket( bool compressQ_ )
        : compressQ { compressQ_ }
        {}
        
        ~KauffmanBracket() = default;
        
        template<IntQ Int>
        LP_T Bracket( const PlanarDiagram<Int> & pd )
        {
            return KauffmanBracket_impl(pd);
        }
        
        template<IntQ Int>
        LP_T operator()( const PlanarDiagram<Int> & pd )
        {
            return Bracket(pd);
        }
        
    private:
        
        template<IntQ Int>
        LP_T UnlinkProduct( const Int n, const Int shift = 0 )
        {
            LP_T p { I{shift}, n % I{2} ? T{-1} : T{1} };

            for( Int k = 0; k < n; ++k )
            {
                LP_T r;
                for( auto & [i,a] : p.Coefficients() )
                {
                    r.AddTo(i-I{2},a);
                    r.AddTo(i+I{2},a);
                }
                std::swap(p,r);
            }

            return p;
        }
        
        template<IntQ Int>
        LP_T KauffmanBracket_impl( const PlanarDiagram<Int> & pd )
        {
//            print("start");
//            TOOLS_DUMP(pd.CrossingCount());
            
            if( pd.CrossingCount() == Int{1} )
            {
                const Int c = pd.FirstActiveCrossing();
                
                if( pd.CrossingRightHandedQ(c) )
                {
                    return LP_T { I{ 3}, T{-1} };
                }
                else
                {
                    return LP_T { I{-3}, T{-1} };
                }
            }
            
            if( pd.CrossingCount() == Int{0} ) { return LP_T { T{1} }; }

            // TODO: We need a better heuristic for which crossing to split.
            const Int c = pd.FirstActiveCrossing();
            
//            TOOLS_DUMP(c);
            
            auto [p_0,p_1] = pd.KauffmanSplit(Int{0},c,compressQ);
            
            if( !p_0.pd.ValidQ() ) { eprint("p_0 not valid."); };
            if( !p_1.pd.ValidQ() ) { eprint("p_1 not valid."); };
            
//            TOOLS_DUMP(p_0.unlink_count);
//            TOOLS_DUMP(p_1.unlink_count);
            
            LP_T B_0 = UnlinkProduct(p_0.unlink_count,Int{ 1});
//            TOOLS_DUMP(B_0);
            
            if( p_0.pd.CrossingCount() > Int{0} )
            {
                B_0 = B_0 * KauffmanBracket_impl(p_0.pd);
            }
            
            LP_T B_1 = UnlinkProduct(p_1.unlink_count,Int{-1});
//            TOOLS_DUMP(B_1);
            
            if( p_1.pd.CrossingCount() > Int{0} )
            {
                B_1 = B_1 * KauffmanBracket_impl(p_1.pd);
            }
            
//            TOOLS_DUMP(p_0.pd.PDCode());
//            TOOLS_DUMP(B_0);
//            TOOLS_DUMP(p_1.pd.PDCode());
//            TOOLS_DUMP(B_1);
            
            return B_0 + B_1;
        }
        
    public:
        
        template<IntQ Int>
        LP_T Jones4( const PlanarDiagram<Int> & pd )
        {
            const I shift = I{3} * int_cast<I>(pd.Writhe());
            LP_T bracket = Bracket(pd);
            LP_T r;
            T sign = shift % I{2} ? T{-1} : T{1};
            
            for( auto & [i,a] : bracket.Coefficients() )
            {
                r.Set(-i + shift, sign * a);
            }
            
            return r;
        }
        
        template<IntQ Int>
        LP_T Jones2( const PlanarDiagram<Int> & pd )
        {
            const I shift = I{3} * int_cast<I>(pd.Writhe());
            LP_T bracket = Bracket(pd);
            LP_T r;
            T sign = shift % I{2} ? T{-1} : T{1};
            
            for( auto & [i,a] : bracket.Coefficients() )
            {
                r.Set((-i + shift)/I{2}, sign * a);
            }
            
            return r;
        }
        
//        template<IntQ Int>
//        LP_T KauffmanBracket_impl(
//            const PlanarDiagram<Int>::PlanarDiagram_with_Unlinks & p,
//            const Int c,
//            const Int n
//        )
//        {
//            if( c == n )
//            {
//                LP_T b { T{ p.unlink_count % Int{2} ? Int{-1} : Int{1} } };
//                
//                for( Int i = 1; i < p.unlink_count; ++i )
//                {
//                    b = (b << 2) + (b << -2);
//                }
//                return b;
//            }
//            
//            auto [p_0,p_1] = KauffmanSplit(p,c);
//            
//            return   (KauffmanBracket_impl(p_0,c+Int{1},n) << 1)
//                   + (KauffmanBracket_impl(p_1,c+Int{1},n) >> 1);
//        }
        
    }; // class KauffmanBracket
    
} // namespace Knoodle
