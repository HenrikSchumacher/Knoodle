#pragma once

namespace KnotTools
{
 
    template<typename Int_>
    class ReAPr
    {
        using Real = Real64;
        using Int  = Int_;
        using LInt = Int64;
        
        using PlanarDiagram_T = PlanarDiagram<Int>;
        using Aggregator_T    = TripleAggregator<Int,Int,Real,LInt>;
        using Matrix_T        = Sparse::MatrixCSR<Real,Int,LInt>;
        
        static constexpr bool Head  = PlanarDiagram_T::Head;
        static constexpr bool Tail  = PlanarDiagram_T::Tail;
        static constexpr bool Left  = PlanarDiagram_T::Left;
        static constexpr bool Right = PlanarDiagram_T::Right;
        static constexpr bool Out   = PlanarDiagram_T::Out;
        static constexpr bool In    = PlanarDiagram_T::In;
        
    private:
        
        Real laplace_reg = 0.01;
        
    public:

        void SetLaplaceRegularization( const Real reg )
        {
            laplace_reg = reg;
        }
        
        Real LaplaceRegularization() const
        {
            return laplace_reg;
        }
        
        cref<Matrix_T> HessianMatrix( mref<PlanarDiagram_T> pd )
        {
            const std::string tag     = ClassName()+"::HessianMatrix";
            const std::string reg_tag = ClassName()+"::LaplaceRegularization";
            
            if(
               (!pd.InCacheQ(tag))
               ||
               (!pd.InCacheQ(reg_tag))
               ||
               (pd.template GetCache<Real>(reg_tag) != laplace_reg)
            )
            {
                pd.ClearCache(tag);
                pd.ClearCache(reg_tag);
                
                const Int m = pd.ArcCount();
                Aggregator_T agg( 5 * m );
                
                const Int comp_count   = pd.LinkComponentCount();
                cptr<Int> comp_arc_ptr = pd.LinkComponentArcPointers().data();
                cptr<Int> comp_arc_idx = pd.LinkComponentArcIndices().data();
                cptr<Int> next_arc     = pd.ArcNextArc().data();
                
                const Real val_0 = 2 + (2 + laplace_reg) * (2 + laplace_reg);
                const Real val_1 = -4 - 2 * laplace_reg;
                const Real val_2 = 1;

                for( Int comp = 0; comp < comp_count; ++comp )
                {
                    const Int k_begin   = comp_arc_ptr[comp    ];
                    const Int k_end     = comp_arc_ptr[comp + 1];
                    
                    Int a   = comp_arc_idx[k_begin];
                    Int ap1 = next_arc[a  ];
                    Int ap2 = next_arc[ap1];
                    
                    for( Int k = k_begin; k < k_end; ++k )
                    {
                        agg.Push(a,a  ,val_0);
                        agg.Push(a,ap1,val_1);
                        agg.Push(a,ap2,val_2);

                        a   = ap1;
                        ap1 = ap2;
                        ap2 = next_arc[ap1];
                    }
                }
                
                agg.Finalize();
                                
                Matrix_T A (agg,m,m,Int(1),true,true);
                
                pd.SetCache(tag,std::move(A));
                pd.SetCache(reg_tag,laplace_reg);
            }
            
            return pd.template GetCache<Matrix_T>(tag);
        }
        
        cref<Matrix_T> ConstraintMatrix( mref<PlanarDiagram_T> pd )
        {
            const std::string tag = ClassName()+"::ConstraintMatrix";
            
            if( !pd.InCacheQ(tag) )
            {
                const Int n = pd.CrossingCount();
                const Int m = pd.ArcCount();
                
                Aggregator_T agg( 2 * n );
                
                cptr<Int>           C_arcs   = pd.Crossings().data();
                cptr<CrossingState> C_states = pd.CrossingStates().data();
                
                for( Int c = 0; c < n; ++c )
                {
                    const Int  a_0 = C_arcs[(c << 2)         ];
                    const Int  a_1 = C_arcs[(c << 2) | Int(1)];
                    const Real s   = static_cast<Real>(ToUnderlying(C_states[c]));
                    
                    agg.Push(c,a_0,-s);
                    agg.Push(c,a_1, s);
                }
                
                agg.Finalize();

                Matrix_T A (agg,n,m,Int(1),true,false);
                
                pd.template SetCache(tag,std::move(A));
            }
            
            return pd.template GetCache<Matrix_T>(tag);
        }
        
//        cref<Sparse::MatrixCSR<Real,Int,LInt>> SystemMatrix(
//            mref<PlanarDiagram_T> pd
//        )
//        {
//            const std::string tag ("SystemMatrixPattern");
//            
//            if( !pd.InCacheQ(tag) )
//            {
//                TripleAggregator<Real,Int,LInt> agg ( 5 * pd.ArcCount() + 5 * CrossingCount() );
//                
//                ComputeHessianMatrix( pd, agg );
//                
//                ComputeConstraintMatrix( pd, agg );
//                
//                const Int size = pd.ArcCount() + CrossingCount();
//                
//                Sparse::MatrixCSR<Int,Int> A ( agg, size, size, Int(1), true, false );
//                
//                pd.SetCache( std::move(L), tag );
//            }
//
//            return pd.GetCache<Sparse::MatrixCSR<Real,Int,Int>>(tag);
//        }
  
        void Optimize_InteriorPointMethod()
        {
        }
//        void Optimize_SemiSmoothNewton()
//        {
//        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("ReAPr") + "<" + TypeName<Int> + ">";
        }
        
        
    }; // class ReAPr
    

} // namespace KnotTools
