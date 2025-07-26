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
        const Int V_count = V_dE.Dim(0);
        
        this->SetCache(
            "VertexCoordinates", CoordsContainer_T ( V_count, Uninitialized )
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
    mref<Tensor1<Int,Int>> x, mref<Tensor1<Int,Int>> y
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
    
    {
        auto [x_min,x_max] = x.MinMax();
        auto [y_min,y_max] = y.MinMax();
        
        Int w = static_cast<Int>(x_max - x_min);
        Int h = static_cast<Int>(y_max - y_min);
        
        this->template SetCache<false>("Width" ,w);
        this->template SetCache<false>("Height",h);
    }

    Int L = 0;
    {
        auto & DvE_DvV  = Dv().Edges();
        auto & DvE_cost = DvEdgeCosts();
        
        const Int DvE_count = DvE_DvV.Dim(0);
        
        for( Int e = 0; e < DvE_count; ++e )
        {
            const Int v_0 = DvE_DvV(e,Tail);
            const Int v_1 = DvE_DvV(e,Head);
            
            L += DvE_cost[e] * Abs( x[v_1] - x[v_0] );
        }
    }
    {
        auto & DhE_DhV  = Dh().Edges();
        auto & DhE_cost = DhEdgeCosts();
        
        const Int DhE_count = DhE_DhV.Dim(0);
        
        for( Int e = 0; e < DhE_count; ++e )
        {
            const Int v_0 = DhE_DhV(e,Tail);
            const Int v_1 = DhE_DhV(e,Head);
            
            L += DhE_cost[e] * Abs( y[v_1] - y[v_0] );
        }
    }
    this->template SetCache<false>("Length",L);
    
    auto & V_DvV = this->GetCache<Tensor1<Int,Int>>("V_DvV");
    auto & V_DhV = this->GetCache<Tensor1<Int,Int>>("V_DhV");
    
    const Int V_count = V_dE.Dim(0);
    
    for( Int v = 0; v < V_count; ++v )
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

void ComputeVertexCoordinates_ByTopologicalOrdering()
{
    auto x = Dv().TopologicalOrdering();
    
    if( x.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalOrdering") + ": Graph Dv() is cyclic.");
    }
    
    auto y = Dh().TopologicalOrdering();
    
    if( y.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalOrdering") + ": Graph Dh() is cyclic.");
    }

    ComputeVertexCoordinates(x,y);
}

void ComputeVertexCoordinates_ByTopologicalNumbering()
{
    auto x = Dv().TopologicalNumbering();
    
    if( x.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalNumbering") + ": Graph Dv() is cyclic.");
    }
    
    auto y = Dh().TopologicalNumbering();
    
    if( y.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalNumbering") + ": Graph Dh() is cyclic.");
    }

    ComputeVertexCoordinates(x,y);
}

void ComputeVertexCoordinates_ByTopologicalTightening()
{
    auto x = Dv().TopologicalTightening(DvEdgeCosts().data());
    
    if( x.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalTightening") + ": Graph Dv() is cyclic.");
    }
    
    auto y = Dh().TopologicalTightening(DvEdgeCosts().data());
    
    if( y.Size() <= Int(0) )
    {
        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalTightening") + ": Graph Dh() is cyclic.");
    }
    
    ComputeVertexCoordinates(x,y);
}

std::string DiagramString() const
{
    const CoordsContainer_T & V_coords = VertexCoordinates();
    
    const Int n_x = Width()  * settings.x_grid_size + 2;
    const Int n_y = Height() * settings.y_grid_size + 1;
    
    std::string s ( ToSize_T(n_x * n_y + 100), ' ');
    
    for( Int y = 0; y < n_y; ++y )
    {
        s[ToSize_T(n_x - Int(1) + n_x * y)] = '\n';
    }
    
    auto set = [&s,n_x,n_y]( Int x, Int y, char c )
    {
        s[ToSize_T(x + n_x * (n_y - Int(1) - y))] = c;
    };
    
    const Int C_count = C_A.Dim(0);
    const Int V_count = V_dE.Dim(0);
    
    // Draw the corners.
    for( Int v = C_count; v < V_count; ++v )
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
        
        if( !DedgeActiveQ(de_0) ) { continue; }
        
        char v_symbol = '|';
        char h_symbol = '-';
        char l_symbol = '<';
        char r_symbol = '>';
        char u_symbol = '^';
        char d_symbol = 'v';
        
        if( DedgeVirtualQ(de_0) )
        {
            v_symbol = '.';
            h_symbol = '.';
            l_symbol = '.';
            r_symbol = '.';
            u_symbol = '.';
            d_symbol = '.';
        }
        
        const Int v_0 = E_V(e,0);
        const Int v_1 = E_V(e,1);
        
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
    
    const Int A_count = A_C.Dim(0);
    
    for( Int a = 0; a < A_count; ++a )
    {
        if( !EdgeActiveQ(a) ) { continue; }
           
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

cref<ArcSplineContainer_T> ArcLines()
{
    TOOLS_PTIMER(timer,ClassName()+"::ArcLines");

    if( !this->InCacheQ("ArcLines") )
    {
        const CoordsContainer_T & V_coords = VertexCoordinates();
        
        const Int v_count = V_dE.Dim(0);
        const Int a_count = A_C.Dim(0);
        
//        TOOLS_DUMP(v_count);
//        TOOLS_DUMP(V_coords.Dim(0));
        
        ArcSplineContainer_T A_lines ( a_count, A_V.ElementCount() );
        
        using A_T = std::array<Int,2>;
        
//        TOOLS_LOGDUMP(A_V.Pointers());
//        TOOLS_LOGDUMP(A_V.Elements());
        
        for( Int a = 0; a < a_count; ++a )
        {
            if( !DedgeActiveQ(ToDarc<Tail>(a)) )
            {
                A_lines.FinishSublist();
                continue;
            }
            
            const Int k_begin = A_V.Pointers()[a  ];
            const Int k_end   = A_V.Pointers()[a+1];
            
            {
                const Int k = k_begin;
                const Int v_0 = A_V.Elements()[k  ];
                const Int v_1 = A_V.Elements()[k+1];
                                
                if( !InIntervalQ(v_0,Int(0),v_count) )
                {
                    eprint("v_0 = " + ToString(v_0) + " out of bounds.");
                }
                if( !InIntervalQ(v_1,Int(0),v_count) )
                {
                    eprint("v_1 = " + ToString(v_1) + " out of bounds.");
                }
                
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
                
                A_lines.Push( A_T{V_coords(v,0),V_coords(v,1)} );
            }
            
            {
                const Int k = k_end-1;
                const Int v_0 = A_V.Elements()[k-1];
                const Int v_1 = A_V.Elements()[k  ];
                
                if( !InIntervalQ(v_0,Int(0),v_count) )
                {
                    eprint("v_0 = " + ToString(v_0) + " out of bounds.");
                }
                if( !InIntervalQ(v_1,Int(0),v_count) )
                {
                    eprint("v_1 = " + ToString(v_1) + " out of bounds.");
                }
                
                Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
                Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
                
                if( !A_overQ(a,Head) )
                {
                    p_1[0] -= settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]);
                    p_1[1] -= settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1]);
                }
                
                A_lines.Push( A_T{p_1[0],p_1[1]} );
            }
            
            A_lines.FinishSublist();
        }
        
        TOOLS_LOGDUMP(A_lines.Pointers());
//        TOOLS_LOGDUMP(A_lines.Elements());
        
        
        for( Int i = 0; i < A_lines.ElementCount(); ++i )
        {
            logvalprint(
                "A_lines.Elements()["+ToString(i)+"]",
                A_lines.Elements()[i]
            );
        }
        
        this->SetCache( "ArcLines", std::move(A_lines) );
    }
    
    return this->GetCache<ArcSplineContainer_T>("ArcLines");
}

cref<ArcSplineContainer_T> ArcSplines()
{
    TOOLS_PTIMER(timer,ClassName()+"::ArcSplines");

    if( !this->InCacheQ("ArcSplines") )
    {
        const CoordsContainer_T & V_coords = VertexCoordinates();
        
        const Int a_count = A_C.Dim(0);
        
        ArcSplineContainer_T A_splines ( a_count, Int(5) * E_V.Dim(0) );
        
        using A_T = std::array<Int,2>;

        const Int x_gap = Min(settings.x_gap_size,settings.x_grid_size/2);
        const Int y_gap = Min(settings.y_gap_size,settings.y_grid_size/2);
        
//        Int x_round = Min(settings.x_rounding_radius,settings.x_grid_size/2);
//        Int y_round = Min(settings.y_rounding_radius,settings.y_grid_size/2);

        const Int x_round = settings.x_rounding_radius;
        const Int y_round = settings.y_rounding_radius;

        for( Int a = 0; a < a_count; ++a )
        {
            if( !DedgeActiveQ(ToDarc<Tail>(a)) )
            {
                A_splines.FinishSublist();
                continue;
            }
            
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
                    x_gap * Sign<Int>(p_1[0] - p_0[0]),
                    y_gap * Sign<Int>(p_1[1] - p_0[1])
                };
                
                // Point on tail crossing.
                if( A_overQ(a,Tail) )
                {
                    A_splines.Push( A_T{p_0[0], p_0[1]}        );
                }
                // Point slightly offset from tail crossing.
                A_splines.Push( A_T{p_0[0] + u[0], p_0[1] + u[1]} );
                
                if( k_end > k_begin + Int(1) )
                {
                    Tiny::Vector<2,Int,Int> w {
                        x_round * Sign<Int>(p_1[0] - p_0[0]),
                        y_round * Sign<Int>(p_1[1] - p_0[1])
                    };
                    
                    // A bit weird, but the splines look better if we duplicate this.
                    A_splines.Push( A_T{p_1[0] - w[0], p_1[1] - w[1]} );
                    A_splines.Push( A_T{p_1[0] - w[0], p_1[1] - w[1]} );
                    
                    // Corner point
                    A_splines.Push( A_T{p_1[0]       , p_1[1]       } );
                }
                else
                {
                    // Point slightly offset from head crossing.
                    A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                    
                    // Point on head crossing.
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
                
//                Tiny::Vector<2,Int,Int> u {
//                    settings.x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
//                    settings.y_gap_size * Sign<Int>(p_1[1] - p_0[1])
//                };
                
                Tiny::Vector<2,Int,Int> w {
                    x_round * Sign<Int>(p_1[0] - p_0[0]),
                    y_round * Sign<Int>(p_1[1] - p_0[1])
                };
                
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_0[0] + w[0], p_0[1] + w[1]} );
                A_splines.Push( A_T{p_0[0] + w[0], p_0[1] + w[1]} );
                
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_1[0] - w[0], p_1[1] - w[1]} );
                A_splines.Push( A_T{p_1[0] - w[0], p_1[1] - w[1]} );
                
                // Corner point
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
                    x_gap * Sign<Int>(p_1[0] - p_0[0]),
                    y_gap * Sign<Int>(p_1[1] - p_0[1])
                };
                
                Tiny::Vector<2,Int,Int> w {
                    x_round * Sign<Int>(p_1[0] - p_0[0]),
                    y_round * Sign<Int>(p_1[1] - p_0[1])
                };
                
                // A bit weird, but the splines look better if we duplicate this.
                A_splines.Push( A_T{p_0[0] + w[0], p_0[1] + w[1]} );
                A_splines.Push( A_T{p_0[0] + w[0], p_0[1] + w[1]} );
                
                // Point slightly offset from head crossing.
                A_splines.Push( A_T{p_1[0] - u[0], p_1[1] - u[1]} );
                
                // Point on head crossing.
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


