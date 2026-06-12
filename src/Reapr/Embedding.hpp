public:

LinkEmbedding_T Embedding( cref<PD_T> pd )
{
    return Embedding(pd, [](Point_T && p) { return p; });
}

LinkEmbedding_T Embedding( cref<PD_T> pd, Matrix_T && A )
{
    return Embedding(pd, [&A](Point_T && p) { return Dot(A,p); });
}

template<typename Transformation_T>
LinkEmbedding_T Embedding( cref<PD_T> pd, Transformation_T && f )
{
    TOOLS_PTIMER(timer,MethodName("Embedding"));
    
    if( pd.CrossingCount() <= Int(0) ) { return LinkEmbedding_T(); }
    
    if( pd.DiagramComponentCount() > Int(1) )
    {
        eprint(MethodName("Embedding") + ": input diagram has " + ToString(pd.DiagramComponentCount()) + " > 1 diagram components. Split it first.");
        return LinkEmbedding_T();
    }
    
    if( settings.permute_randomQ )
    {
        return Embedding_impl(pd.CreatePermutedRandom(random_engine),f);
    }
    else
    {
        return Embedding_impl(pd,f);
    }
}


private:

template<typename Transformation_T>
LinkEmbedding_T Embedding_impl( cref<PD_T> pd, Transformation_T && f )
{
    TOOLS_PTIMER(timer,MethodName("Embedding_impl"));
    
    OrthoDraw_T H ( pd, PD_T::Uninitialized, settings.ortho_draw_settings );
    Tensor1<Real,Int> L = Levels(pd);
    
    auto [comp_ptr,comp_color,x] = Embedding_VertexCoordinates(pd, H, L, f);
                         
    LinkEmbedding<Real,Int> emb ( std::move(comp_ptr), std::move(comp_color) );

    emb.template ReadVertexCoordinates<false,true>( &x.data()[0][0] );

    return emb;
}


// Taking the x-y-coordinates from OrthoDraw literally. As z-coordinates we use the scaled levels function. For settings.scaling == 1, we scale the length of the bounding in z-direction roughly matches the smaller of the lengths of the bounding box in x- and y-direction.
// TODO: This typically contains long edges that are bad for Reapr. We should better find a way to subdivide the edges nicely, e.g., to make it so that each x in x-direction has length H.HorizontalGridSize()/2), each edge in y-direction has length H.VerticalGridSize()/2). I am not sure what the idea length in z-direction would be. Maybe the minimum of the two? Also, we have to beware that we have to incoporate the jump somewhere.
template<typename Transformation_T>
std::tuple<Tensor1<Int,Int>,Tensor1<Int,Int>,Tensor1<Point_T,Int>> Embedding_VertexCoordinates(
    cref<PD_T> pd, cref<OrthoDraw_T> H, cref<Tensor1<Real,Int>> L, Transformation_T & f )
{
    auto [L_min,L_max] = L.MinMax();
    
    const Real w = static_cast<Real>(H.Width()  * H.HorizontalGridSize());
    const Real h = static_cast<Real>(H.Height() * H.VerticalGridSize()  );
    
    const Real scale = settings.scaling * Min(w,h) / (L_max - L_min);
    
    const auto & lc_arcs = pd.LinkComponentArcs();
    const Int lc_count   = lc_arcs.SublistCount();
    
    RaggedList<Point_T,Int> V_agg ( lc_count, H.VertexCount() + H.ArcCount() );

    const auto & A_V  = H.ArcVertices();
    cptr<Int> A_V_ptr = A_V.Pointers().data();
    cptr<Int> V       = A_V.Elements().data();
    
    cptr<Int> A_next_A = H.ArcNextArc().data();
    cptr<Int> A_color  = pd.ArcColors().data();
    
    const auto & V_coords = H.VertexCoordinates();

    Tensor1<Int,Int> comp_color (lc_count);
    
    // cycle over link components
    for( Int lc = 0; lc < lc_count; ++lc )
    {
        {
            const Int a   = *(lc_arcs.Sublist(lc).begin());
            comp_color[lc] = A_color[a];
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
                V_agg.Push( f(Point_T{x,y,L_a}) );
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
                    V_agg.Push( f(Point_T{x,y,L_a}) );
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
                V_agg.Push( f(Point_T{x,y,L_a}) );
                
                if( L_b != L_a )
                {
                    // In the case of a jump we need a duplicate.
                    V_agg.Push( f(Point_T{x,y,L_b}) );
                }
            }
            
            for( Int k = k_half; k < k_end - Int(1); ++k )
            {
                const Int v = V[k];
                const Real x = static_cast<Real>(V_coords(v,0));
                const Real y = static_cast<Real>(V_coords(v,1));
                V_agg.Push( f(Point_T{x,y,L_b}) ); // Caution: Here we use the level of next arc's tail.
            }
        }
        
        V_agg.FinishSublist();
    }
    
    auto [comp_ptr,x] = V_agg.Disband();
    
    return {comp_ptr,comp_color,x};
}
