#pragma once

namespace Knoodle
{
    template<typename T_ = Int64, SignedIntQ I_ = Int64>
    class JonesPolynomial final
    {
    public:
        
        using T       = T_;
        using I       = I_;
        using Class_T = JonesPolynomial;
        using LP_T    = LaurentPolynomial<I,T>;
        
    public:
        
        JonesPolynomial() = default;
        
        ~JonesPolynomial() = default;
        
        
        /*!@brief Return `V(X^4)`, where `V` is the Jones polynomial.
         *
         * This uses the mixture of Kauffman state space sum and diagram simplification compute the Jones polynomial.*/
        
        template<IntQ Int>
        LP_T Jones4(
            const PlanarDiagram<Int> & pd,
            const PlanarDiagramComplex<Int>::Simplify_Args_T & args = typename PlanarDiagramComplex<Int>::Simplify_Args_T()
        )
        {
            return Jones4(PlanarDiagramComplex<Int>(pd),args);
        }
        
        /*!@brief Return `V(X^4)`, where `V` is the Jones polynomial.
         *
         * This uses the mixture of Kauffman state space sum and diagram simplification compute the Jones polynomial.*/
        
        template<IntQ Int>
        LP_T Jones4(
            const PlanarDiagramComplex<Int> & pdc,
            const PlanarDiagramComplex<Int>::Simplify_Args_T & args = typename PlanarDiagramComplex<Int>::Simplify_Args_T()
        )
        {
            using PDC_T = PlanarDiagramComplex<Int>;
            // TODO: Simplify pd first.
            // TODO: Ask for sim
            
            PDC_T pdc_ = pdc;
            
//            logvalprint("Cr before", pd.CrossingCount());
//            pdc_.Simplify(args);
//            logvalprint("Cr after", pdc_.TotalCrossingCount());
//            return Jones4_impl(pdc_.ToSingleDiagram(),args);
            
//            const Int cr_0 = pdc_.TotalCrossingCount();
            
            
            pdc_.Simplify(args);
            pdc_.Connect();
            pdc_.Split();
            
//            const Int cr_1 = pdc_.TotalCrossingCount();
//            logprint("crossing_count ", cr_0, " -> ", cr_1, " (", cr_1 - cr_0 , ")");
//            TOOLS_LOGDUMP(pdc_.DiagramCount());
//
            const Int k = pdc_.DiagramCount() - Int{1};
            LP_T J { k % Int{2} ? T{-1} : T{1} };
            for( Int i = 0; i < k; ++i ) { J = (J << 2) + (J >> 2); }

//            for( auto & pd_i : pdc_.Diagrams() )
//            {
//                if( pd_i.InvalidQ() ) { continue; }
//                J *= Jones4_impl(pd_i,args);
//            }
//
//            return J;
            
            for( auto & pd_i : pdc_.Diagrams() )
            {
                if( pd_i.InvalidQ() ) { continue; }

                PDC_T pdc_i (pd_i);
                pdc_i.Disconnect();
                pdc_i.Split();

                for( auto & pd_ij : pdc_i.Diagrams() )
                {
                    if( pd_ij.InvalidQ() ) { continue; }
                    J *= Jones4_impl(pd_ij,args);
                }
            }
            
            return J;
        }
        
        /*!@brief Return `V(X^2)`, where `V` is the Jones polynomial.
         *
         * This uses the mixture of Kauffman state space sum and diagram simplification compute the Jones polynomial.*/
        
        template<IntQ Int>
        LP_T Jones2(
            const PlanarDiagram<Int> & pd,
            const PlanarDiagramComplex<Int>::Simplify_Args_T & args = typename PlanarDiagramComplex<Int>::Simplify_Args_T()
        )
        {
            auto J_0 = Jones4(pd,args);
            LP_T J;
            for( auto & [e,c] : J_0.Coefficients() ) { J.Set( e/I{2}, c ); }
            return J;
        }
        
        
        /*!@brief Return `V(X^2)`, where `V` is the Jones polynomial.
         *
         * This uses the mixture of Kauffman state space sum and diagram simplification compute the Jones polynomial.*/
        
        template<IntQ Int>
        LP_T Jones2(
            const PlanarDiagramComplex<Int> & pdc,
            const PlanarDiagramComplex<Int>::Simplify_Args_T & args = typename PlanarDiagramComplex<Int>::Simplify_Args_T()
        )
        {
            auto J_0 = Jones4(pdc,args);
            LP_T J;
            for( auto & [e,c] : J_0.Coefficients() ) { J.Set( e/I{2}, c ); }
            return J;
        }
        
    private:
        
        template<IntQ Int>
        LP_T Jones4_impl(
            const PlanarDiagram<Int> & pd,
            const PlanarDiagramComplex<Int>::Simplify_Args_T & args
        )
        {
//            print("start");
//            TOOLS_LOGDUMP(pd.CrossingCount());
            
            (void)args;
            
            if( pd.CrossingCount() <= Int{4} )
            {
                if( pd.InvalidQ() )
                {
                    eprint("Invalid polygon.");
                }
                
                if( pd.CrossingCount() == Int{0} )
                {
//                    logprint("no crossings");
                    LP_T J { T{1} };
                    return J;
                }
                
                if( pd.ProvenUnknotQ() )
                {
//                    logprint("unknot");
                    LP_T J { T{1} };
                    return J;
                }
                
                if( pd.ProvenTrefoilQ() )
                {
//                    logprint("trefoil");
                    
                    LP_T J;
                    const T sign = pd.CrossingRightHandedQ(pd.FirstActiveCrossing()) ? T{1} : T{-1};
                    J.Set( I{  4},  sign );
                    J.Set( I{ 12},  sign );
                    J.Set( I{ 16}, -sign );
                    return J;
                }
                
                if( pd.ProvenHopfLinkQ() )
                {
//                    logprint("hopf link");
                    
                    LP_T J;
                    J.Set( I{  2}, T{-1} );
                    J.Set( I{ 10}, T{-1} );
                    return J;
                }
                
                if( pd.ProvenFigureEightQ() )
                {
//                    logprint("figure-eight");
                    
                    LP_T J;
                    J.Set( I{-8}, T{ 1} );
                    J.Set( I{-4}, T{-1} );
                    J.Set( I{ 1}, T{ 1} );
                    J.Set( I{ 4}, T{-1} );
                    J.Set( I{ 8}, T{ 1} );
                    return J;
                }
            }
            
            // TODO: We need a better heuristic for which crossing to split.
            const Int c = pd.FirstActiveCrossing();
            
            // TODO: Compress or not compress? That is the question.
            auto [p_0,p_1] = pd.KauffmanSplit(Int{0},c,true);

            // TODO: Simplify p_0.pd and p_1.pd.
            
            const Int writhe   = pd.Writhe();
            const Int k_0 = Int{3} * (writhe - p_0.pd.Writhe());
            const Int k_1 = Int{3} * (writhe - p_1.pd.Writhe());

            LP_T J_0 { (p_0.unlink_count + k_0) % Int{2} ? T{-1} : T{1} };
            for( Int i = 0; i < p_0.unlink_count; ++i )
            {
                J_0 = (J_0 << 2) + (J_0 >> 2);
            }
            if( !p_0.pd.ProvenUnknotQ() )
            {
                // TODO: Here we could simplify p_0.pd first.
                J_0 = J_0 * Jones4(p_0.pd,args);
            }
            

            LP_T J_1 { (p_1.unlink_count + k_1) % Int{2} ? T{-1} : T{1} };
            for( Int i = 0; i < p_1.unlink_count; ++i )
            {
                J_1 = (J_1 << 2) + (J_1 >> 2);
            }
            if( !p_1.pd.ProvenUnknotQ() )
            {
                // TODO: Here we could simplify p_1.pd first.
                J_1 = J_1 * Jones4(p_1.pd,args);
            }
            
//            logprint("=======");
//            
//            logprint("u_0",p_0.unlink_count);
//            logprint("k_0",k_0);
//            logprint("J_0: ", p_0.pd.PDCode(), " -> ", ToString(J_0) );
//            
//            logprint("u_1",p_1.unlink_count);
//            logprint("k_1",k_1);
//            logprint("J_1: u_1 -> ", p_1.unlink_count, ", ", p_1.pd.PDCode(), " -> ", ToString(J_1) );
            
            auto J = (J_0 << (k_0 - Int{1}) ) + (J_1 << (k_1 + Int{1}) );
            
//            logprint("J  : ", pd.PDCode(), " -> ", ToString(J) );
//            logprint("=======");
            
            return J;
        }
        
    }; // class JonesPolynomial
    
} // namespace Knoodle
