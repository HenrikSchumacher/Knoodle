PD_print("\t\tBegin of original edge " + ToString(i) + " (simple)");
PD_print("\t\t{");


const Int v   = edge_idx;
const Int v_1 = (edge_idx < edge_end-1) ? edge_idx+1 : edge_begin;
x[0][v] = p[0][0][i];
x[1][v] = p[0][1][i];
x[2][v] = p[0][2][i];
assert( v < pd->EdgeCount());
e[0][v] = v;
e[1][v] = v_1;
++edge_idx;
PD_print("\t\t\tStoring edge "+ToString(v)+" = { "+ToString(v)+", "+ToString(v_1)+" } ");

PD_print("\t\t}");
PD_print("\t\tEnd   of original edge " + ToString(i) + " (simple)");
PD_print("");
