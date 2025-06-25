public:

Int Height() const
{
    if( !this->InCacheQ("Height") )
    {
        return Uninitialized;
    }
    else
    {
        return this->GetCache<Int>("Height");
    }
}

Int Width() const
{
    if( !this->InCacheQ("Width") )
    {
        return Uninitialized;
    }
    else
    {
        return this->GetCache<Int>("Width");
    }
}

Int Area() const
{
    return Width() * Height();
}


Int Length() const
{
    if( !this->InCacheQ("Length") )
    {
        return Uninitialized;
    }
    else
    {
        return this->GetCache<Int>("Length");
    }
}
    
mref<CoordsContainer_T> VertexCoordinates() const
{
    if( !this->InCacheQ("VertexCoordinates") )
    {
        this->SetCache(
            "VertexCoordinates", CoordsContainer_T ( vertex_count, Uninitialized )
        );
        
        this->ClearCache("Height");
        this->ClearCache("Width");
        this->ClearCache("Length");
        this->ClearCache("ArcLines");
        this->ClearCache("ArcSplines");
    };
    
    return this->GetCache<CoordsContainer_T>("VertexCoordinates");
}


void ComputeVertexCoordinates(
    cref<Tensor1<Int,Int>> x, cref<Tensor1<Int,Int>> y
)  const
{
    TOOLS_PTIMER(timer,ClassName()+"::ComputeVertexCoordinates");
    
    this->ClearCache("ArcLines");
    this->ClearCache("ArcSplines");
    this->ClearCache("Width");
    this->ClearCache("Height");
    this->ClearCache("Length");
    
    if( x.Size() <= Int(0) )
    {
        wprint(ClassName()+"::ComputeVertexCoordinates: Container x is empty. Aborting.");
        
        this->ClearCache("VertexCoordinates");
        return;
    }
    
    if( y.Size() <= Int(0) )
    {
        wprint(ClassName()+"::ComputeVertexCoordinates: Container x is empty. Aborting.");
        
        this->ClearCache("VertexCoordinates");
        return;
    }
    CoordsContainer_T & V_coords = VertexCoordinates();
    
    auto [x_min,x_max] = x.MinMax();
    auto [y_min,y_max] = y.MinMax();
    
    this->template SetCache<false>( "Width",  x_max - x_min );
    this->template SetCache<false>( "Height", y_max - y_min );
    
    Int L = 0;
    {
        auto & DvE_DvV  = Dv().Edges();
        auto & DvE_cost = DvEdgeCosts();
        
        
        for( Int e = 0; e < DvE_DvV.Dim(0); ++e )
        {
            const Int v_0 = DvE_DvV(e,Tail);
            const Int v_1 = DvE_DvV(e,Head);
            
            L += DvE_cost[e] * Abs( x[v_1] - x[v_0] );
        }
    }
    {
        auto & DhE_DhV  = Dh().Edges();
        auto & DhE_cost = DhEdgeCosts();
        
        
        for( Int e = 0; e < DhE_DhV.Dim(0); ++e )
        {
            const Int v_0 = DhE_DhV(e,Tail);
            const Int v_1 = DhE_DhV(e,Head);
            
            L += DhE_cost[e] * Abs( y[v_1] - y[v_0] );
        }
    }
    this->template SetCache<false>( "Length", L );
    
    auto & V_DvV = this->GetCache<Tensor1<Int,Int>>("V_DvV");
    auto & V_DhV = this->GetCache<Tensor1<Int,Int>>("V_DhV");
    
    for( Int v = 0; v < vertex_count; ++v )
    {
        if( VertexActiveQ(v) )
        {
            V_coords(v,0) = settings.x_grid_size * x[V_DvV[v]];
            V_coords(v,1) = settings.y_grid_size * y[V_DhV[v]];
        }
        else
        {
            V_coords(v,0) = Uninitialized;
            V_coords(v,1) = Uninitialized;
        }
    }
}

void ComputeVertexCoordinates_ByTopologicalOrdering() const
{
    ComputeVertexCoordinates(
        Dv().TopologicalOrdering(),
        Dh().TopologicalOrdering()
    );
}

void ComputeVertexCoordinates_ByTopologicalNumbering() const
{
    ComputeVertexCoordinates(
        Dv().TopologicalNumbering(),
        Dh().TopologicalNumbering()
    );
}

void ComputeVertexCoordinates_ByTopologicalTightening() const
{
    ComputeVertexCoordinates(
        Dv().TopologicalTightening(DvEdgeCosts().data()),
        Dh().TopologicalTightening(DhEdgeCosts().data())
    );
}

std::string DiagramString() const
{
    const CoordsContainer_T & V_coords = VertexCoordinates();
    
    const Int n_x = Width()  * settings.x_grid_size + 2;
    const Int n_y = Height() * settings.y_grid_size + 1;
    
    std::string s ( n_x * n_y + 100, ' ');
    for( Int y = 0; y < n_y; ++y )
    {
        s[n_x-1 + n_x * y] = '\n';
    }
    
    auto set = [&s,n_x,n_y]( Int x, Int y, char c )
    {
        s[ x + n_x * (n_y - Int(1) - y) ] = c;
    };
    
    // Draw the corners.
    for( Int v = crossing_count; v < vertex_count; ++v )
    {
        if( !VertexActiveQ(v) ) { continue; }
        
        const Int x = V_coords(v,0);
        const Int y = V_coords(v,1);

        set(x,y,'+');
    }
    
    // Draw the edges.
    for( Int e = 0; e < edge_count; ++e )
    {
        const Int de_0 = ToDedge(e,Tail);
        const Int de_1 = ToDedge(e,Head);
        
        if( !DedgeActiveQ(de_0) || !DedgeActiveQ(de_1) ) { continue; }
        
        
        char v_symbol = '|';
        char h_symbol = '-';
        char l_symbol = '<';
        char r_symbol = '>';
        char u_symbol = '^';
        char d_symbol = 'v';
        
        if( DedgeVirtualQ(de_0) || DedgeVirtualQ(de_1) )
        {
            v_symbol = '.';
            h_symbol = '.';
            l_symbol = '.';
            r_symbol = '.';
            u_symbol = '.';
            d_symbol = '.';
        }
        
        const Int v_0 = E_V.data()[de_0];
        const Int v_1 = E_V.data()[de_1];
        
        Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
        Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
        

        switch( E_dir[e] )
        {
            case East:
            {
                const Int y = p_0[1];
                
                for( Int x = p_0[0] + 1; x < p_1[0] - 1; ++x )
                {
                    set(x,y,h_symbol);
                }
                {
                    const Int x = p_1[0]-1;
                    set(x,y,r_symbol);
                }
                break;
            }
            case West:
            {
                const Int y = p_0[1];
                
                for( Int x = p_1[0] + 1; x < p_0[0]; ++x )
                {
                    set(x,y,h_symbol);
                }
                {
                    const Int x = p_1[0] + 1;
                    set(x,y,l_symbol);
                }
                break;
            }
            case North:
            {
                const Int x = p_0[0];
                
                for( Int y = p_0[1] + 1; y < p_1[1]; ++y )
                {
                    set(x,y,v_symbol);
                }
                {
                    const Int y = p_1[1] - 1;
                    set(x,y,u_symbol);
                }
                break;
            }
                
            case South:
            {
                const Int x = p_0[0];
                for( Int y = p_1[1] + 1; y < p_0[1]; ++y )
                {
                    set(x,y,v_symbol);
                }
                {
                    const Int y = p_1[1] + 1;
                    set(x,y,d_symbol);
                }
                
                break;
            }
        }
    }
    
    // Draw the crossings.
    for( Int a = 0; a < arc_count; ++a )
    {
        if( A_overQ(a,Tail) )
        {
            const Int v = E_V(a,Tail);
            
            const Int x = V_coords(v,0);
            const Int y = V_coords(v,1);
            set(x,y, ((E_dir[a] % 2) == 0) ? '-' : '|');
        }
    }
    
    return s;
}


//void ComputeArcLines()
//{
//    if( A_line_coords.Dimension(0) != A_V_ptr[arc_count] )
//    {
//        A_line_coords = CoordsContainer_T( A_V_ptr[arc_count] );
//    }
//    
//    for( Int a = 0; a < arc_count; ++a )
//    {
//        const Int k_begin = A_V_ptr[a    ];
//        const Int k_end   = A_V_ptr[a + 1];
//        
//        for( Int k = k_begin; k < k_end; ++k )
//        {
//            const Int v = A_V_idx[k];
//            
//            copy_buffer<2>( V_coords.data(v), A_line_coords.data(k) );
//        }
//    }
//}


//mref<CoordsContainer_T> ArcLines() const
//{
//    if( !this->InCacheQ("ArcLines") )
//    {
//        this->SetCache(
//            "ArcLines", CoordsContainer_T ( A_V.ElementCount(), Uninitialized )
//        );
//    };
//    
//    return this->GetCache<CoordsContainer_T>("ArcLines");
//}

cref<ArcSplineContainer_T> ArcLines()
{
    TOOLS_PTIMER(timer,ClassName()+"::ArcLines");

    if( !this->InCacheQ("ArcLines") )
    {
        const CoordsContainer_T & V_coords = VertexCoordinates();
        
//        CoordsContainer_T A_line_coords ( A_V.ElementCount() );
        
        ArcSplineContainer_T A_lines ( arc_count, A_V.ElementCount() );
        
        using A_T = std::array<Int,2>;
        
        for( Int a = 0; a < arc_count; ++a )
        {
            const Int k_begin = A_V.Pointers()[a  ];
            const Int k_end   = A_V.Pointers()[a+1];
            
            {
                const Int k = k_begin;
                const Int v_0 = A_V.Elements()[k  ];
                const Int v_1 = A_V.Elements()[k+1];
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                if( !A_overQ(a,Tail) )
                {
                    p_0[0] += settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]);
                    p_0[1] += settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1]);
                }
                
                A_lines.Push( A_T{p_0[0],p_0[1]});
            }
            
            for( Int k = k_begin + 1; k < k_end - 1; ++k )
            {
                const Int v = A_V.Elements()[k];
                
//                copy_buffer<2>( V_coords.data(v), A_line_coords.data(k) );
                
                A_lines.Push( A_T{V_coords(v,0),V_coords(v,1)} );
            }
            
            {
                const Int k = k_end-1;
                const Int v_0 = A_V.Elements()[k-1];
                const Int v_1 = A_V.Elements()[k  ];
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                if( !A_overQ(a,Head) )
                {
                    p_1[0] -= settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]);
                    p_1[1] -= settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1]);
                }
                
//                A_line_coords(k,0) = p_1[0];
//                A_line_coords(k,1) = p_1[1];
                
                A_lines.Push( A_T{p_1[0],p_1[1]} );
            }
            
            A_lines.FinishSublist();
        }
        
        this->SetCache( "ArcLines", std::move(A_lines) );
    }
    
    return this->GetCache<ArcSplineContainer_T>("ArcLines");
}



//mref<ArcSplineContainer_T> ArcSplines() const
//{
//    if( !this->InCacheQ("ArcSplines") )
//    {
//        return ArcSplineContainer_T();
//    }
//    else
//    {
//        return this->GetCache<ArcSplineContainer_T>("ArcSplines");
//    }
//}

cref<ArcSplineContainer_T> ArcSplines()
{
    TOOLS_PTIMER(timer,ClassName()+"::ArcSplines");

    if( !this->InCacheQ("ArcSplines") )
    {
        const CoordsContainer_T & V_coords = VertexCoordinates();
        
        ArcSplineContainer_T A_splines ( arc_count, Int(5) * E_V.Dim(0) );
        
        using A_T = std::array<Int,2>;
        
        for( Int a = 0; a < arc_count; ++a )
        {
            const Int k_begin = A_E.Pointers()[a  ];
            const Int k_end   = A_E.Pointers()[a+1];
            
            {
                const Int k   = k_begin;
                const Int e   = A_E.Elements()[k];
                const Int v_0 = E_V(e,Tail);
                const Int v_1 = E_V(e,Head);
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                Tiny::Vector<2,Int,Int> u {
                    settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                    settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1])
                };
                
                if( A_overQ(a,Tail) )
                {
                    A_splines.Push( A_T{p_0[0], p_0[1]}        );
                }
                
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                
                if( k_end > k_begin + Int(1) )
                {
                    // A bit weird, but the splines look better if we duplicate this.
                    A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                    A_splines.Push( A_T{p_1[0]       , p_1[1]       } );
                }
                else
                {
                    if( A_overQ(a,Head) )
                    {
                        A_splines.Push( A_T{p_1[0]       , p_1[1]      } );
                    }
                }
            }
            
            for( Int k = k_begin + 1; k < k_end - 1; ++k )
            {
                const Int e   = A_E.Elements()[k];
                const Int v_0 = E_V(e,Tail);
                const Int v_1 = E_V(e,Head);
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                Tiny::Vector<2,Int,Int> u {
                    settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                    settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1])
                };
                
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                A_splines.Push( A_T{p_1[0]       , p_1[1]       } );
            }
            
            if( k_end > k_begin + Int(1) )
            {
                const Int k   = k_end-1;
                const Int e   = A_E.Elements()[k];
                const Int v_0 = E_V(e,Tail);
                const Int v_1 = E_V(e,Head);
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                Tiny::Vector<2,Int,Int> u {
                    settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                    settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1])
                };
                
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                
                if( A_overQ(a,Head) )
                {
                    A_splines.Push( A_T{p_1[0]       , p_1[1]       } );
                }
            }
            
            A_splines.FinishSublist();
        }

        this->SetCache( "ArcSplines", std::move(A_splines) );
    }
    
    return this->GetCache<ArcSplineContainer_T>("ArcSplines");
}


