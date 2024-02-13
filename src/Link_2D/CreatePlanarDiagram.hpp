    public:
        
        PlanarDiagram<Int> CreatePlanarDiagram()
        {
            constexpr bool Tip   = true;
            constexpr bool Tail  = false;
            constexpr bool Left  = false;
            constexpr bool Right = true;
            constexpr bool In    = true;
            constexpr bool Out   = false;
        
        
            const Int intersection_count = static_cast<Int>(intersections.size());
            
            Int unlink_count = 0;
            for( Int c = 0; c < component_count; ++c )
            {
                // The range of arcs belonging to this component.
                const Int arc_begin  = edge_ptr[component_ptr[c  ]];
                const Int arc_end    = edge_ptr[component_ptr[c+1]];

                if( arc_begin == arc_end )
                {
                    ++unlink_count;
                }
            }
            
            PlanarDiagram<Int> pd ( intersection_count, unlink_count );
            
            //Preparing pointers for quick access.
            
            mptr<Int> C_arcs [2][2] = {
                {pd.Crossings().data(0,0), pd.Crossings().data(0,1)},
                {pd.Crossings().data(1,0), pd.Crossings().data(1,1)}
            };
            
            mptr<Crossing_State> C_state = pd.CrossingStates().data();
            
            mptr<Int> A_crossings [2] = {pd.Arcs().data(0), pd.Arcs().data(1)};
            
            mptr<Arc_State> A_state = pd.ArcStates().data();
            
            // Now we go through all components
            //      then through all edges of the component
            //              then through all intersections of the edge
            // and generate new vertices, edges, crossings, and arcs in one go.
            

            
            PD_print("Begin of Link");
            PD_print("{");
            for( Int comp = 0; comp < component_count; ++comp )
            {
                PD_print("\tBegin of component " + ToString(c));
                PD_print("\t{");
                
                // The range of arcs belonging to this component.
                const Int arc_begin  = edge_ptr[component_ptr[comp  ]];
                const Int arc_end    = edge_ptr[component_ptr[comp+1]];
                
                PD_valprint("\t\tarc_begin", arc_begin);
                PD_valprint("\t\tarc_end"  , arc_end  );

                if( arc_begin == arc_end )
                {
                    // Component is an unlink. Just skip it.
                    continue;
                }
                
                // If we arrive here, then there is definitely a crossing in the first edge.

                for( Int b = arc_begin, a = arc_end-1; b < arc_end; a = (b++) )
                {
                    const Int c = edge_intersections[b];
                    
                    const bool overQ = edge_overQ[b];
                    
                    Intersection_T & inter = intersections[c];
                    
                    A_crossings[Tip ][a] = c; // c is tip  of a
                    A_crossings[Tail][b] = c; // c is tail of b
                    
                    PD_assert( inter.sign > SI(0) || inter.sign < SI(0) );
                    
                    bool positiveQ = inter.sign > SI(0);
                    
                    C_state[c] = positiveQ ? Crossing_State::Positive : Crossing_State::Negative;
                    A_state[a] = Arc_State::Active;
                    
                    /*
                        positiveQ == true and overQ == true:

                          C_arcs[Out][Left][c]  .       .  C_arcs[Out][Right][c] = b
                                                .       .
                                                +       +
                                                 ^     ^
                                                  \   /
                                                   \ /
                                                    /
                                                   / \
                                                  /   \
                                                 /     \
                                                +       +
                                                .       .
                       a = C_arcs[In][Left][c]  .       .  C_arcs[In][Right][c]
                    */
                    const bool over_in_side = (positiveQ == overQ) ? Left : Right ;
                    
                    
                    C_arcs[In ][ over_in_side][c] = a;
                    C_arcs[Out][!over_in_side][c] = b;
                }
        
                
                
                PD_print("\t}");
                PD_print("\tEnd   of component " + ToString(c));
                
                PD_print("");
                
            }
            PD_print("");
            PD_print("}");
            PD_print("End   of Link");
            PD_print("");
            
//            pd.CheckAllCrossings();
//            pd.CheckAllArcs();
            
            return pd;
        }
