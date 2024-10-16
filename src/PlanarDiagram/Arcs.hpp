public:

/*!
 * @brief Returns the arc following arc `a` by going to the crossing at the head of `a` and then turning left.
 */

Arrow_T NextLeftArc( const Int a, const bool headtail ) const
{
    // TODO: Signed indexing does not work because of 0!
    
    AssertArc(a);
    
    const Int c = A_cross(a,headtail);
    
    AssertCrossing(c);
    
    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( headtail == Head )
    {
        // Using `C_arcs(c,In ,Left ) != a` instead of `C_arcs(c,In ,Right) == a` gives us a 50% chance that we do not have to read any index again.

        const Int b = C_arcs(c,In,Left);
        
        if( b != a )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //   b     a

            return Arrow_T(b,Tail);
        }
        else // if( b == a )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            // a == b

            return Arrow_T(C_arcs(c,Out,Left),Head);
        }
    }
    else // if( headtail == Tail )
    {
        // Also here we can make it so that we have to read for a second time only in 50% of the cases.

        const Int b = C_arcs(c,Out,Right);
        
        if( b != a )
        {
            //   a     b
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return Arrow_T(b,Head);
        }
        else // if( b == a )
        {
            //      a == b
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return Arrow_T(C_arcs(c,In,Right),Tail);
        }
    }
}

Int NextLeftArc( const Int A ) const
{
    const Int c = A_cross.data()[A];

    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( (A & Int(1)) == Head )
    {
        // We exploit a 50% chance that we do not have to read any index again.
        
        const Int B = (C_arcs(c,In,Left) << 1) | Int(Head);
        
        if( B != A )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //   B     A

            return B ^ Int(1);
        }
        else // if( B == A )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            // A == B

            return (C_arcs(c,Out,Left) << 1) | Int(Head);
        }
    }
    else // if( headtail == Tail )
    {
        // We exploit a 50% chance that we do not have to read any index again.

        const Int B = (C_arcs(c,Out,Right) << 1) | Int(Tail);
        
        if( B != A )
        {
            //   A     B
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return B ^ Int(1);
        }
        else // if( B == A )
        {
            //      A == B
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return (C_arcs(c,In,Right) << 1) | Int(Tail);
        }
    }
}


bool CheckNextLeftArc() const
{
    bool passedQ = true;
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        {
            const Int A = (a << 1) | Int(Tail);
            
            const Int B = NextLeftArc(A);
            
            auto [b,dir] = NextLeftArc(a,Tail);
            
            passedQ = passedQ && ( b == (B >> 1) ) && ( dir == (B & Int(1)) );
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextLeftArc failed at " + ArcString(a) + " (Tail).");
                
                dump(a);
                dump(A);
                dump(B);
                dump(B >> 1);
                dump(B & Int(1));
                dump(b);
                dump(dir);
                return false;
            }
        }
        
        {
            const Int A = (a << 1) | Int(Head);
            
            const Int B = NextLeftArc(A);
            
            auto [b,dir] = NextLeftArc(a,Head);
            
            passedQ = passedQ && ( b == (B >> 1) ) && ( dir == (B & Int(1) ) );
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextLeftArc failed at " + ArcString(a) + " (Head).");
                
                dump(a);
                dump(A);
                dump(B);
                dump(B >> 1);
                dump(B & Int(1));
                dump(b);
                dump(dir);
                return false;
            }
        }
    }
    
    if( passedQ )
    {
        logprint(ClassName()+"::CheckNextLeftArc passed.");
    }
    
    return passedQ;
}


/*!
 * @brief Returns the arc following arc `a` by going to the crossing at the head of `a` and then turning right.
 */

Arrow_T NextRightArc( const Int a, const bool headtail ) const
{
    // TODO: Signed indexing does not work because of 0!
    
    AssertArc(a);
    
    const Int c = A_cross(a,headtail);
    
    AssertCrossing(c);
    
    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( headtail == Head )
    {
        // Using `C_arcs(c,In ,Left ) != a` instead of `C_arcs(c,In ,Right) == a` gives us a 50% chance that we do not have to read any index again.

        const Int b = C_arcs(c,In,Right);

        if( b != a )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //   a     b

            return Arrow_T(b,Tail);
        }
        else // if( b == a )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //       a == b

            return Arrow_T(C_arcs(c,Out,Right),Head);
        }
    }
    else // if( headtail == Tail )
    {
        // Also here we can make it so that we have to read for a second time only in 50% of the cases.

        const Int b = C_arcs(c,Out,Left);

        if( b != a )
        {
            //   b     a
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return Arrow_T(b,Head);
        }
        else // if( b == a )
        {
            // a == b
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return Arrow_T(C_arcs(c,In,Left),Tail);
        }
    }
}

Int NextRightArc( const Int A ) const
{
    const Int c = A_cross.data()[A];
    
    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( (A & Int(1)) == Head )
    {
        // We exploit a 50% chance that we do not have to read any index again.

        const Int B = (C_arcs(c,In,Right) << 1) | Int(Head);

        if( B != A )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //   A     B

            return B ^ Int(1);
        }
        else // if( A == B )
        {
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O
            //       A == B

            return (C_arcs(c,Out,Right) << 1) | Int(Head);
        }
    }
    else
    {
        // We exploit a 50% chance that we do not have to read any index again.

        const Int B = (C_arcs(c,Out,Left) << 1) | Int(Tail);

        if( B != A )
        {
            //   B     A
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return B ^ Int(1);
        }
        else // if( B == A )
        {
            // A == B
            //   O     O
            //    ^   ^
            //     \ /
            //      X c
            //     / \
            //    /   \
            //   O     O

            return (C_arcs(c,In,Left) << 1) | Int(Tail);
        }
    }
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



bool CheckNextRightArc() const
{
    bool passedQ = true;
    
    for( Int a = 0; a < initial_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        {
            const Int A = (a << 1) | Int(Tail);
            
            const Int B = NextRightArc(A);
            
            auto [b,dir] = NextRightArc(a,Tail);
            
            passedQ = passedQ && ( b == (B >> 1) ) && ( dir == (B & Int(1) ) );
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextRightArc failed at " + ArcString(a) + " (Tail).");
                
                dump(B);
                dump(b);
                dump(dir);
                return false;
            }
        }
        
        {
            const Int A = (a << 1) | Int(Head);
            
            const Int B = NextRightArc(A);
            
            auto [b,dir] = NextRightArc(a,Head);
            
            passedQ = passedQ && ( b == (B >> 1) ) && ( dir == (B & Int(1) ) );
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextRightArc failed at " + ArcString(a) + " (Head).");
                
                dump(B);
                dump(b);
                dump(dir);
                return false;
            }
        }
    }
    
    if( passedQ )
    {
        logprint(ClassName()+"::CheckNextRightArc passed.");
    }
    
    return passedQ;
}







/*!
 * @brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing at the head/tail of `a`.
 */

template<bool headtail>
Int NextArc( const Int a ) const
{
    return NextArc<headtail>(a,A_cross(a,headtail));
}

/*!
 * @brief Returns the arc next to arc `a`, i.e., the arc reached by going straight through the crossing `c` at the head/tail of `a`. This function exploits that `c` is already known; so it saves one memory lookup.
 */

template<bool headtail>
Int NextArc( const Int a, const Int c ) const
{
    AssertArc(a);
    AssertCrossing(c);
    
    PD_ASSERT( A_cross(a,headtail) == c );

    // We leave through the arc at the port opposite to where a arrives.
    const bool side = (C_arcs(c, headtail,Right) != a);
    
    const Int a_next = C_arcs(c,!headtail,side );
    
    AssertArc(a_next);
    
    return a_next;
}


/*!
 * @brief Returns the ArcView object next to ArcView object `A`, i.e., the ArcView object reached by going straight through the crossing at the head/tail of `A`.
 */

template<bool headtail>
ArcView NextArc( const ArcView & A )
{
    AssertArc(A.Idx());
    
    const Int c = A(headtail);

    AssertCrossing(c);

    // We leave through the arc at the port opposite to where a arrives.
    const bool side = (C_arcs(c,headtail,Right) != A.Idx());

    return GetArc( C_arcs(c,!headtail,side) );
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
