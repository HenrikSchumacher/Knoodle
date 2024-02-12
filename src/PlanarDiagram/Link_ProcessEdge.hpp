PD_print("\t\tBegin of original edge " + ToString(i));
PD_print("\t\t{");


// v_00         __ "+" means inserted point to separate crossings.
//   |         /
//   v        v
//   |---->---+---->---X---->---+---->---+---->---X---->---+---->---.....----->---|
//   .       .^                                   ^                               ^
//   .       . \                                   \                               \ "|" means orginal vertex.
//   \_______/  \__ This one is excluded.           \__ "X" means crossing          \To be dealt with by next edge.
//       ^
//        \___ Dealing with this part here.
//
// First vertex of edge is taken anyways...
Int v_00 = (edge_idx >= edge_begin) ? edge_idx : edge_end - (edge_begin - edge_idx);
x[0][v_00] = p[0][0][i];
x[1][v_00] = p[0][1][i];
x[2][v_00] = p[0][2][i];
PD_assert( v_00 < pd->EdgeCount());
e[0][v_00] = v_00;
PD_assert( v_00+1 < pd->EdgeCount());
e[1][v_00] = v_00+1;
++edge_idx;
PD_print("\t\t\tStoring edge "+ToString(v_00)+" = { "+ToString(v_00)+", "+ToString(v_00+1)+" } ");

// We want that any two neighboring crossings are always separated by an arc with at least one edge.
// Thus we insert
//      -   a vertex before the crossing
//      -   a vertex onto the crossing (up or down vertex of the crossing)
//      -   a vertex after the crossing

Real t_00 = 0;

for( Int k = k_begin; k < k_end; ++k )
{
    PD_print("\t\t\tBegin of crossing " + ToString(k));
    PD_print("\t\t\t{");
    const bool not_last_crossing = (k < k_end-1);
    
    const Int crossing_index = edge_intersections[k];
    
    Intersection_T & I = intersections[crossing_index];
    
    PD_assert( (I.edges[0] == i) || (I.edges[1] == i) );
    
    // Crossing time.
    const Real t = edge_times[k];
    
    // Time before crossing.
    const Real t_0 = (t_00 + t) / static_cast<Real>(2);
 
    const Real t_11 = not_last_crossing ? edge_times[k+1] : 1;
    
    // Time after crossing.
    const Real t_1 = (t + t_11) / static_cast<Real>(2);
    
    // Prepare t_00 for next crossing.
    t_00 = t_1;

    
//                             v_00     v_0       v       v_1
//                              |        |        |        |
//                              v        v        v        v
//   |------.......----X---->---+---->---+---->---X---->---+---->---+---------.....---------|
//                                       ^       ^
//                                       |       |
//                                       \_______/
//                                           ^
//                                            \___ Dealing with this part here.
    
    const Int v_0 = (edge_idx >= edge_begin) ? edge_idx : edge_end - (edge_begin - edge_idx);
    const Int v = edge_idx+1;
    PD_assert( v_0 < pd->EdgeCount());
    PD_assert( v < pd->EdgeCount());
    
    x[0][v_0] = p[0][0][i]*(one-t_0) + t_0 *p[1][0][i];
    x[1][v_0] = p[0][1][i]*(one-t_0) + t_0 *p[1][1][i];
    x[2][v_0] = p[0][2][i]*(one-t_0) + t_0 *p[1][2][i];
    e[0][v_0] = v_0;
    e[1][v_0] = v;
    ++edge_idx;
    PD_print("\t\t\t\tStoring edge "+ToString(v_0)+" = { "+ToString(v_0)+", "+ToString(v)+" } ");
    
//                           v_00     v_0       v       v_1
//                              |        |        |        |
//                              v        v        v        v
//   |------.......----X---->---+---->---+---->---X---->---+---->---+---------.....---------|
//                                                ^       ^
//                                                |       |
//                                                \_______/
//                                                    ^
//                                                     \___ Dealing with this part here.
    
    x[0][v] = p[0][0][i]*(one-t) + t *p[1][0][i];
    x[1][v] = p[0][1][i]*(one-t) + t *p[1][1][i];
    x[2][v] = p[0][2][i]*(one-t) + t *p[1][2][i];
    e[0][v] = v;
    e[1][v] = v+1;
    ++edge_idx;
    PD_print("\t\t\t\tStoring edge "+ToString(v)+" = { "+ToString(v)+", "+ToString(v+1)+" } ");
    
    
//                             v_00     v_0       v       v_1      v_11 ( can be a wrap-around!!!)
//                              |        |        |        |        |
//                              v        v        v        v        V
//   |------.......----X---->---+---->---+---->---X---->---+---->---+---------.....---------|
//                                                         ^       ^
//                                                         |       |
//                                                         \_______/
//                                                             ^
//                                                              \___ Dealing with this part here.
    
    // Beware of wrap-around if at component's end.
    const Int v_1  = edge_idx;
    const Int v_11 = (v_1 < edge_end-1) ? v_1 + 1 : edge_begin;
    x[0][v_1] = p[0][0][i]*(one-t_1) + t_1 *p[1][0][i];
    x[1][v_1] = p[0][1][i]*(one-t_1) + t_1 *p[1][1][i];
    x[2][v_1] = p[0][2][i]*(one-t_1) + t_1 *p[1][2][i];
    PD_assert( v_1 < pd->EdgeCount());
    e[0][v_1] = v_1;
    e[1][v_1] = v_11;
    ++edge_idx;
    PD_print("\t\t\t\tStoring edge "+ToString(v_1)+" = { "+ToString(v_1)+", "+ToString(v_11)+" } ");
    
    
    typename PD_T::Crossing_T & C = crossings[crossing_index];
    
    const Int a_prev = ( arc_idx > arc_begin ) ? arc_idx-1 : arc_end-1;
    const Int a_next = arc_idx;
    
    typename PD_T::Arc_T & A_prev = arcs[a_prev];
    typename PD_T::Arc_T & A_next = arcs[a_next];
    
    PD_valprint("\t\t\t\ta_prev",a_prev);
    PD_valprint("\t\t\t\ta_next",a_next);
    PD_valprint("\t\t\t\tA_prev.Identifier()",A_prev.Identifier());
    PD_valprint("\t\t\t\tA_next.Identifier()",A_next.Identifier());

    C.State() = (I.sign > 0) ? Crossing_State::Positive : Crossing_State::Negative;
    
    const Crossing_OU ou = (I.edges[0] == i) ? Crossing_OU::Over : Crossing_OU::Under;
    
    C.VertexIndex( Crossing_IO::In,  ou )  = v_0;
    C.VertexIndex(                   ou )  = v;
    C.VertexIndex( Crossing_IO::Out, ou )  = v_1;
    
    C.EdgeIndex  ( Crossing_IO::In,  ou )  = v_0;
    C.EdgeIndex  ( Crossing_IO::Out, ou )  = v;

    
    C.ArcIndex   ( Crossing_IO::In,  ou )  = A_prev.Identifier();
    C.ArcIndex   ( Crossing_IO::Out, ou )  = A_next.Identifier();
    
    A_prev.TipCrossingIndex()  = C.Identifier();
    A_next.TailCrossingIndex() = C.Identifier();
    
    A_prev.TipEdgeIndex()    = v_00;
    A_next.TailEdgeIndex()   = v_1;
    
    // We have to remember v_00 so that the next arc can link to it's edge.
    v_00 = v_11;
    
    ++arc_idx;
    
    PD_print("\t\t\t}");
    PD_print("\t\t\tEnd   of crossing " + ToString(k));
    PD_print("");
}

PD_print("\t\t}");
PD_print("\t\tEnd   of original edge " + ToString(i));
PD_print("");
