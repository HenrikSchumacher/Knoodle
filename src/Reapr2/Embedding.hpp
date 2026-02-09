public:

LinkEmbedding_T Embedding( cref<PD_T> pd, bool rotate_randomQ = true )
{
    TOOLS_PTIMER(timer,MethodName("Embedding"));
    
    if( pd.CrossingCount() <= Int(0) ) { return LinkEmbedding_T(); }
    
    // TODO: Improve handling of split links!
    if( pd.DiagramComponentCount() > Int(1) )
    {
        eprint(MethodName("Embedding") + ": input PlanarDiagram has " + ToString(pd.DiagramComponentCount()) + " > 1 diagram components. Split it first.");
        return LinkEmbedding_T();
    }
    
    if( settings.permute_randomQ )
    {
        return Embedding_impl(pd.PermuteRandom(random_engine),rotate_randomQ);
    }
    else
    {
        return Embedding_impl(pd,rotate_randomQ);
    }
}


private:

LinkEmbedding_T Embedding_impl( cref<PD_T> pd, bool rotate_randomQ = true )
{
    OrthoDraw_T H;
    Tensor1<Real,Int> L;
    
    ParallelDo(
        [&H,&L,&pd,this](const Int thread)
        {
            if( thread == Int(0) )
            {
                H = OrthoDraw_T( pd, PD_T::Uninitialized, settings.ortho_draw_settings );
            }
            else if( thread == Int(1) )
            {
                L = Levels(pd);
            }
        },
        Int(2),
        (settings.ortho_draw_settings.parallelizeQ ? Int(2) : (Int(1)))
    );

    auto [L_min,L_max] = L.MinMax();
    
    const Real w = static_cast<Real>(H.Width()  * H.HorizontalGridSize());
    const Real h = static_cast<Real>(H.Height() * H.VerticalGridSize()  );
    
    const Real scale = settings.scaling * Min(w,h) / (L_max - L_min);
    
    // TODO: Check whether I have to erase cache of PlanarDiagram pd.
    // TODO: Or maybe better: give OrthoDraw a full copy of PlanarDiagram.
    const auto & lc_arcs = pd.LinkComponentArcs();
    const Int lc_count   = lc_arcs.SublistCount();
    
    RaggedList<Point_T,Int> V_agg ( lc_count, H.VertexCount() + H.ArcCount() );

    const auto & A_V  = H.ArcVertices();
    cptr<Int> A_V_ptr = A_V.Pointers().data();
    cptr<Int> V       = A_V.Elements().data();
    
    cref<Tensor1<Int,Int>> A_next_A = H.ArcNextArc();

    const auto & V_coords = H.VertexCoordinates();

    Tensor1<Int,Int> comp_color (lc_count);

    // cycle over link components
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        {
            const Int a   = *(lc_arcs.Sublist(lc).begin());
            comp_color[a] = pd.ArcColors()[a];
        }
        
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
                V_agg.Push( Point_T{x,y,L_a} );
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
                    V_agg.Push( Point_T{x,y,L_a} );
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
                V_agg.Push( Point_T{x,y,L_a} );
                
                if( L_b != L_a )
                {
                    // In the case of a jump we need a duplicate.
                    V_agg.Push( Point_T{x,y,L_b} );
                }
            }
            
            for( Int k = k_half; k < k_end - Int(1); ++k )
            {
                const Int v = V[k];
                const Real x = static_cast<Real>(V_coords(v,0));
                const Real y = static_cast<Real>(V_coords(v,1));
                V_agg.Push( Point_T{x,y,L_b} ); // Caution: Here we use the level of next arc's tail.
            }
        }
        
        V_agg.FinishSublist();
    }

    auto [comp_ptr,x] = V_agg.Disband();
    
    LinkEmbedding<Real,Int> emb ( std::move(comp_ptr), std::move(comp_color) );
    
    if( rotate_randomQ )
    {
        emb.SetTransformationMatrix( RandomRotation() );
    }
    
    emb.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
    
    return emb;
}
