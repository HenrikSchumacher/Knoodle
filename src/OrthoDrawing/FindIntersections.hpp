public:

/*!@brief Computes the minimal Manhattan distance between two line segments `{X_0,X_1}` and `{Y_0,Y_1}` that are horizontal or vertical.
 */

static Int EdgeEdgeManhattanDistance(
    cptr<Int> X_0, cptr<Int> X_1, cptr<Int> Y_0, cptr<Int> Y_1
)
{
    Int dist = 0;
    
    for( Int k = 0; k < Int(2); ++k )
    {
        auto [a,b] = MinMax( X_0[k], X_1[k] );
        auto [c,d] = MinMax( Y_0[k], Y_1[k] );
        
        const Int A = Max(a,c);
        const Int B = Min(b,d);
        
        const Int x = (A > B) ? (A-B) : Int(0);
        
        dist += Abs(x);
    }

    return dist;
}

/*!@brief Computes the minimal Manhattan distance between two line segments given by the undirected edges `e_0` and `e_1` with vertex coordinates given by `coords`.
 */

Int EdgeEdgeManhattanDistance(
    cref<CoordsContainer_T> coords, const Int e_0, const Int e_1
)
{    
    const Int v [2][2] = {
        { E_V(e_0,Tail), E_V(e_0,Head) },
        { E_V(e_1,Tail), E_V(e_1,Head) },
    };
    
    // If the edges interect because their end vertices coincide (which can happen, e.g., if the PlanarDiagram had a Reidemeister I loop), then we have to report "no intersection"
    if(
       ( v[0][0] == v[1][0] )
       ||
       ( v[0][0] == v[1][1] )
       ||
       ( v[0][1] == v[1][0] )
       ||
       ( v[0][1] == v[1][1] )
    )
    {
        return Int(1);
    }
    
    return EdgeEdgeManhattanDistance(
        coords.data(v[0][0]),
        coords.data(v[0][1]),
        coords.data(v[1][0]),
        coords.data(v[1][1])
    );
}

template<bool verboseQ = false>
Tiny::VectorList_AoS<3,Int,Int> FindAllIntersections(
    cref<CoordsContainer_T> coords
)
{
    std::vector<std::array<Int,3>> agg;

    // TODO: Better use TraverseAllFaces!
    
    
    // If we soften the virtual edges, then we should walk along the faces of the turn-nonregularized facs. Otherwise we risk detecting some intersections. (And the Manhattan distance won't work on the virtual edges are they might be diagonal.)
    cref<RaggedList<Int,Int>> F_dE = FaceDedges(settings.soften_virtual_edgesQ);
    
    const Int f_count = F_dE.SublistCount();
    
    for( Int f = 0; f < f_count; ++f )
    {
        this->template FindIntersections<verboseQ>(coords,F_dE,agg,f);
    }
    
    Tiny::VectorList_AoS<3,Int,Int> result ( int_cast<Int>( agg.size() ) );
    result.Read( &agg[0][0] );
    
    return result;
}

/*!@brief Cycles around the face ` f` and finds the first directed edge `de` that has at least one intersection with another edge. Then it returns the triple `{de,da,db}`, where `da` and `db` are the first edge and the last edge of the face that `de` intersects.
 *
 */

// TODO: Make this independent of RequireFaces.

template<bool verboseQ = false>
void FindIntersections(
    cref<CoordsContainer_T> coords,
    cref<RaggedList<Int,Int>> F_dE,
    mref<std::vector<std::array<Int,3>>> agg,
    const Int f
)
{
//    print("===============================================");
    if constexpr ( verboseQ )
    {
        print("FindIntersections("+ToString(f)+")");
    }
    
    const Int face_begin = F_dE.Pointers()[f    ];
    const Int face_end   = F_dE.Pointers()[f + 1];
    
    const Int n = face_end - face_begin;
    
    
//    TOOLS_DUMP(n);
    
    if( n <= Int(4) )
    {
        return;
    }
    
//    valprint("face", ArrayToString( &F_dE_idx[face_begin], {n} ) );
    
    for( Int i = 0; i < n; ++i )
    {
        const Int de_i = F_dE.Elements()[face_begin + i];
        auto [e_i,d_i] = FromDarc(de_i);
        
        if( settings.soften_virtual_edgesQ && DedgeVirtualQ(de_i) )
        {
            print("!");
            continue;
        }
        
        Int da = Uninitialized;
        Int db = Uninitialized;
        
//        TOOLS_DUMP(i);
//        TOOLS_DUMP(de_i);
//        TOOLS_DUMP(e_i);
        
        for( Int delta = Int(2); delta < n - Int(1); ++delta )
        {
//            print("forward run");
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(i + delta);
            Int j = (i + delta < n) ? (i + delta) : (i + delta - n);
            const Int de_j = F_dE.Elements()[face_begin + j];
            
            if( settings.soften_virtual_edgesQ && DedgeVirtualQ(de_j) )
            {
                print("!");
                continue;
            }
            
            auto [e_j,d_j] = FromDarc(de_j);
            
//            TOOLS_DUMP(j);
//            TOOLS_DUMP(de_j);
//            TOOLS_DUMP(e_j);
//            TOOLS_DUMP(ModDistance(n,i,j));
            
            const Int dist = EdgeEdgeManhattanDistance(coords,e_i,e_j);
            
            if( dist <= Int(0) )
            {
                da = de_j;
                break;
            }
        }
        
        if( da == Uninitialized )
        {
//            print(">>>>>>>>>>>> forward run found no intersection for de_i = " + ToString(de_i));
            continue;
        }
        
//        print(">>>>>>>>>>>> forward run found intersection for de_i = " + ToString(de_i) + " at da = " + ToString(da));
        
        for( Int delta = n - Int(1); delta --> Int(2); )
        {
//            print("backward run");
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(i + delta);
            
            Int j = (i + delta < n) ? (i + delta) : (i + delta - n);
            const Int de_j = F_dE.Elements()[face_begin + j];
            
            if( settings.soften_virtual_edgesQ && DedgeVirtualQ(de_j) )
            {
                print("!");
                continue;
            }
            
            auto [e_j,d_j] = FromDarc(de_j);
            
//            TOOLS_DUMP(delta);
//            TOOLS_DUMP(j);
//            TOOLS_DUMP(de_j);
//            TOOLS_DUMP(e_j);
//            TOOLS_DUMP(ModDistance(n,i,j));
            
            const Int dist = EdgeEdgeManhattanDistance(coords,e_i,e_j);
            
            if( dist <= Int(0) )
            {
                db = de_j;
                break;
            }
        }
        
        std::array<Int,3> result = {de_i,da,db};
        
//        const Int a = da / 2;
//        const Int b = db / 2;
//        
//        TOOLS_DUMP(de_i);
//        TOOLS_DUMP(e_i);
//        TOOLS_DUMP(da);
//        TOOLS_DUMP(a);
//        TOOLS_DUMP(db);
//        TOOLS_DUMP(b);
//        
//        TOOLS_DUMP(FaceSize(f));
//        
//        TOOLS_DUMP(E_V(a,Tail));
//        TOOLS_DUMP(E_V(a,Head));
//        
//        TOOLS_DUMP(E_V(b,Tail));
//        TOOLS_DUMP(E_V(b,Head));
//        
//        valprint("X_0", ArrayToString(coords.data(E_V(a,Tail)),{2}));
//        valprint("X_1", ArrayToString(coords.data(E_V(a,Head)),{2}));
//        valprint("Y_0", ArrayToString(coords.data(E_V(b,Tail)),{2}));
//        valprint("Y_1", ArrayToString(coords.data(E_V(b,Head)),{2}));
//        
//        TOOLS_DUMP(result);
        
        agg.push_back(std::move(result));
        
    } // for( Int i = face_begin; i < face_end; ++i )
}
