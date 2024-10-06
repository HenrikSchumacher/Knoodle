public:

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

Tensor2<Int,Int> ArcLeftArc() const
{
    Tensor2<Int,Int> A_left ( initial_arc_count, 2 );
    
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
            
            A_left(A[Out][Left ],Tail) = arrows[Out][Right];
            
            A_left(A[Out][Right],Tail) = arrows[In ][Right];
            
            A_left(A[In ][Left ],Head) = arrows[Out][Left ];
            
            A_left(A[In ][Right],Head) = arrows[In ][Left ];
            
            
            PD_ASSERT(
                (A_left(A[In ][Left ],Head) >> 1)
                ==
                NextLeftArc(A[In ][Left ],Head).first
            );
            
            PD_ASSERT(
                (A_left(A[In ][Left ],Head) & Int(1))
                ==
                NextLeftArc(A[In ][Left ],Head).second
            );
            
            PD_ASSERT(
                (A_left(A[In ][Right],Head) >> 1)
                ==
                NextLeftArc(A[In ][Right],Head).first
            );
            
            PD_ASSERT(
                (A_left(A[In ][Right],Head) & Int(1))
                ==
                NextLeftArc(A[In ][Right],Head).second
            );
            
            PD_ASSERT(
                (A_left(A[Out][Left ],Tail) >> 1)
                ==
                NextLeftArc(A[Out][Left ],Tail).first
            );
            
            PD_ASSERT(
                (A_left(A[Out][Left ],Tail) & Int(1))
                ==
                NextLeftArc(A[Out][Left ],Tail).second
            );
            
            PD_ASSERT(
                (A_left(A[Out][Right],Tail) >> 1)
                ==
                NextLeftArc(A[Out][Right],Tail).first
            );
            
            PD_ASSERT(
                (A_left(A[Out][Right],Tail) & Int(1))
                ==
                NextLeftArc(A[Out][Right],Tail).second
            );
        }
    }
    
    return A_left;
}

Tensor2<Int,Int> ArcLeftArc_slow() const
{
    Tensor2<Int,Int> A_left ( initial_arc_count, 2 );
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        auto aa = NextLeftArc(a,Tail);
        A_left(a,Tail) = (aa.first << 1) | Int(aa.second);
        auto bb = NextLeftArc(a,Head);
        A_left(a,Head) = (bb.first << 1) | Int(bb.second);
    }
    
    return A_left;
}
