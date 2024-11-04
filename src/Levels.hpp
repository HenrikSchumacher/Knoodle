#pragma once

namespace KnotTools
{
 
    template<typename Int_>
    class Levels
    {
        using Real = Real64;
        using Int  = Int_;
        using LInt = Int64;;
        
        using PlanarDiagram_T = PlanarDiagram<Int>;
        

        void ComputeHessianMatrix
            cref<PlanarDiagram_T> pd, mref<TripleAggregators<Real,Int,LInt>> agg
        )
        {
            const Int n = pd.CrossingCount();
            const Int m = pd.ArcCount();
            
//            const Int comp_count = pd.ComponentCount();
//            
//            cptr<Int> comp_arc_ptr = pd.ComponentArcPointers().data();
//            cptr<Int> comp_arc_idx = pd.ComponentArcIndices().data();
            
            cptr<Int> next_arc = ArcNextArc().data();
            
            for( Int comp = 0; comp < comp_count; ++comp )
            {
                const Int k_begin = comp_arc_ptr[comp    ];
                const Int k_end   = comp_arc_ptr[comp + 1];
                const Int comp_size = k_end - k_begin;
                
                Int a = comp_arc_idx[k];
                Int am1 = pd.NextArc<Tail>(a  );
                Int am2 = pd.NextArc<Tail>(am1);
                Int ap1 = pd.NextArc<Head>(a  );
                Int ap2 = pd.NextArc<Head>(ap1);
                
                for( Int k = k_begin; k < k_end; ++k )
                {
                    agg.push( a0, am2, Real( 1) );
                    agg.push( a0, am1, Real(-4) );
                    agg.push( a0, a0 , Real( 6) );
                    agg.push( a0, ap1, Real(-4) );
                    agg.push( a0, ap2, Real( 1) );
                    
                    am2 = am1;
                    am1 = a0;
                    a0  = ap1;
                    ap1 = ap2;
                    ap2 = next_arc[ap1];
                }
            }
            

//            Int a0  = pd.NextArc<Tail>(  0);
//            Int am1 = pd.NextArc<Tail>( a0);
////            Int am2 = pd.NextArc<Tail>(am1);
//            Int am2 = 0;
//            Int ap1 = 0;
//            Int ap2 = pd.NextArc<Head>(0);
//            
//            for( Int a = 0; a < m; ++a )
//            {
//                am2 = am1;
//                am1 = a0;
//                a0  = ap1;
//                ap1 = ap2;
//                ap2 = next_arc[ap1];
//                
//                agg.push( a0, am2, Real( 1) );
//                agg.push( a0, am1, Real(-4) );
//                agg.push( a0, a0 , Real( 6) );
//                agg.push( a0, ap1, Real(-4) );
//                agg.push( a0, ap2, Real( 1) );
//            }
        }
        
        ComputeConstraintMatrix(
            mref<PlanarDiagram_T> pd, mref<TripleAggregator<Real,Int,LInt>> agg
        )
        {
            const Int n = pd.CrossingCount();
            const Int m = pd.ArcCount();
            
            cptr<Int>           C_arcs   = pd.Crossings().data();
            cptr<CrossingState> C_states = pd.CrossingStates().data();
            
            for( Int c = 0; c < n; ++c )
            {
                const Int k   = m + c;
                const Int a_0 = C_arcs[ (c << 2)          ];
                const Int a_1 = C_arcs[ (c << 2) | Int(1) ];
                const Real s  = static_cast<Real>(ToUnderlying(C_states[c]));
                
                // For the upper-right block.
                agg.push( a_0, k, -s );
                agg.push( a_1, k,  s );
                
                // For the lower-left block.
                agg.push( k, a_0, -s );
                agg.push( k, a_1,  s );
                
                // For the lower-right block.
                agg.push( k, k, Real(0) );
            }
        }
        
        cref<Sparse::MatrixCSR<Real,Int,LInt>> SystemMatrix(
            mref<PlanarDiagram_T> pd
        )
        {
            const std::string tag ("SystemMatrixPattern");
            
            if( !pd.InCacheQ(tag) )
            {
                TripleAggregator<Real,Int,LInt> agg ( 5 * pd.ArcCount() + 5 * CrossingCount() );
                
                ComputeHessianMatrix( pd, agg );
                
                ComputeConstraintMatrix( pd, agg );
                
                const Int size = pd.ArcCount() + CrossingCount();
                
                Sparse::MatrixCSR<Int,Int> A ( agg, size, size, Int(1), true, false );
                
                pd.SetCache( std::move(L), tag );
            }

            return pd.GetCache<Sparse::BinaryMatrixCSR<Real,Int,Int>>(tag);
        }
        
        void SemiSmoothNewton()
        {
        }
    }
}
