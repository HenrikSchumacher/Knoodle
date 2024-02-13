Int ComponentCount()
{
    return comp_ptr.Size()-1;
}

Int FaceCount()
{
    return face_ptr.Size()-1;
}

void RequireFaces()
{
    if( faces_initialized )
    {
        return;
    }
    
    // Maybe not required, but it would be nice of each arc can tell in which component it lies.
    arc_comp = Tensor1<Int,Int>( initial_arc_count );
    
    // These are going to become edge of the dual graph(s). One dual edge for each arc.
    arc_faces = Tiny::VectorList<2,Int,Int>( initial_arc_count );
    
    mptr<Int> A_faces [2] = { arc_faces.data(0), arc_faces.data(1) };
    // Convention: Left face first:
    //
    //            arc_faces[1][a]
    //
    //            <------------|  a
    //
    //            arc_faces[0][a]
    //
    // Entry -1 means "unvisited but to be visited".
    // Entry -2 means "do not visited".
    
    for( Int a = 0; a < initial_arc_count; ++ a )
    {
        if( ArcActive(a) )
        {
            A_faces[0][a] = -1;
            A_faces[1][a] = -1;
        }
        else
        {
            A_faces[0][a] = -2;
            A_faces[1][a] = -2;
        }
    }
    
    Int          arc_counter = 0;
    Int unsigned_arc_counter = 0;
    
    // Each arc will appear in two faces.
    face_arcs = Tensor1<Int,Int> ( 2 * arc_count         );
    face_ptr  = Tensor1<Int,Int> ( 2 * initial_arc_count ); // TODO: Refine this upper bound.
    face_ptr[0] = 0;
    Int face_counter = 0;
    
    // Data for forming the graph components.
    // Each arc will appear in precisely one component.
    comp_arcs = Tensor1<Int,Int> ( arc_count );
    comp_ptr  = Tensor1<Int,Int> ( arc_count );   // TODO: Refine this upper bound.
    comp_ptr[0] = 0;
    Int comp_counter = 0;

    // A stack for storing arcs of the current component that have yet to be visited.
    std::vector<Int> comp_arc_stack;
    
    Int arc_finder = 0;
    
    while( arc_finder < initial_arc_count )
    {
        // Start a new graph component.
        
        while( (arc_finder < initial_arc_count) && (A_faces[0][arc_finder] != -1) )
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
            
//                    valprint("a",a);
            
            Int dir;
            
            PD_assert( (A_faces[0][a] >= -1) || (A_faces[1][a] >= -1) );

            
            if( A_faces[0][a] == -1 )
            {
                dir = true;
            }
            else if(A_faces[1][a] == -1)
            {
                dir = false;
            }
            else
            {
                // a was already treated and we can just go on with popping the comp_arc_stack.
                continue;
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
                A_faces[!dir][a] = face_counter;
            
                // Declare this arc to belong to the current face.
                face_arcs[arc_counter] = a;
                
//                        // Beware: Dirty hack to store sign of arc w.r.t. to face.
//                        face_arcs[arc_counter] = dir ? (a+1) : -(a+1);
                
//                        // Return the tail.
//                        faces_crossings[arc_counter] = A_cross(a,!dir);
                
                // Remember this arc to find the other face later.
                if( A_faces[dir][a] == -1 )
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
            face_ptr[face_counter] = arc_counter;
        }
    
        ++comp_counter;
        comp_ptr[comp_counter] = unsigned_arc_counter;
    }
    
exit:
        
    face_ptr.Resize(face_counter+1);
    comp_ptr.Resize(comp_counter+1);
    
    faces_initialized = true;
    
//            print("faces = " + ToString(faces));
//            print("face_ptr = " + ToString(face_ptr));
//
//            print("comps = " + ToString(comps));
//            print("comp_ptr = " + ToString(comp_ptr));
    
//            print("A_faces[0] = " + ToString(arc_faces[0]));
//            print("A_faces[1] = " + ToString(arc_faces[1]));
}

const Tensor1<Int,Int> & FaceArcs()
{
    RequireFaces();
    
    return face_arcs;
}

const Tensor1<Int,Int> & FacePointers()
{
    RequireFaces();
    
    return face_ptr;
}

const Tensor1<Int,Int> & ComponentArcs()
{
    RequireFaces();
    
    return comp_arcs;
}

const Tensor1<Int,Int> & ComponentPointers()
{
    RequireFaces();
    
    return comp_ptr;
}

const Tiny::VectorList<2,Int,Int> ArcFaces()
{
    RequireFaces();
    
    return arc_faces;
}

const Tensor1<Int,Int> & ArcComponents()
{
    RequireFaces();
    
    return arc_comp;
}

const Tiny::VectorList<3,Int,Int> DualArcData()
{
    RequireFaces();
    
    Tiny::VectorList<3,Int,Int> dual_arc_data (arc_count); // Convention: Left face first.
    
    cptr<Int> A_faces [2] = { arc_faces.data(0), arc_faces.data(1) };
    
    mptr<Int> e [3] = { dual_arc_data.data(0), dual_arc_data.data(1), dual_arc_data.data(2) };
    
    Int counter = 0;
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        if( A_faces[0][a] >= 0 )
        {
            e[0][counter] = A_faces[0][a];
            e[1][counter] = A_faces[1][a];
            e[2][counter] = a;
            ++counter;
        }
    }
    
    return dual_arc_data;
}


const Tiny::VectorList<2,Int,Int> DuplicateDualArcs()
{
    RequireFaces();

    Tiny::VectorList<3,Int,Int> dual_arc_data (arc_count); // Convention: Left face first.
    
    cptr<Int> A_faces [2] = { arc_faces.data(0), arc_faces.data(1) };
    
    mptr<Int> e [3] = { dual_arc_data.data(0), dual_arc_data.data(1), dual_arc_data.data(2) };

    std::vector<Int> first_arc;
    std::vector<Int> second_arc;
    
    Int counter = 0;

    for( Int a = 0; a < initial_arc_count; ++a )
    {
        if( A_faces[0][a] >= 0 )
        {
            e[0][counter] = A_faces[0][a];
            e[1][counter] = A_faces[1][a];
            e[2][counter] = a;
            ++counter;
        }
    }

    TwoArraySort<Int,Int,Size_T> Q2;
    ThreeArraySort<Int,Int,Int,Size_T> Q3;

    Q3( e[0], e[1], e[2], arc_count);
    
    //Now e[0] is sorted.
    //Continue to sort parts of e[1] that belong to ties in e[0];

    Int a = 0;
    Int b = 0;
    Int e_0_a = e[0][a] ;

    while( a < arc_count )
    {
        while( (b < arc_count) && (e[0][b] == e_0_a) )
        {
            ++b;
        }
        
        Q2( &e[1][a], &e[2][a], b-a );
        
//        for( Int i = a; i < b-1; ++i )
//        {
//            if( e[1][i] == e[1][i+1])
//            {
//                first_arc.push_back (e[2][i  ]);
//                second_arc.push_back(e[2][i+1]);
//            }
//        }
        
        // Only take first duplicate of that face because I cannot handle all of them at the same time, yet.
        if( a < b-1 )
        {
            if( e[1][a] == e[1][a+1])
            {
                first_arc.push_back (e[2][a  ]);
                second_arc.push_back(e[2][a+1]);
            }
        }
        
        a = b;
        e_0_a = e[0][a] ;
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
