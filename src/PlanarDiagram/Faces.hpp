Int ComponentCount()
{
    return comp_arc_ptr.Size()-1;
}

Int FaceCount()
{
    return face_arc_ptr.Size()-1;
}

void RequireFaces()
{
    if( faces_initialized )
    {
        return;
    }
    
    // Maybe not required, but it would be nice if each arc can tell in which component it lies.
    arc_comp = Tensor1<Int,Int>( initial_arc_count );
    
    // These are going to become edges of the dual graph(s). One dual edge for each arc.
//    A_faces = Tiny::VectorList<2,Int,Int>( initial_arc_count );
    
    A_faces = Tensor2<Int,Int>( initial_arc_count,2 );
    
    // Convention: Left face first:
    //
    //            A_faces(a,1)
    //
    //            <------------|  a
    //
    //            A_faces(a,0)
    //
    // Entry -1 means "unvisited but to be visited".
    // Entry -2 means "do not visit".
    
    for( Int a = 0; a < initial_arc_count; ++ a )
    {
        if( ArcActiveQ(a) )
        {
            A_faces(a,0) = -1;
            A_faces(a,1) = -1;
        }
        else
        {
            A_faces(a,0) = -2;
            A_faces(a,1) = -2;
        }
    }
    
    Int          arc_counter = 0;
    Int unsigned_arc_counter = 0;
    
    // Each arc will appear in two faces.
    face_arcs        = Tensor1<Int,Int> ( 2 * arc_count         );
    face_arc_ptr     = Tensor1<Int,Int> ( 2 * initial_arc_count ); // TODO: Refine this upper bound.
    face_arc_ptr[0]  = 0;
    Int face_counter = 0;
    
    // Data for forming the graph components.
    // Each arc will appear in precisely one component.
    comp_arcs        = Tensor1<Int,Int> ( arc_count );
    comp_arc_ptr     = Tensor1<Int,Int> ( arc_count );   // TODO: Refine this upper bound.
    comp_arc_ptr[0]  = 0;
    Int comp_counter = 0;

    // A stack for storing arcs of the current component that have yet to be visited.
    std::vector<Int> comp_arc_stack;
    
    comp_arc_stack.reserve( initial_arc_count );
    
    Int arc_finder = 0;
    
    while( arc_finder < initial_arc_count )
    {
        // Start a new graph component.
        
        
        while( (arc_finder < initial_arc_count) && (A_faces(arc_finder,0) != -1) )
        {
            ++arc_finder;
        }
        
        if( arc_finder >= initial_arc_count )
        {
            goto exit;
        }
        
        // Push
        comp_arc_stack.push_back(arc_finder);

        while( !comp_arc_stack.empty() )
        {
            // Pop
            Int a = comp_arc_stack.back();
            comp_arc_stack.pop_back();
            
            Int dir;
            
            {
                const Int e [2] = { A_faces(a,0), A_faces(a,1) };
                
                PD_assert( (e[0] >= -1) || (e[1] >= -1) );
                
                if( e[0] == -1 )
                {
                    dir = true;
                }
                else if(e[1] == -1)
                {
                    dir = false;
                }
                else
                {
                    // a was already treated and we can just go on with popping the comp_arc_stack.
                    continue;
                }
            }
            
            Int starting_arc = a;
            
            do
            {
                if( dir )
                {
                    // Declare arc a to be member of the current graph component.
                    // We don't want to count arcs twice, so we do this only if dir == true;
                    comp_arcs[unsigned_arc_counter] = a;
                    
                    arc_comp[a] = comp_counter;
                    
                    ++unsigned_arc_counter;
                }

                // Declare current face to be a face of this arc.
                A_faces(a,!dir) = face_counter;
                
                // Declare this arc to belong to the current face.
                face_arcs[arc_counter] = a;
                
                // Remember this arc to find the other face later.
                if( A_faces(a,dir) == -1 )
                {
                    // Push
                    comp_arc_stack.push_back(a);
                }
                
                // Move to next arc.
                std::tie(a,dir) = NextLeftArc( a, dir );
                ++arc_counter;
            }
            while( a != starting_arc );
            
            ++face_counter;
            face_arc_ptr[face_counter] = arc_counter;
        }
    
        ++comp_counter;
        comp_arc_ptr[comp_counter] = unsigned_arc_counter;
    }
    
exit:
        
    face_arc_ptr.template Resize<true>(face_counter+1);
    comp_arc_ptr.template Resize<true>(comp_counter+1);
    
    faces_initialized = true;
}

const Tensor1<Int,Int> & FaceArcs()
{
    RequireFaces();
    
    return face_arcs;
}

const Tensor1<Int,Int> & FaceArcPointers()
{
    RequireFaces();
    
    return face_arc_ptr;
}

const Tensor1<Int,Int> & ComponentArcs()
{
    RequireFaces();
    
    return comp_arcs;
}

const Tensor1<Int,Int> & ComponentArcPointers()
{
    RequireFaces();
    
    return comp_arc_ptr;
}

const Tensor2<Int,Int> ArcFaces()
{
    RequireFaces();
    
    return A_faces;
}

const Tensor1<Int,Int> & ArcComponents()
{
    RequireFaces();
    
    return arc_comp;
}

const Tensor2<Int,Int> ArcFacesData()
{
    RequireFaces();
    
    Tensor2<Int,Int> dual_arc_data (arc_count, 3); // Convention: Left face, right face, arc.
    
    Int arc_counter = 0;
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        const Int e [2] = { A_faces(a,0), A_faces(a,1) };
        
        if( e[0] >= 0 )
        {
            dual_arc_data(arc_counter,0) = e[0];
            dual_arc_data(arc_counter,1) = e[1];
            dual_arc_data(arc_counter,2) = a;
            ++arc_counter;
        }
    }
    
    return dual_arc_data;
}


const Tiny::VectorList<2,Int,Int> DuplicateDualArcs()
{
    RequireFaces();
    
    // It is tempting to use a Tensor2 here, but that disallows the use of ThreeArraySort below!
    Tiny::VectorList<3,Int,Int> dual_arc_data (arc_count); // Convention: Left face first.
    
    mptr<Int> d [3] = { dual_arc_data.data(0), dual_arc_data.data(1), dual_arc_data.data(2) };

    std::vector<Int> first_arc;
    std::vector<Int> second_arc;
    
    Int counter = 0;

    for( Int a = 0; a < initial_arc_count; ++a )
    {
        const Int e [2] = { A_faces(a,0), A_faces(a,1) };
        
        if( e[0] >= 0 )
        {
            d[0][counter] = e[0];
            d[1][counter] = e[1];
            d[2][counter] = a;
            ++counter;
        }
    }

    TwoArraySort<Int,Int,Size_T> Q2;
    ThreeArraySort<Int,Int,Int,Size_T> Q3;

    Q3( d[0], d[1], d[2], arc_count);
    
    //Now e[0] is sorted.
    //Continue to sort parts of e[1] that belong to ties in e[0];

    Int a = 0;
    Int b = 0;
    Int d_0_a = d[0][a] ;

    while( a < arc_count )
    {
        while( (b < arc_count) && (d[0][b] == d_0_a) )
        {
            ++b;
        }
        
        Q2( &d[1][a], &d[2][a], b-a );
        
//        for( Int i = a; i < b-1; ++i )
//        {
//            if( d[1][i] == d[1][i+1])
//            {
//                first_arc.push_back (d[2][i  ]);
//                second_arc.push_back(d[2][i+1]);
//            }
//        }
        
        // Only take first duplicate of that face because I cannot handle all of them at the same time, yet.
        if( a < b-1 )
        {
            if( d[1][a] == d[1][a+1])
            {
                first_arc.push_back (d[2][a  ]);
                second_arc.push_back(d[2][a+1]);
            }
        }
        
        a = b;
        d_0_a = d[0][a] ;
    }
    
    Tiny::VectorList<2,Int,Int> result ( static_cast<Int>(first_arc.size()) );
    
    if( first_arc.size() > 0 )
    {
        result[0].Read( &first_arc [0] );
        result[1].Read( &second_arc[0] );
    }
    return result;
    
//    return dual_arc_data;
}
