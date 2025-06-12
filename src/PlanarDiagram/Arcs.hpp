public:

/*!
 * @brief This tells us whether a giving arc goes over the crossing at the indicated end.
 *
 *  @param a The index of the arc in equations.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcUnderQ( const Int a )  const
{
    AssertArc(a);

    const Int c = A_cross(a,headtail);

    AssertCrossing(c);
    
    // Tail == 0 == Out
    // Head == 1 == In
    // const side = C_arcs(c_0,headtail,Right) == a ? Right : Left;
//            const bool side = (C_arcs(c,headtail,Right) == a);
    
//            // Long version of code for documentation and debugging.
//            if( headtail == Head )
//            {
//
//                PD_ASSERT( A_cross(a,Head) == c );
//                PD_ASSERT( C_arcs(c,In,side) == a );
//
//                return (side == CrossingRightHandedQ(c));
//
    
                /* (side == Right) && CrossingRightHandedQ(c)
                 *
                 *         O     O
                 *          ^   ^
                 *           \ /
                 *            / c
                 *           / \
                 *          /   \
                 *         O     O
                 *                ^
                 *                 \
                 *                  \ a
                 *                   \
                 *                    X
                 *
                 *  (side == Left) && CrossingLeftHandedQ(c)
                 *
                 *         O     O
                 *          ^   ^
                 *           \ /
                 *            \ c
                 *           / \
                 *          /   \
                 *         O     O
                 *        ^
                 *       /
                 *      / a
                 *     /
                 *    X
                 */
    
//            }
//            else // if( headtail == Tail )
//            {
//                PD_ASSERT( A_cross(a,Tail) == c );
//                PD_ASSERT( C_arcs(c,Out,side) == a );
//
//                return (side == CrossingLeftHandedQ(c));
//
    
                /* Positive cases:
                 *
                 * (side == Right) && CrossingLeftHandedQ(c)
                 *
                 *
                 *                   ^
                 *                  /
                 *                 / a
                 *                /
                 *         O     O
                 *          ^   ^
                 *           \ /
                 *            \ c
                 *           / \
                 *          /   \
                 *         O     O
                 *
                 * (side == Left) && CrossingRightHandedQ(c)
                 *
                 *     ^
                 *      \
                 *       \ a
                 *        \
                 *         O     O
                 *          ^   ^
                 *           \ /
                 *            / c
                 *           / \
                 *          /   \
                 *         O     O
                 */
    
//            }
    
//            // Short version of code for performance.
    return (
        (C_arcs(c,headtail,Right) == a) == ( headtail == CrossingRightHandedQ(c) )
    );
}

bool ArcUnderQ( const Int a, const bool headtail )  const
{
    AssertArc(a);

    const Int c = A_cross(a,headtail);

    AssertCrossing(c);
    
    return (
        (C_arcs(c,headtail,Right) == a) == ( headtail == CrossingRightHandedQ(c) )
    );
}

/*!
 * @brief This tells us whether a giving arc goes under the crossing at the indicated end.
 *
 *  @param a The index of the arc in equations.
 *
 *  @tparam headtail Boolean that indicates whether the relation should be computed for the crossing at the head of `a` (`headtail == true`) or at the tail (`headtail == false`).
 */

template<bool headtail>
bool ArcOverQ( const Int a ) const
{
    return !ArcUnderQ<headtail>(a);
}

bool ArcOverQ( const Int a, const bool headtail )  const
{
    return !ArcUnderQ(a,headtail);
}

bool AlternatingQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (ArcOverQ<Tail>(a) == ArcOverQ<Head>(a)) )
        {
            return false;
        }
    }

    return true;
}


/*!
 * @brief Returns the arc following arc `a` by going to the crossing at the head of `a` and then turning left.
 */

Arrow_T NextLeftArc( const Int a, const bool d ) const
{
    // TODO: Signed indexing does not work because of 0!
    
    AssertArc(a);
    
    const Int c = A_cross(a,d);
    
    AssertCrossing(c);
    
    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( d == Head )
    {
        // Using `C_arcs(c,In ,Left ) != a` instead of `C_arcs(c,In ,Right) == a` gives us a 50% chance that we do not have to read any index again.

        const Int b = C_arcs(c,In,Left);
        
        if( b != a )
        {
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             *   b     a
             */

            return Arrow_T(b,Tail);
        }
        else // if( b == a )
        {
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             * a == b
             */

            return Arrow_T(C_arcs(c,Out,Left),Head);
        }
    }
    else // if( a_dir == Tail )
    {
        // Also here we can make it so that we have to read for a second time only in 50% of the cases.

        const Int b = C_arcs(c,Out,Right);
        
        if( b != a )
        {
            /*   a     b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return Arrow_T(b,Head);
        }
        else // if( b == a )
        {
            /*      a == b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return Arrow_T(C_arcs(c,In,Right),Tail);
        }
    }
}

Int NextLeftArc( const Int da ) const
{
    const Int c = A_cross.data()[da];

    auto [a,d] = FromDarc(da);
    
    // It might seem a bit weird, but on my Apple M1 these conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( d == Head )
    {
        // We exploit a 50% chance that we do not have to read any index again.
        
//        const Int db = ToDarc(C_arcs(c,In,Left),Head);
        const Int b = C_arcs(c,In,Left);
        
        if( b != a )
        {
            /*    O     O
             *     ^   ^
             *      \ /
             *       X c
             *      / \
             *     /   \
             *    O     O
             *    b     a
             */
             
            return ToDarc<Tail>(b);
        }
        else // if( db == da )
        {
            /*    O     O
             *     ^   ^
             *      \ /
             *       X c
             *      / \
             *     /   \
             *    O     O
             * a == b
             */

            return ToDarc<Head>(C_arcs(c,Out,Left));
        }
    }
    else // if( headtail == Tail )
    {
        // We exploit a 50% chance that we do not have to read any index again.

//        const Int db = ToDarc(C_arcs(c,Out,Right),Tail);
        const Int b = C_arcs(c,Out,Right);
        
        if( b != a )
        {
            /*   a     b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return ToDarc<Head>(b);
        }
        else // if( b == a )
        {
            /*       a == b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return ToDarc<Tail>(C_arcs(c,In,Right));
        }
    }
}


bool CheckNextLeftArc() const
{
    bool passedQ = true;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        {
            const Int da = ToDarc(a,Tail);
            
            const Int db = NextLeftArc(da);
            
            auto [b,dir] = NextLeftArc(a,Tail);
            
            passedQ = passedQ && (db == ToDarc(b,dir));
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextLeftArc failed at " + ArcString(a) + " (Tail).");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(b);
                TOOLS_DUMP(dir);
                return false;
            }
        }
        
        {
            const Int da = ToDarc(a,Head);
            
            const Int db = NextLeftArc(da);
            
            auto [b,dir] = NextLeftArc(a,Head);
            
            passedQ = passedQ && (db == ToDarc(b,dir) );
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextLeftArc failed at " + ArcString(a) + " (Head).");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(b);
                TOOLS_DUMP(dir);
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
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             *   a     b
             */

            return Arrow_T(b,Tail);
        }
        else // if( b == a )
        {
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             *       a == b
             */

            return Arrow_T(C_arcs(c,Out,Right),Head);
        }
    }
    else // if( headtail == Tail )
    {
        // Also here we can make it so that we have to read for a second time only in 50% of the cases.

        const Int b = C_arcs(c,Out,Left);

        if( b != a )
        {
            /*   b     a
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return Arrow_T(b,Head);
        }
        else // if( b == a )
        {
            /* a == b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return Arrow_T(C_arcs(c,In,Left),Tail);
        }
    }
}

Int NextRightArc( const Int da ) const
{
    const Int c = A_cross.data()[da];
    
    // It might seem a bit weird, but on my Apple M1 this conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    auto [a,d] = FromDarc(da);
    
    if( d == Head )
    {
        // We exploit a 50% chance that we do not have to read any index again.

        const Int b = C_arcs(c,In,Right);

        if( b != a )
        {
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             *   a     b
             */

            return ToDarc<Tail>(b);
        }
        else // if( a == b )
        {
            /*   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             *       a == b
             */

            return ToDarc<Head>(C_arcs(c,Out,Right));
        }
    }
    else
    {
        // We exploit a 50% chance that we do not have to read any index again.

        const Int b = C_arcs(c,Out,Left);

        if( b != a )
        {
            /*   b     a
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return ToDarc<Head>(b);
        }
        else // if( b == a )
        {
            /* a == b
             *   O     O
             *    ^   ^
             *     \ /
             *      X c
             *     / \
             *    /   \
             *   O     O
             */

            return ToDarc<Tail>(C_arcs(c,In,Left));
        }
    }
}


mref<ArcContainer_T> ArcLeftArc() const
{
    // Return value needs to be mutable so that StrandSimplifier can update it.
    
    std::string tag ("ArcLeftArc");
    
    if( !this->InCacheQ(tag) )
    {
        ArcContainer_T A_left ( max_arc_count );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                Int A [2][2];
                
                copy_buffer<4>( C_arcs.data(c), &A[0][0] );
                
                const Int arrows [2][2] =
                {
                    {
                        ToDarc<Head>(A[Out][Left ]),
                        ToDarc<Head>(A[Out][Right])
                    },{
                        ToDarc<Tail>(A[In ][Left ]),
                        ToDarc<Tail>(A[In ][Right])
                    }
                };
                
                
                /* A[Out][Left ]         A[Out][Right]
                 *               O     O
                 *                ^   ^
                 *                 \ /
                 *                  X c
                 *                 ^ ^
                 *                /   \
                 *               O     O
                 * A[In ][Left ]         A[In ][Right]
                 */
                
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
//            else
//            {
//                A_left(A[Out][Left ],Tail) = Uninitialized;
//
//                A_left(A[Out][Right],Tail) = Uninitialized;
//
//                A_left(A[In ][Left ],Head) = Uninitialized;
//
//                A_left(A[In ][Right],Head) = Uninitialized;
//            }
        }
        
        this->SetCache(tag,std::move(A_left));
    }
    
    return this->GetCache<ArcContainer_T>(tag);
}


bool CheckNextRightArc() const
{
    bool passedQ = true;
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        {
            const Int da = ToDarc<Tail>(a);
            
            const Int db = NextRightArc(da);
            
            auto [b,d] = NextRightArc(a,Tail);
            
            
            
            passedQ = passedQ && (db == ToDarc(b,d));
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextRightArc failed at " + ArcString(a) + " (Tail).");
                
                TOOLS_DUMP(db);
                TOOLS_DUMP(b);
                TOOLS_DUMP(d);
                return false;
            }
        }
        
        {
            const Int da = ToDarc<Head>(a);
            
            const Int db = NextRightArc(da);
            
            auto [b,d] = NextRightArc(a,Head);
            
            passedQ = passedQ && (db == ToDarc(b,d));
            
            if( !passedQ )
            {
                eprint(ClassName()+"::CheckNextRightArc failed at " + ArcString(a) + " (Head).");
                
                TOOLS_DUMP(db);
                TOOLS_DUMP(b);
                TOOLS_DUMP(d);
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

//Tensor3<Int,Int> ArcWings() const
//{
//    Tensor3<Int,Int> A_wings ( max_arc_count, 2, 2 );
//    
//    for( Int c = 0; c < max_crossing_count; ++c )
//    {
//        if( CrossingActiveQ(c) )
//        {
//            Int A [2][2];
//            
//            copy_buffer<4>( C_arcs.data(c), &A[0][0] );
//            
//            
//            constexpr Int tail_mask = 0;
//            constexpr Int head_mask = 1;
//            
//            const Int arrows [2][2] =
//            {
//                {
//                    ToDarc<Head>(A[Out][Left ]),
//                    ToDarc<Head>(A[Out][Right])
//                },{
//                    ToDarc<Tail>(A[In ][Left ]),
//                    ToDarc<Tail>(A[In ][Right])
//                }
//            };
//            
//            
//            /* A[Out][Left ]         A[Out][Right]
//             *               O     O
//             *                ^   ^
//             *                 \ /
//             *                  X c
//             *                 ^ ^
//             *                /   \
//             *               O     O
//             * A[In ][Left ]         A[In ][Right]
//             */
//             
//            A_wings(A[Out][Left ],Tail,Left ) = arrows[In ][Left ];
//            A_wings(A[Out][Left ],Tail,Right) = arrows[Out][Right];
//            
//            A_wings(A[Out][Right],Tail,Left ) = arrows[Out][Left ];
//            A_wings(A[Out][Right],Tail,Right) = arrows[In ][Right];
//            
//            A_wings(A[In ][Left ],Head,Left ) = arrows[Out][Left ];
//            A_wings(A[In ][Left ],Head,Right) = arrows[In ][Right];
//            
//            A_wings(A[In ][Right],Head,Left ) = arrows[In ][Left ];
//            A_wings(A[In ][Right],Head,Right) = arrows[Out][Right];
//            
//            PD_ASSERT(
//                (A_wings(A[In ][Left ],Head,Left ) >> 1)
//                ==
//                NextLeftArc(A[In ][Left ],Head).first
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[In ][Left ],Head,Left ) & Int(1))
//                ==
//                NextLeftArc(A[In ][Left ],Head).second
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[In ][Right],Head,Left ) >> 1)
//                ==
//                NextLeftArc(A[In ][Right],Head).first
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[In ][Right],Head,Left ) & Int(1))
//                ==
//                NextLeftArc(A[In ][Right],Head).second
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[Out][Left ],Tail,Right) >> 1)
//                ==
//                NextLeftArc(A[Out][Left ],Tail).first
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[Out][Left ],Tail,Right) & Int(1))
//                ==
//                NextLeftArc(A[Out][Left ],Tail).second
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[Out][Right],Tail,Right) >> 1)
//                ==
//                NextLeftArc(A[Out][Right],Tail).first
//            );
//            
//            PD_ASSERT(
//                (A_wings(A[Out][Right],Tail,Right) & Int(1))
//                ==
//                NextLeftArc(A[Out][Right],Tail).second
//            );
//        }
//    }
//    
//    return A_wings;
//}


cref<Tensor1<Int,Int>> ArcNextArc() const
{
    std::string tag ("ArcNextArc");
    
    if( !this->InCacheQ(tag) )
    {
        Tensor1<Int,Int> A_next ( max_arc_count, Uninitialized );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                A_next(C_arcs(c,In,Left )) = C_arcs(c,Out,Right);
                A_next(C_arcs(c,In,Right)) = C_arcs(c,Out,Left );
            }
        }
        
        this->SetCache(tag,std::move(A_next));
    }
    
    return this->GetCache<Tensor1<Int,Int>>(tag);
}

Tensor1<Int,Int> ArcPrevArc() const
{
    std::string tag ("ArcPrevArc");
    
    if( !this->InCacheQ(tag) )
    {
        Tensor1<Int,Int> A_prev ( max_arc_count, Uninitialized );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                A_prev(C_arcs(c,Out,Right)) = C_arcs(c,In,Left );
                A_prev(C_arcs(c,Out,Left )) = C_arcs(c,In,Right);
            }
        }
        
        this->SetCache(tag,std::move(A_prev));
    }
    
    return this->GetCache<Tensor1<Int,Int>>(tag);
}
