public:

static PD_T Unknot( const Int color )
{
    // needs to know all member variables
    
    PD_T pd ( Int(0) );
    pd.last_color_deactivated = color;
    pd.proven_minimalQ = true;
    pd.SetCache("LinkComponentCount",Int(1));
    return pd;
}

static PD_T HopfLink( const Int color_0, const Int color_1 )
{
    // needs to know all member variables
    
    PD_T pd ( Int(2), true );
    pd.crossing_count  = pd.max_crossing_count;
    pd.arc_count       = pd.max_crossing_count;
    pd.proven_minimalQ = true;
    pd.SetCache("LinkComponentCount",Int(2));
    
    constexpr Int C[2][2][2] = {{{0, 3}, {2, 1}}, {{2, 1}, {0, 3}}};
    pd.C_arcs.Read(&C[0][0][0]);
    pd.C_state.Fill(CrossingState_T::RightHanded);
//    pd.C_arcs(0,0,0) = 0;
//    pd.C_arcs(0,0,1) = 3;
//    pd.C_arcs(0,1,0) = 2;
//    pd.C_arcs(0,1,1) = 1;
//    pd.C_arcs(1,0,0) = 2;
//    pd.C_arcs(1,0,1) = 1;
//    pd.C_arcs(1,1,0) = 0;
//    pd.C_arcs(1,1,1) = 3;
    
//    pd.A_cross(0,0) = 0;
//    pd.A_cross(0,1) = 1;
//    pd.A_cross(1,0) = 1;
//    pd.A_cross(1,1) = 0;
//    pd.A_cross(2,0) = 1;
//    pd.A_cross(2,1) = 0;
//    pd.A_cross(3,0) = 0;
//    pd.A_cross(3,1) = 1;
//    
    constexpr Int A[6][2] = {{0, 1}, {1, 0}, {1, 0}, {0, 1}};
    pd.A_cross.Read(&A[0][0]);
    pd.A_state.Fill(ArcState_T::Active);
    
    pd.A_color[0] = color_0;
    pd.A_color[1] = color_0;
    pd.A_color[2] = color_1;
    pd.A_color[3] = color_1;

    return pd;
}

static PD_T TrefoilKnot( const Int color, const CrossingState_T handedness )
{
    // needs to know all member variables
    
    PD_T pd ( Int(3), true );
    pd.crossing_count  = pd.max_crossing_count;
    pd.arc_count       = pd.max_crossing_count;
    pd.proven_minimalQ = true;
    pd.SetCache("LinkComponentCount",Int(1));
    
    constexpr Int C[3][2][2] = {{{3, 0}, {5, 2}}, {{1, 4}, {3, 0}}, {{5, 2}, {1, 4}}};
    pd.C_arcs.Read(&C[0][0][0]);
    pd.C_state.Fill(handedness);
    
    constexpr Int A[6][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 1}, {1, 2}, {2, 0}};
    pd.A_cross.Read(&A[0][0]);
    pd.A_state.Fill(ArcState_T::Active);
    pd.A_color.Fill(color);
    
    return pd;
}

static PD_T FigureEightKnot( const Int color )
{
    // needs to know all member variables
    
    PD_T pd ( Int(4), true );
    pd.crossing_count  = pd.max_crossing_count;
    pd.arc_count       = pd.max_crossing_count;
    pd.proven_minimalQ = true;
    pd.SetCache("LinkComponentCount",Int(1));
    
    constexpr Int C[4][2][2] = {{{3, 0}, {7, 2}}, {{6, 1}, {0, 5}}, {{2, 5}, {4, 1}}, {{7, 4}, {3, 6}}};
    pd.C_arcs.Read(&C[0][0][0]);
    pd.C_state[0] = CrossingState_T::RightHanded;
    pd.C_state[1] = CrossingState_T::LeftHanded;
    pd.C_state[2] = CrossingState_T::RightHanded;
    pd.C_state[3] = CrossingState_T::LeftHanded;
    
    constexpr Int A[8][2] = {{0, 1}, {1, 2}, {2, 0}, {0, 3}, {3, 2}, {2, 1}, {1, 3}, {3, 0}};
    pd.A_cross.Read(&A[0][0]);
    pd.A_state.Fill(ArcState_T::Active);
    pd.A_color.Fill(color);
    
    return pd;
}
