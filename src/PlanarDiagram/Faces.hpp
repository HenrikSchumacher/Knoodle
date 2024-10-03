public:

Int FaceCount()
{
    RequireFaces();
    
    return F_arc_ptr.Size()-1;
}

std::string FaceString( const Int f ) const
{
//    RequireFaces();
    
    const Int i_begin = F_arc_ptr[f  ];
    const Int i_end   = F_arc_ptr[f+1];
    
    return std::string("face ") + ToString(f) + " = " + ArrayToString( &F_arc_idx[i_begin], { i_end - i_begin } );
}

const Tensor1<Int,Int> & FaceArcIndices()
{
    RequireFaces();
    
    return F_arc_idx;
}

const Tensor1<Int,Int> & FaceArcPointers()
{
    RequireFaces();
    
    return F_arc_ptr;
}

const Tensor2<Int,Int> ArcFaces()
{
    RequireFaces();
    
    return A_faces;
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



Tensor3<Int,Int> ArcWings() const
{
    Tensor3<Int,Int> A_wings ( initial_arc_count, 2, 2 );
    
    
    for( Int c = 0; c < initial_crossing_count; ++c )
    {
        if( CrossingActiveQ(c) )
        {
            Int A [2][2];
            
            copy_buffer<4>( C_arcs.data(c), &A[0][0] );
            
            
            constexpr Int tail_mask = 0;
            constexpr Int head_mask = 1;
            
            const Int arrows [2][2] =
            {
                {
                    (A[Out][Left ] << 1) | head_mask,
                    (A[Out][Right] << 1) | head_mask
                },{
                    (A[In ][Left ] << 1) | tail_mask,
                    (A[In ][Right] << 1) | tail_mask
                }
            };
            
            
            // A[Out][Left ]         A[Out][Right]
            //               O     O
            //                ^   ^
            //                 \ /
            //                  X c
            //                 ^ ^
            //                /   \
            //               O     O
            // A[In ][Left ]         A[In ][Right]
            
            A_wings(A[Out][Left ],Tail,Left ) = arrows[In ][Left ];
            A_wings(A[Out][Left ],Tail,Right) = arrows[Out][Right];
            
            A_wings(A[Out][Right],Tail,Left ) = arrows[Out][Left ];
            A_wings(A[Out][Right],Tail,Right) = arrows[In ][Right];
            
            A_wings(A[In ][Left ],Head,Left ) = arrows[Out][Left ];
            A_wings(A[In ][Left ],Head,Right) = arrows[In ][Right];
            
            A_wings(A[In ][Right],Head,Left ) = arrows[In ][Left ];
            A_wings(A[In ][Right],Head,Right) = arrows[Out][Right];
            
//#ifdef PD_DEBUG
//            
//            if( (A_wings(A[In ][Left ],Head,Left ) >> 1)
//               !=
//               NextLeftArc(A[In ][Left ],Head).first
//            )
//            {
//                dump(A[In ][Left ]);
//                
//                dump((A_wings(A[In ][Left ],Head,Left ) >> 1));
//                
//                dump(NextLeftArc(A[In ][Left ],Head).first);
//            }
//#endif
            
            PD_ASSERT(
                (A_wings(A[In ][Left ],Head,Left ) >> 1)
                ==
                NextLeftArc(A[In ][Left ],Head).first
            );
            
            PD_ASSERT(
                (A_wings(A[In ][Left ],Head,Left ) & Int(1))
                ==
                NextLeftArc(A[In ][Left ],Head).second
            );
            
            PD_ASSERT(
                (A_wings(A[In ][Right],Head,Left ) >> 1)
                ==
                NextLeftArc(A[In ][Right],Head).first
            );
            
            PD_ASSERT(
                (A_wings(A[In ][Right],Head,Left ) & Int(1))
                ==
                NextLeftArc(A[In ][Right],Head).second
            );
            
            PD_ASSERT(
                (A_wings(A[Out][Left ],Tail,Right) >> 1)
                ==
                NextLeftArc(A[Out][Left ],Tail).first
            );
            
            PD_ASSERT(
                (A_wings(A[Out][Left ],Tail,Right) & Int(1))
                ==
                NextLeftArc(A[Out][Left ],Tail).second
            );
            
            PD_ASSERT(
                (A_wings(A[Out][Right],Tail,Right) >> 1)
                ==
                NextLeftArc(A[Out][Right],Tail).first
            );
            
            PD_ASSERT(
                (A_wings(A[Out][Right],Tail,Right) & Int(1))
                ==
                NextLeftArc(A[Out][Right],Tail).second
            );
        }
    }
    
    return A_wings;
}

Tiny::MatrixList<2,2,Int,Int> ArcWings2() const
{
    Tiny::MatrixList<2,2,Int,Int> A_wings ( initial_arc_count );
    
    for( Int c = 0; c < initial_crossing_count; ++c )
    {
        if( CrossingActiveQ(c) )
        {
            Int A [2][2];
            
            copy_buffer<4>( C_arcs.data(c), &A[0][0] );
            
            // A[Out][Left ]         A[Out][Right]
            //               O     O
            //                ^   ^
            //                 \ /
            //                  X c
            //                 ^ ^
            //                /   \
            //               O     O
            // A[In ][Left ]         A[In ][Right]
            
            
            constexpr Int tail_mask = 0;
            constexpr Int head_mask = 1;
            
            const Int arrow [2][2] =
            {
                {
                    (A[Out][Left ] << 1) | head_mask,
                    (A[Out][Right] << 1) | head_mask
                },{
                    (A[In ][Left ] << 1) | tail_mask,
                    (A[In ][Right] << 1) | tail_mask
                }
            };
            
            A_wings(Tail,Left ,A[Out][Left ]) = arrow[In ][Left ];
            A_wings(Tail,Right,A[Out][Left ]) = arrow[Out][Right];
            
            A_wings(Tail,Left ,A[Out][Right]) = arrow[Out][Left ];
            A_wings(Tail,Right,A[Out][Right]) = arrow[In ][Right];
            
            A_wings(Head,Left ,A[In ][Left ]) = arrow[Out][Left ];
            A_wings(Head,Right,A[In ][Left ]) = arrow[In ][Right];
            
            A_wings(Head,Left ,A[In ][Right]) = arrow[In ][Left ];
            A_wings(Head,Right,A[In ][Right]) = arrow[Out][Right];
            
            
            PD_ASSERT( (A_wings(Head,Left ,A[In ][Left ]) >> 1)
                !=
                NextLeftArc(A[In ][Left ],Head).first
            );
            
            PD_ASSERT( (A_wings(Head,Left ,A[In ][Left ]) & Int(1))
                !=
                NextLeftArc(A[In ][Left ],Head).second
            );
            
            PD_ASSERT( (A_wings(Head,Left ,A[In ][Right]) >> 1)
                !=
                NextLeftArc(A[In ][Right],Head).first
            );
            
            PD_ASSERT( (A_wings(Head,Left ,A[In ][Right]) & Int(1))
                !=
                NextLeftArc(A[In ][Right],Head).second
            );
            
            PD_ASSERT( (A_wings(Tail,Right,A[Out][Left ]) >> 1)
                !=
                NextLeftArc(A[Out][Left ],Tail).first
            );
            
            PD_ASSERT( (A_wings(Tail,Right,A[Out][Left ]) & Int(1))
                !=
                NextLeftArc(A[Out][Left ],Tail).second
            );
            
            PD_ASSERT( (A_wings(Tail,Right,A[Out][Right]) >> 1)
                !=
                NextLeftArc(A[Out][Right],Tail).first
            );
            
            PD_ASSERT( (A_wings(Tail,Right,A[Out][Right]) & Int(1))
                ==
                NextLeftArc(A[Out][Right],Tail).second
            );
        }
    }
    
    return A_wings;
}


void RequireFaces()
{
    if( faces_initialized )
    {
        return;
    }

    ptic(ClassName()+"::RequireFaces");
    
    auto A_wings = ArcWings();

    // These are going to become edges of the dual graph(s). One dual edge for each arc.
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
    
    // Each arc will appear in two faces.
    F_arc_idx     = Tensor1<Int,Int> ( 2 * arc_count         );
    F_arc_ptr     = Tensor1<Int,Int> ( 2 * initial_arc_count ); // TODO: Refine this upper bound.
    F_arc_ptr[0]  = 0;
    Int F_counter = 0;

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
            
            const Int e [2] = { A_faces(a,0), A_faces(a,1) };
            
            PD_ASSERT( (e[0] >= -1) || (e[1] >= -1) );
            
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
            
            Int starting_arc = a;
            
            do
            {
                // Declare current face to be a face of this arc.
                A_faces(a,!dir) = F_counter;
                
                // Declare this arc to belong to the current face.
                F_arc_idx[arc_counter] = a;
                
                // Remember this arc to find the other face later.
                if( A_faces(a,dir) == -1 )
                {
                    // Push
                    comp_arc_stack.push_back(a);
                }

                
//                // Move to next arc.
                
                std::tie(a,dir) = NextLeftArc( a, dir );
                
//                // Using the precomputed A_wings instead of NextLeftArc seems to be faster.
//                const Int bits = A_wings(a,dir,!dir);
//                
//                a   = static_cast<Int>(bits >> 1);
//                dir = static_cast<Int>(bits & Int(1));
                
                ++arc_counter;
            }
            while( a != starting_arc );
            
            ++F_counter;
            
            F_arc_ptr[F_counter] = arc_counter;
        }
    }
    
exit:
        
    F_arc_ptr.template Resize<true>(F_counter+1);
    
    faces_initialized = true;
    
    ptoc(ClassName()+"::RequireFaces");
}
