public:

void ComputeVertexCoordinates(
    cref<Tensor1<Int,Int>> x, cref<Tensor1<Int,Int>> y
)
{
    TOOLS_PTIC(ClassName()+"::ComputeVertexCoordinates");
    
    if( x.Size() <= Int(0) )
    {
        wprint(ClassName()+"::ComputeVertexCoordinates: Container x is empty. Aborting.");
        return;
    }
    
    if( y.Size() <= Int(0) )
    {
        wprint(ClassName()+"::ComputeVertexCoordinates: Container x is empty. Aborting.");
        return;
    }
    
    if( V_coords.Dimension(0) != vertex_count )
    {
        V_coords = CoordsContainer_T( vertex_count );
    }
    
    auto [x_min,x_max] = x.MinMax();
    auto [y_min,y_max] = y.MinMax();

    width  = x_max - x_min;
    height = y_max - y_min;
    
    for( Int v = 0; v < vertex_count; ++v )
    {
        if( VertexActiveQ(v) )
        {
            V_coords(v,0) = x_grid_size * x[V_Vs[v]];
            V_coords(v,1) = y_grid_size * y[V_Hs[v]];
        }
        else
        {
            V_coords(v,0) = Uninitialized;
            V_coords(v,1) = Uninitialized;
        }
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeVertexCoordinates");
}

void ComputeVertexCoordinates_ByTopologicalOrdering()
{
    ComputeVertexCoordinates(
        Dv.TopologicalOrdering(),
        Dh.TopologicalOrdering()
    );
}

void ComputeVertexCoordinates_ByTopologicalNumbering()
{
    ComputeVertexCoordinates(
        Dv.TopologicalNumbering(),
        Dh.TopologicalNumbering()
    );

}

void ComputeVertexCoordinates_ByTopologicalTightening()
{
    ComputeVertexCoordinates(
        Dv.TopologicalTightening(DvE_costs.data()),
        Dh.TopologicalTightening(DhE_costs.data())
    );
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


void ComputeArcLines()
{
    TOOLS_PTIC(ClassName()+"::ComputeArcLines");
    
    if( A_line_coords.Dimension(0) != A_V_ptr[arc_count] )
    {
        A_line_coords = CoordsContainer_T( A_V_ptr[arc_count] );
    }
    
    for( Int a = 0; a < arc_count; ++a )
    {
        const Int k_begin = A_V_ptr[a  ];
        const Int k_end   = A_V_ptr[a+1];
        
        {
            const Int k = k_begin;
            const Int v_0 = A_V_idx[k  ];
            const Int v_1 = A_V_idx[k+1];
            
            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            
            if( !A_overQ(a,Tail) )
            {
                p_0[0] += x_gap_size * Sign<Int>(p_1[0] - p_0[0]);
                p_0[1] += y_gap_size * Sign<Int>(p_1[1] - p_0[1]);
            }
            
            A_line_coords(k,0) = p_0[0];
            A_line_coords(k,1) = p_0[1];
        }
        
        for( Int k = k_begin + 1; k < k_end - 1; ++k )
        {
            const Int v = A_V_idx[k];
            
            copy_buffer<2>( V_coords.data(v), A_line_coords.data(k) );
        }
        
        {
            const Int k = k_end-1;
            const Int v_0 = A_V_idx[k-1];
            const Int v_1 = A_V_idx[k  ];
            
            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            
            if( !A_overQ(a,Head) )
            {
                p_1[0] -= x_gap_size * Sign<Int>(p_1[0] - p_0[0]);
                p_1[1] -= y_gap_size * Sign<Int>(p_1[1] - p_0[1]);
            }
            
            A_line_coords(k,0) = p_1[0];
            A_line_coords(k,1) = p_1[1];
        }
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeArcLines");
}


void ComputeArcSplines()
{
    TOOLS_PTIC(ClassName()+"::ComputeArcSplines");

    Aggregator<Int,Int> A_spline_ptr_agg ( arc_count + 1);
    A_spline_ptr_agg.Push(Int(0));
    
    PairAggregator<Int,Int,Int> A_spline_coords_agg ( Int(4) * arc_count );
    
    for( Int a = 0; a < arc_count; ++a )
    {
        const Int k_begin = A_E_ptr[a  ];
        const Int k_end   = A_E_ptr[a+1];
        
        {
            const Int k   = k_begin;
            const Int e   = A_E_idx[k];
            const Int v_0 = E_V(e,Tail);
            const Int v_1 = E_V(e,Head);

            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            
            Tiny::Vector<2,Int,Int> u {
                x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                y_gap_size * Sign<Int>(p_1[1] - p_0[1])
            };
            
            if( A_overQ(a,Tail) )
            {
                A_spline_coords_agg.Push( p_0[0]       , p_0[1]        );
            }
            
                A_spline_coords_agg.Push( p_0[0] + u[0], p_0[1] + u[1] );
                A_spline_coords_agg.Push( p_1[0] - u[0], p_1[1] - u[1] );
            
            if( k_end > k_begin + Int(1) )
            {
                // A bit weird, but the splines look better if we duplicate this.
                A_spline_coords_agg.Push( p_1[0] - u[0], p_1[1] - u[1] );
                
                A_spline_coords_agg.Push( p_1[0]       , p_1[1]        );
            }
            else
            {
                if( A_overQ(a,Head) )
                {
                    A_spline_coords_agg.Push( p_1[0]       , p_1[1]        );
                }
            }
        }
        
        for( Int k = k_begin + 1; k < k_end - 1; ++k )
        {
            const Int e   = A_E_idx[k];
            const Int v_0 = E_V(e,Tail);
            const Int v_1 = E_V(e,Head);

            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            
            Tiny::Vector<2,Int,Int> u {
                x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                y_gap_size * Sign<Int>(p_1[1] - p_0[1])
            };
            
                A_spline_coords_agg.Push( p_0[0] + u[0], p_0[1] + u[1] );
                // A bit weird, but the splines look better if we duplicate this.
                A_spline_coords_agg.Push( p_0[0] + u[0], p_0[1] + u[1] );
                // A bit weird, but the splines look better if we duplicate this.
                A_spline_coords_agg.Push( p_1[0] - u[0], p_1[1] - u[1] );
                A_spline_coords_agg.Push( p_1[0] - u[0], p_1[1] - u[1] );
                A_spline_coords_agg.Push( p_1[0]       , p_1[1]        );
        }
        
        if( k_end > k_begin + Int(1) )
        {
            const Int k   = k_end-1;
            const Int e   = A_E_idx[k];
            const Int v_0 = E_V(e,Tail);
            const Int v_1 = E_V(e,Head);

            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            
            Tiny::Vector<2,Int,Int> u {
                x_gap_size * Sign<Int>(p_1[0] - p_0[0]),
                y_gap_size * Sign<Int>(p_1[1] - p_0[1])
            };
            
                A_spline_coords_agg.Push( p_0[0] + u[0], p_0[1] + u[1] );
                // A bit weird, but the splines look better if we duplicate this.
                A_spline_coords_agg.Push( p_0[0] + u[0], p_0[1] + u[1] );
                A_spline_coords_agg.Push( p_1[0] - u[0], p_1[1] - u[1] );
            
            if( A_overQ(a,Head) )
            {
                A_spline_coords_agg.Push( p_1[0]       , p_1[1]        );
            }
        }
        
        A_spline_ptr_agg.Push(A_spline_coords_agg.Size());
    }
    
    A_spline_ptr    = A_spline_ptr_agg.Get();
    A_spline_coords = CoordsContainer_T( A_spline_coords_agg.Size() );
    
    // TODO: This is too much copying. Do something about it.
    auto x = A_spline_coords_agg.Get_0();
    auto y = A_spline_coords_agg.Get_1();
        
    for( Int i = 0; i < A_spline_coords_agg.Size(); ++i )
    {
        A_spline_coords(i,0) = x[i];
        A_spline_coords(i,1) = y[i];
    }
    
    TOOLS_PTOC(ClassName()+"::ComputeArcSplines");
}


std::string DiagramString()
{
    const Int n_x = width  * x_grid_size + 2;
    const Int n_y = height * y_grid_size + 1;
    

    std::string s ( n_x * n_y + 100, ' ');
    for( Int y = 0; y < n_y; ++y )
    {
        s[n_x-1 + n_x * y] = '\n';
    }
    
    auto set = [&s,n_x,n_y]( Int x, Int y, char c )
    {
        s[ x + n_x * (n_y - Int(1) - y) ] = c;
    };
    
//    for( Int v = 0; v < crossing_count; ++v )
//    {
//        if( VertexActiveQ(v) )
//        {
//            const Int x = V_coords(v,0);
//            const Int y = V_coords(v,1);
//            set(x,y,'X');
//        }
//    }
    
    // Draw the corners.
    for( Int v = crossing_count; v < vertex_count; ++v )
    {
        if( VertexActiveQ(v) )
        {
            const Int x = V_coords(v,0);
            const Int y = V_coords(v,1);
            set(x,y,'+');
        }
    }
    
    // Draw the edges.
    for( Int e = 0; e < edge_count; ++e )
    {
        if( EdgeActiveQ(e) )
        {
            const Int v_0 = E_V(e,Tail);
            const Int v_1 = E_V(e,Head);
            
            Tiny::Vector<2,Int,Int> p_0 ( V_coords.data(v_0) );
            Tiny::Vector<2,Int,Int> p_1 ( V_coords.data(v_1) );
            

            switch( E_dir[e] )
            {
                case East:
                {
                    const Int y = p_0[1];
                    
                    for( Int x = p_0[0] + 1; x < p_1[0] - 1; ++x )
                    {
                        set(x,y,'-');
                    }
                    {
                        const Int x = p_1[0]-1;
                        set(x,y,'>');
                    }
                    break;
                }
                case West:
                {
                    const Int y = p_0[1];
                    
                    for( Int x = p_1[0] + 1; x < p_0[0]; ++x )
                    {
                        set(x,y,'-');
                    }
                    {
                        const Int x = p_1[0] + 1;
                        set(x,y,'<');
                    }
                    break;
                }
                case North:
                {
                    const Int x = p_0[0];
                    
                    for( Int y = p_0[1] + 1; y < p_1[1]; ++y )
                    {
                        set(x,y,'|');
                    }
                    {
                        const Int y = p_1[1] - 1;
                        set(x,y,'^');
                    }
                    break;
                }
                    
                case South:
                {
                    const Int x = p_0[0];
                    for( Int y = p_1[1] + 1; y < p_0[1]; ++y )
                    {
                        set(x,y,'|');
                    }
                    {
                        const Int y = p_1[1] + 1;
                        set(x,y,'v');
                    }
                    
                    break;
                }
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
