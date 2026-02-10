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
            "VertexCoordinates", CoordsContainer_T ( V_dE.Dim(0), Uninitialized )
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
    
    for( Int v = 0; v < V_end; ++v )
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

// Does not prevent self-intersections.
//void ComputeVertexCoordinates_ByTopologicalTightening()
//{
//    auto x = Dv().TopologicalTightening(DvEdgeCosts().data());
//    
//    if( x.Size() <= Int(0) )
//    {
//        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalTightening") + ": Graph Dv() is cyclic.");
//    }
//    
//    auto y = Dh().TopologicalTightening(DvEdgeCosts().data());
//    
//    if( y.Size() <= Int(0) )
//    {
//        eprint(MethodName("ComputeVertexCoordinates_ByTopologicalTightening") + ": Graph Dh() is cyclic.");
//    }
//    
//    ComputeVertexCoordinates(x,y);
//}

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

    // ANSI color highlighting support.
    static constexpr const char* kHighlightColors[] = {
        "\x1b[91m",  // bright red       (group 0)
        "\x1b[92m",  // bright green     (group 1)
        "\x1b[94m",  // bright blue      (group 2)
        "\x1b[93m",  // bright yellow    (group 3)
        "\x1b[95m",  // bright magenta   (group 4)
        "\x1b[96m",  // bright cyan      (group 5)
    };
    static constexpr int kHighlightColorCount = 6;
    static constexpr const char* kColorReset = "\x1b[0m";

    const bool has_highlights = !settings.highlight_arc_groups.empty();

    // arc_group[a] = color group index, or -1 if not highlighted.
    std::vector<int> arc_group;
    if( has_highlights )
    {
        arc_group.assign(ToSize_T(A_C.Dim(0)), -1);
        for( int g = 0; g < static_cast<int>(settings.highlight_arc_groups.size()); ++g )
        {
            for( Int a : settings.highlight_arc_groups[g] )
            {
                if( a >= Int(0) && a < A_C.Dim(0) )
                {
                    arc_group[ToSize_T(a)] = g;
                }
            }
        }
    }

    // Parallel color map: same size as `s`, stores group index per cell (-1 = no color).
    std::vector<int> color_map;
    if( has_highlights )
    {
        color_map.assign(ToSize_T(n_x * n_y + 100), -1);
    }

    auto set_color = [&color_map,n_x,n_y]( Int x, Int y, int group )
    {
        color_map[ToSize_T(x + n_x * (n_y - Int(1) - y))] = group;
    };

    const Int C_end = C_A.Dim(0);
    
    // Draw the corners.
    for( Int v = C_end; v < V_end; ++v )
    {
        if( !VertexActiveQ(v) ) { continue; }
        
        const Int x = V_coords(v,0);
        const Int y = V_coords(v,1);

        set(x,y,'+');
    }
    
    // Draw the edges.
    for( Int e = 0; e < E_end; ++e )
    {
        const Int de_0 = ToDedge(e,Tail);

        if( !DedgeActiveQ(e) ) { continue; }

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

        // Determine highlight group for this edge's arc.
        int edge_group = -1;
        if( has_highlights && !DedgeVirtualQ(de_0) && e < E_A.Size() && E_A[e] < A_C.Dim(0) )
        {
            edge_group = arc_group[ToSize_T(E_A[e])];
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
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                {
                    const Int x = p_1[0]-1;
                    set(x,y,r_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                break;
            }
            case West:
            {
                const Int y = p_0[1];

                for( Int x = p_1[0] + 1; x < p_0[0]; ++x )
                {
                    set(x,y,h_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                {
                    const Int x = p_1[0] + 1;
                    set(x,y,l_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                break;
            }
            case North:
            {
                const Int x = p_0[0];

                for( Int y = p_0[1] + 1; y < p_1[1]; ++y )
                {
                    set(x,y,v_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                {
                    const Int y = p_1[1] - 1;
                    set(x,y,u_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                break;
            }

            case South:
            {
                const Int x = p_0[0];
                for( Int y = p_1[1] + 1; y < p_0[1]; ++y )
                {
                    set(x,y,v_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
                }
                {
                    const Int y = p_1[1] + 1;
                    set(x,y,d_symbol);
                    if( edge_group >= 0 ) set_color(x,y,edge_group);
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

            if( has_highlights )
            {
                int g = arc_group[ToSize_T(a)];
                if( g >= 0 ) set_color(x,y,g);
            }
        }
    }

    // Helper to place a multi-character string at (x, y) with bounds checking.
    auto set_string = [&s,n_x,n_y]( Int x, Int y, const std::string & label )
    {
        for( std::size_t i = 0; i < label.size(); ++i )
        {
            const Int xi = x + static_cast<Int>(i);
            if( xi >= Int(0) && xi < n_x - Int(1) && y >= Int(0) && y < n_y )
            {
                s[ToSize_T(xi + n_x * (n_y - Int(1) - y))] = label[i];
            }
        }
    };

    // Helper to color a multi-character string at (x, y).
    auto set_string_color = [&color_map,n_x,n_y,has_highlights]( Int x, Int y, std::size_t len, int group )
    {
        if( !has_highlights || group < 0 ) return;
        for( std::size_t i = 0; i < len; ++i )
        {
            const Int xi = x + static_cast<Int>(i);
            if( xi >= Int(0) && xi < n_x - Int(1) && y >= Int(0) && y < n_y )
            {
                color_map[ToSize_T(xi + n_x * (n_y - Int(1) - y))] = group;
            }
        }
    };

    // Pass 4: Crossing labels.
    if( settings.label_crossingsQ )
    {
        for( Int c = 0; c < C_end; ++c )
        {
            if( !VertexActiveQ(c) ) { continue; }

            const Int x = V_coords(c,0);
            const Int y = V_coords(c,1);

            // Place label diagonally below-right (on screen: x+1, y-1).
            // This position is always free because edges go in cardinal directions only.
            set_string( x + Int(1), y - Int(1), std::to_string(c) );
        }
    }

    // Pass 5: Arc labels.
    if( settings.label_arcsQ || settings.label_levelsQ )
    {
        for( Int a = 0; a < A_count; ++a )
        {
            if( !EdgeActiveQ(a) ) { continue; }

            const Int k_begin = A_E.Pointers()[a  ];
            const Int k_end   = A_E.Pointers()[a+1];

            if( k_begin >= k_end ) { continue; }

            // Find the longest edge in this arc.
            Int best_edge  = A_E.Elements()[k_begin];
            Int best_len   = Int(0);

            for( Int k = k_begin; k < k_end; ++k )
            {
                const Int e  = A_E.Elements()[k];
                const Int v0 = E_V(e,0);
                const Int v1 = E_V(e,1);

                const Int dx = Abs( V_coords(v1,0) - V_coords(v0,0) );
                const Int dy = Abs( V_coords(v1,1) - V_coords(v0,1) );
                const Int len = dx + dy;

                if( len > best_len )
                {
                    best_len  = len;
                    best_edge = e;
                }
            }

            std::string label;

            if( settings.label_arcsQ )
            {
                label += std::to_string(a);
            }

            if( settings.label_levelsQ
                && ToSize_T(a) < settings.arc_levels.size() )
            {
                label += "(L" + std::to_string(settings.arc_levels[ToSize_T(a)]) + ")";
            }

            int label_group = has_highlights ? arc_group[ToSize_T(a)] : -1;

            const Int v0 = E_V(best_edge,0);
            const Int v1 = E_V(best_edge,1);

            const Int x0 = V_coords(v0,0);
            const Int y0 = V_coords(v0,1);
            const Int x1 = V_coords(v1,0);
            const Int y1 = V_coords(v1,1);

            if( y0 == y1 )
            {
                // Horizontal edge: overlay digits on the dashes at the midpoint.
                const Int mid_x = (x0 + x1) / Int(2) - static_cast<Int>(label.size()) / Int(2);
                set_string( mid_x, y0, label );
                set_string_color( mid_x, y0, label.size(), label_group );
            }
            else
            {
                // Vertical edge: place digits to the right of the '|' at midpoint height.
                const Int mid_y = (y0 + y1) / Int(2);
                set_string( x0 + Int(1), mid_y, label );
                set_string_color( x0 + Int(1), mid_y, label.size(), label_group );
            }
        }
    }

    // Pass 5.5: Jump markers for level transitions.
    if( settings.label_levelsQ && !settings.arc_levels.empty() )
    {
        const auto & A_next_A = ArcNextArc();

        for( Int a = 0; a < A_count; ++a )
        {
            if( !EdgeActiveQ(a) ) { continue; }
            if( ToSize_T(a) >= settings.arc_levels.size() ) { continue; }

            const Int next_a = A_next_A[a];

            if( ToSize_T(next_a) >= settings.arc_levels.size() ) { continue; }
            if( settings.arc_levels[ToSize_T(a)] == settings.arc_levels[ToSize_T(next_a)] ) { continue; }

            const Int k_begin = A_E.Pointers()[a  ];
            const Int k_end   = A_E.Pointers()[a+1];

            if( k_begin >= k_end ) { continue; }

            // Find the longest edge (label_edge) — same logic as Pass 5.
            Int label_edge = A_E.Elements()[k_begin];
            Int label_len  = Int(0);

            for( Int k = k_begin; k < k_end; ++k )
            {
                const Int e  = A_E.Elements()[k];
                const Int v0 = E_V(e,0);
                const Int v1 = E_V(e,1);

                const Int dx = Abs( V_coords(v1,0) - V_coords(v0,0) );
                const Int dy = Abs( V_coords(v1,1) - V_coords(v0,1) );
                const Int len = dx + dy;

                if( len > label_len )
                {
                    label_len  = len;
                    label_edge = e;
                }
            }

            // Jump edge = last edge of arc (closest to head crossing).
            Int jump_edge = A_E.Elements()[k_end - 1];

            if( jump_edge == label_edge && k_end - k_begin > Int(1) )
            {
                jump_edge = A_E.Elements()[k_end - 2];
            }

            // Compute jump edge length and place marker.
            const Int jv0 = E_V(jump_edge,0);
            const Int jv1 = E_V(jump_edge,1);
            const Int jdx = Abs( V_coords(jv1,0) - V_coords(jv0,0) );
            const Int jdy = Abs( V_coords(jv1,1) - V_coords(jv0,1) );
            const Int edge_len = jdx + jdy;

            std::string marker;
            if( edge_len >= Int(8) )
            {
                marker = "(JUMP)";
            }
            else if( edge_len >= Int(5) )
            {
                marker = "(J)";
            }
            else
            {
                marker = "J";
            }

            int jump_group = has_highlights ? arc_group[ToSize_T(a)] : -1;

            const Int jx0 = V_coords(jv0,0);
            const Int jy0 = V_coords(jv0,1);
            const Int jx1 = V_coords(jv1,0);
            const Int jy1 = V_coords(jv1,1);

            if( jump_edge != label_edge )
            {
                // Place at midpoint of jump edge.
                if( jy0 == jy1 )
                {
                    const Int mid_x = (jx0 + jx1) / Int(2) - static_cast<Int>(marker.size()) / Int(2);
                    set_string( mid_x, jy0, marker );
                    set_string_color( mid_x, jy0, marker.size(), jump_group );
                }
                else
                {
                    const Int mid_y = (jy0 + jy1) / Int(2);
                    set_string( jx0 + Int(1), mid_y, marker );
                    set_string_color( jx0 + Int(1), mid_y, marker.size(), jump_group );
                }
            }
            else
            {
                // Single-edge fallback: place at 3/4 point (closer to head).
                if( jy0 == jy1 )
                {
                    const Int qx = jx0 + (jx1 - jx0) * Int(3) / Int(4) - static_cast<Int>(marker.size()) / Int(2);
                    set_string( qx, jy0, marker );
                    set_string_color( qx, jy0, marker.size(), jump_group );
                }
                else
                {
                    const Int qy = jy0 + (jy1 - jy0) * Int(3) / Int(4);
                    set_string( jx0 + Int(1), qy, marker );
                    set_string_color( jx0 + Int(1), qy, marker.size(), jump_group );
                }
            }
        }
    }

    // Pass 6: Face labels via flood fill on the character grid.
    if( settings.label_facesQ )
    {
        const auto grid_size = ToSize_T(n_x * n_y + 100);

        // Component ID per cell: -1 = unvisited, >= 0 = component index.
        std::vector<int> comp_id( grid_size, -1 );

        // Per-component info.
        struct CompInfo
        {
            double sum_x = 0.0;
            double sum_y = 0.0;
            int    count = 0;
            bool   exteriorQ = false;
        };
        std::vector<CompInfo> comps;

        // Stack for iterative flood fill.
        std::vector<std::pair<Int,Int>> stack;

        // Scan every cell; flood-fill connected components of space characters.
        for( Int gy = 0; gy < n_y; ++gy )
        {
            for( Int gx = 0; gx < n_x - Int(1); ++gx ) // skip newline column
            {
                const auto idx = ToSize_T( gx + n_x * (n_y - Int(1) - gy) );
                if( idx >= grid_size ) continue;
                if( comp_id[idx] >= 0 ) continue;
                if( s[idx] != ' ' ) continue;

                // Start a new component.
                int cid = static_cast<int>( comps.size() );
                comps.emplace_back();
                CompInfo & ci = comps.back();

                stack.clear();
                stack.push_back({gx, gy});
                comp_id[idx] = cid;

                while( !stack.empty() )
                {
                    auto [cx, cy] = stack.back();
                    stack.pop_back();

                    ci.sum_x += static_cast<double>(cx);
                    ci.sum_y += static_cast<double>(cy);
                    ci.count += 1;

                    // Boundary touch check (grid coordinates).
                    if( cx == Int(0) || cx == n_x - Int(2)
                     || cy == Int(0) || cy == n_y - Int(1) )
                    {
                        ci.exteriorQ = true;
                    }

                    // 4-connected neighbours.
                    const std::pair<Int,Int> nbrs[4] = {
                        {cx - Int(1), cy}, {cx + Int(1), cy},
                        {cx, cy - Int(1)}, {cx, cy + Int(1)}
                    };

                    for( auto [nx2, ny2] : nbrs )
                    {
                        if( nx2 < Int(0) || nx2 >= n_x - Int(1) ) continue;
                        if( ny2 < Int(0) || ny2 >= n_y )           continue;

                        const auto nidx = ToSize_T( nx2 + n_x * (n_y - Int(1) - ny2) );
                        if( nidx >= grid_size )     continue;
                        if( comp_id[nidx] >= 0 )    continue;
                        if( s[nidx] != ' ' )         continue;

                        comp_id[nidx] = cid;
                        stack.push_back({nx2, ny2});
                    }
                }
            }
        }

        // Label all faces with sequential numbers.
        // Interior faces: label at centroid.
        // Exterior face (all boundary-touching components): single label in upper-right corner.

        // First, assign the exterior face number (face 0) and label it.
        int face_number = 0;
        {
            std::string label = "F" + std::to_string(face_number);

            Int ext_x = n_x - Int(2) - static_cast<Int>(label.size());
            Int ext_y = n_y - Int(1);

            bool fits = true;
            for( std::size_t i = 0; i < label.size(); ++i )
            {
                Int lx = ext_x + static_cast<Int>(i);
                if( lx < Int(0) || lx >= n_x - Int(1) )
                {
                    fits = false;
                    break;
                }
                auto li = ToSize_T( lx + n_x * (n_y - Int(1) - ext_y) );
                if( li >= grid_size || s[li] != ' ' )
                {
                    fits = false;
                    break;
                }
            }

            if( fits )
            {
                set_string( ext_x, ext_y, label );
            }

            ++face_number;
        }

        // Helper: check whether a grid cell holds a diagram edge character
        // (safe to overwrite with a label).
        auto is_edge_char = [](char c) -> bool
        {
            return c == '-' || c == '|' || c == '+' ||
                   c == '>' || c == '<' || c == '^' || c == 'v';
        };

        // Now label interior faces.
        for( int cid = 0; cid < static_cast<int>(comps.size()); ++cid )
        {
            const CompInfo & ci = comps[ToSize_T(cid)];

            if( ci.exteriorQ ) continue;

            std::string label = "F" + std::to_string(face_number);

            // Compute centroid in grid coordinates.
            Int cent_x = static_cast<Int>( ci.sum_x / ci.count );
            Int cent_y = static_cast<Int>( ci.sum_y / ci.count );

            bool placed = false;

            // Try interior placement if the face is large enough.
            // Collect all cells in this face sorted by distance from centroid,
            // then try each as a label anchor until one fits.
            if( ci.count >= static_cast<int>(label.size()) )
            {
                auto lsize = static_cast<Int>(label.size());

                // Gather face cells sorted by distance from centroid.
                std::vector<std::pair<Int,std::pair<Int,Int>>> candidates;

                for( Int sy = 0; sy < n_y; ++sy )
                {
                    for( Int sx = 0; sx < n_x - Int(1); ++sx )
                    {
                        auto si = ToSize_T( sx + n_x * (n_y - Int(1) - sy) );
                        if( si >= grid_size || comp_id[si] != cid ) continue;
                        Int d = Abs(sx - cent_x) + Abs(sy - cent_y);
                        candidates.push_back({ d, {sx, sy} });
                    }
                }

                std::sort( candidates.begin(), candidates.end() );

                for( const auto & [dist, pos] : candidates )
                {
                    auto [px, py] = pos;

                    // Center the label on this cell.
                    Int label_x = px - lsize / Int(2);

                    bool fits = true;
                    for( Int i = 0; i < lsize; ++i )
                    {
                        Int lx = label_x + i;
                        if( lx < Int(0) || lx >= n_x - Int(1) )
                            { fits = false; break; }
                        auto li = ToSize_T( lx + n_x * (n_y - Int(1) - py) );
                        if( li >= grid_size || s[li] != ' ' )
                            { fits = false; break; }
                    }

                    if( fits )
                    {
                        set_string( label_x, py, label );
                        placed = true;
                        break;
                    }
                }
            }

            // If interior placement failed, place on an adjacent edge with arrow.
            if( !placed )
            {
                // Find the cell in this face closest to the centroid.
                Int face_x = -1, face_y = -1;
                Int best_dist = n_x + n_y;

                for( Int sy = 0; sy < n_y; ++sy )
                {
                    for( Int sx = 0; sx < n_x - Int(1); ++sx )
                    {
                        auto si = ToSize_T( sx + n_x * (n_y - Int(1) - sy) );
                        if( si >= grid_size || comp_id[si] != cid ) continue;

                        Int d = Abs(sx - cent_x) + Abs(sy - cent_y);
                        if( d < best_dist )
                        {
                            best_dist = d;
                            face_x = sx;
                            face_y = sy;
                        }
                    }
                }

                if( face_x >= Int(0) )
                {
                    // Try 4 directions from face cell to find an adjacent edge.
                    // Arrow points TOWARD the face.
                    struct DirInfo { Int dx; Int dy; char arrow; bool horiz; };
                    const DirInfo dirs[4] = {
                        { Int(0),  Int(1), 'v', true  }, // edge above  -> arrow v
                        { Int(0), Int(-1), '^', true  }, // edge below  -> arrow ^
                        { Int(-1), Int(0), '>', false }, // edge left   -> arrow >
                        { Int(1),  Int(0), '<', false }  // edge right  -> arrow <
                    };

                    for( int d = 0; d < 4 && !placed; ++d )
                    {
                        Int ex = face_x + dirs[d].dx;
                        Int ey = face_y + dirs[d].dy;

                        if( ex < Int(0) || ex >= n_x - Int(1) ) continue;
                        if( ey < Int(0) || ey >= n_y )           continue;

                        auto ei = ToSize_T( ex + n_x * (n_y - Int(1) - ey) );
                        if( ei >= grid_size || !is_edge_char(s[ei]) ) continue;

                        std::string arrow_label = label + dirs[d].arrow;
                        auto alen = static_cast<Int>(arrow_label.size());

                        if( dirs[d].horiz )
                        {
                            // Place horizontally on row ey, centered on ex.
                            Int lx = ex - alen / Int(2);

                            bool fits = true;
                            for( Int i = 0; i < alen; ++i )
                            {
                                Int xi = lx + i;
                                if( xi < Int(0) || xi >= n_x - Int(1) )
                                    { fits = false; break; }
                                auto idx = ToSize_T(
                                    xi + n_x * (n_y - Int(1) - ey) );
                                if( idx >= grid_size || !is_edge_char(s[idx]) )
                                    { fits = false; break; }
                            }

                            if( fits )
                            {
                                for( Int i = 0; i < alen; ++i )
                                {
                                    auto idx = ToSize_T(
                                        (lx + i) + n_x * (n_y - Int(1) - ey) );
                                    s[idx] = arrow_label[ToSize_T(i)];
                                }
                                placed = true;
                            }
                        }
                        else
                        {
                            // Place vertically on column ex, centered on ey.
                            Int ly = ey + alen / Int(2);  // top of label (high y)

                            bool fits = true;
                            for( Int i = 0; i < alen; ++i )
                            {
                                Int yi = ly - i;  // top to bottom
                                if( yi < Int(0) || yi >= n_y )
                                    { fits = false; break; }
                                auto idx = ToSize_T(
                                    ex + n_x * (n_y - Int(1) - yi) );
                                if( idx >= grid_size || !is_edge_char(s[idx]) )
                                    { fits = false; break; }
                            }

                            if( fits )
                            {
                                for( Int i = 0; i < alen; ++i )
                                {
                                    Int yi = ly - i;
                                    auto idx = ToSize_T(
                                        ex + n_x * (n_y - Int(1) - yi) );
                                    s[idx] = arrow_label[ToSize_T(i)];
                                }
                                placed = true;
                            }
                        }
                    }
                }
            }

            ++face_number;
        }
    }

    // Color-aware serialization.
    if( !has_highlights )
    {
        return s;
    }

    using HM = typename OrthoDraw::HighlightMode_T;

    switch( settings.highlight_mode )
    {
        case HM::Chars:
        {
            // Character substitution tables: [group][0]=horiz, [group][1]=vert
            static constexpr char kCharSubst[][2] = {
                { '=', '!' },  // group 0
                { '~', ':' },  // group 1
                { '#', '%' },  // group 2
                { '*', '$' },  // group 3
                { '@', '&' },  // group 4
                { '`', ';' },  // group 5
            };
            static constexpr int kCharSubstCount = 6;

            for( std::size_t i = 0; i < s.size(); ++i )
            {
                if( i >= color_map.size() ) break;

                int g = color_map[i];

                if( g < 0 ) continue;

                char c = s[i];

                if( c == '-' )
                {
                    s[i] = kCharSubst[g % kCharSubstCount][0];
                }
                else if( c == '|' )
                {
                    s[i] = kCharSubst[g % kCharSubstCount][1];
                }
            }

            return s;
        }

        case HM::HTML:
        {
            static constexpr const char* kHtmlColors[] = {
                "#ff6666",  // group 0
                "#66ff66",  // group 1
                "#6666ff",  // group 2
                "#ffff66",  // group 3
                "#ff66ff",  // group 4
                "#66ffff",  // group 5
            };
            static constexpr int kHtmlColorCount = 6;

            std::string result;
            result.reserve( s.size() + s.size() / 2 );

            result += "<pre>";

            int current_group = -1;

            for( std::size_t i = 0; i < s.size(); ++i )
            {
                char c = s[i];

                if( c == '\n' )
                {
                    if( current_group >= 0 )
                    {
                        result += "</span>";
                        current_group = -1;
                    }
                    result += c;
                    continue;
                }

                int g = (i < color_map.size()) ? color_map[i] : -1;

                if( g != current_group )
                {
                    if( current_group >= 0 )
                    {
                        result += "</span>";
                    }
                    if( g >= 0 )
                    {
                        result += "<span style=\"color:";
                        result += kHtmlColors[g % kHtmlColorCount];
                        result += "\">";
                    }
                    current_group = g;
                }

                // HTML-escape special characters.
                if( c == '<' )       result += "&lt;";
                else if( c == '>' )  result += "&gt;";
                else if( c == '&' )  result += "&amp;";
                else                 result += c;
            }

            if( current_group >= 0 )
            {
                result += "</span>";
            }

            result += "</pre>";

            return result;
        }

        case HM::ANSI:
        default:
        {
            // Walk character grid and color_map in parallel, emitting ANSI escape codes.
            std::string result;
            result.reserve( s.size() + s.size() / 4 ); // extra room for escape codes

            int current_group = -1;

            for( std::size_t i = 0; i < s.size(); ++i )
            {
                char c = s[i];

                if( c == '\n' )
                {
                    // Reset color before newline for clean line endings.
                    if( current_group >= 0 )
                    {
                        result += kColorReset;
                        current_group = -1;
                    }
                    result += c;
                    continue;
                }

                int g = (i < color_map.size()) ? color_map[i] : -1;

                if( g != current_group )
                {
                    if( g >= 0 )
                    {
                        result += kHighlightColors[g % kHighlightColorCount];
                    }
                    else if( current_group >= 0 )
                    {
                        result += kColorReset;
                    }
                    current_group = g;
                }

                result += c;
            }

            // Final reset if we ended in a color.
            if( current_group >= 0 )
            {
                result += kColorReset;
            }

            return result;
        }
    }
}

cref<ArcSplineContainer_T> ArcLines()
{
    TOOLS_PTIMER(timer,ClassName()+"::ArcLines");

    if( !this->InCacheQ("ArcLines") )
    {
        const CoordsContainer_T & V_coords = VertexCoordinates();
        
        const Int v_count = V_dE.Dim(0);
        const Int a_count = A_C.Dim(0);
        
        ArcSplineContainer_T A_lines ( a_count, A_V.ElementCount() );
        
        using A_T = std::array<Int,2>;
        
        for( Int a = 0; a < a_count; ++a )
        {
            if( !EdgeActiveQ(a) )
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
        
        if( A_lines.SublistCount() != a_count )
        {
            eprint(MethodName("ArcLines")+": A_lines.SublistCount() != a_count.");
            TOOLS_DDUMP(A_lines.SublistCount());
            TOOLS_DDUMP(a_count);
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
            if( !DedgeActiveQ(ToDarc(a,Tail)) )
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


