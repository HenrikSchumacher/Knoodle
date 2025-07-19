public:

template<
    typename Int, typename V_T = std::array<Real,3>
>
RaggedList<V_T,Int> Embedding( mref<PlanarDiagram<Int>> pd )
{
    TOOLS_PTIMER(timer,MethodName("Embedding"));
    
    if( pd.CrossingCount() <= Int(0) ) { return RaggedList<V_T,Int>(); }
    
    // TODO: Improve handling of split links!
    if( pd.DiagramComponentCount() > Int(1) )
    {
        eprint(MethodName("Embedding") + ": input PlanarDiagram has " + ToString(pd.DiagramComponentCount()) + " > 1 diagram components. Split it first.");
        return RaggedList<V_T,Int>();
    }
    
//    TOOLS_PDUMP( pd.PDCode() );
    OrthoDrawing<Int> H (pd);

    Tensor1<Real,Int> L = Levels(pd);

    auto [L_min,L_max] = L.MinMax();
    
    const Real w = static_cast<Real>(H.Width()  * H.HorizontalGridSize());
    const Real h = static_cast<Real>(H.Height() * H.VerticalGridSize()  );
    
    const Real scale = scaling * Min(w,h) / (L_max - L_min);
    
    // TODO: Check whether I have to erase cache of PlanarDiagram pd.
    // TODO: Or maybe better: give OrthoDrawing a full copy of PlanarDiagram.
    const auto & lc_arcs = pd.LinkComponentArcs();
    const Int lc_count   = lc_arcs.SublistCount();
    
    RaggedList<V_T,Int> V_agg ( lc_count, H.VertexCount() + H.ArcCount() );

    const auto & A_V  = H.ArcVertices();
    cptr<Int> A_V_ptr = A_V.Pointers().data();
    cptr<Int> V       = A_V.Elements().data();
    
    cref<Tensor1<Int,Int>> A_next_A = H.ArcNextArc();

    const auto & V_coords = H.VertexCoordinates();

    // cycle over link components
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        for( Int a : lc_arcs.Sublist(lc) )
        {
            const Int b = A_next_A[a];
            const Real L_a = scale * L[a];
            const Real L_b = scale * L[b];
            
            const Int k_begin = A_V_ptr[a         ];
            const Int k_end   = A_V_ptr[a + Int(1)];
            
            const Int n       = k_end - k_begin;
            const Int k_half  = k_begin + n/2;

//                   a_begin   a_end
//                 X-------->X-------->+
//       V[k_begin]      V[k_half]      V[k_end]
            
//                     a_begin                       a_end
//                 X-------->+-------->+-------->X-------->+
//       V[k_begin]               V[k_half]                 V[k_end]
            
            
            for( Int k = k_begin; k < k_half; ++k )
            {
                const Int v = V[k];
                const Real x = static_cast<Real>(V_coords(v,0));
                const Real y = static_cast<Real>(V_coords(v,1));
                V_agg.Push( V_T{x,y,L_a} );
            }
            
            if( n % Int(2) )
            {
                // Odd number of vertices.
                if( L_b != L_a )
                {
                    // In the case of a jump we need a duplicate.
                    const Int v = V[k_half];
                    const Real x = static_cast<Real>(V_coords(v,0));
                    const Real y = static_cast<Real>(V_coords(v,1));
                    V_agg.Push( V_T{x,y,L_a} );
                }
            }
            else
            {
                // Even number of vertices.
                // Thus, we need to insert a point in the middle.
                const Int v_0 = V[k_half-1];
                const Int v_1 = V[k_half  ];
                
                const Real x = Scalar::Half<Real> * static_cast<Real>(V_coords(v_0,0) + V_coords(v_1,0));
                const Real y = Scalar::Half<Real> * static_cast<Real>(V_coords(v_0,1) + V_coords(v_1,1));
                V_agg.Push( V_T{x,y,L_a} );
                
                if( L_b != L_a )
                {
                    // In the case of a jump we need a duplicate.
                    V_agg.Push( V_T{x,y,L_b} );
                }
            }
            
            for( Int k = k_half; k < k_end - Int(1); ++k )
            {
                const Int v = V[k];
                const Real x = static_cast<Real>(V_coords(v,0));
                const Real y = static_cast<Real>(V_coords(v,1));
                V_agg.Push( V_T{x,y,L_b} ); // Caution: Here we use the level of next arc's tail.
            }
        }
        
        V_agg.FinishSublist();
    }
    
    return V_agg;
}
