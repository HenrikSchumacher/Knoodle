public:

static constexpr Int ToDarc( const Int a, const HeadTail_T d )
{
    return Int(2) * a + d;
}

static constexpr std::pair<Int,HeadTail_T> FromDarc( Int da )
{
    return std::pair( da / Int(2), da % Int(2) );
}

static constexpr Int ReverseDarc( const Int da )
{
    return da ^ Int(1);
}

std::string DarcString( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    
    return "darc " + Tools::ToString(da) + " = { "
        + Tools::ToString(A_cross.data()[ReverseDarc(da)]) + ", "
        + Tools::ToString(A_cross.data()[da]) + " } ("
        + ToString(A_state[a]) + ")";
}

Int DarcCrossing( const Int da ) const
{
    return A_cross.data()[da];
}

bool DarcSide( const Int da )  const
{
    auto [a,d] = FromDarc(da);
    return ArcSide(a,d);
}

//#########################################################
//###       LeftDarc
//#########################################################

Int LeftDarc( const Int da ) const
{
    const Int c = A_cross.data()[da];

    auto [a,d] = FromDarc(da);
    
    // It might seem a bit weird, but on my Apple M1 these conditional ifs are _faster_ than computing the Booleans to index into C_arcs and doing the indexing then. The reason must be that the conditionals have a 50% chance to prevent loading a second entry from C_arcs.
    
    if( d == Head )
    {
        // We exploit a 50% chance that we do not have to read any index again.

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
             
            return ToDarc(b,Tail);
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

            return ToDarc(C_arcs(c,Out,Left),Head);
        }
    }
    else // if( headtail == Tail )
    {
        // We exploit a 50% chance that we do not have to read any index again.

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

            return ToDarc(b,Head);
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

            return ToDarc(C_arcs(c,In,Right),Tail);
        }
    }
}

mref<ArcContainer_T> ArcLeftDarcs() const
{
    // Return value needs to be mutable so that StrandSimplifier can update it.
    
    std::string tag ("ArcLeftDarcs");
    
    if( !this->InCacheQ("ArcLeftDarcs") )
    {
        TOOLS_PTIMER(timer,MethodName(tag));
        
        ArcContainer_T A_left_buffer ( max_arc_count );
        
        mptr<Int> dA_left = A_left_buffer.data();

        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                const C_Arcs_T C = CopyCrossing(c);
                const C_Arcs_T in_darcs {
                    { ToDarc(C[Out][Left ],Tail), ToDarc(C[Out][Right],Tail) },
                    { ToDarc(C[In ][Left ],Head), ToDarc(C[In ][Right],Head) }
                };
                
                /* C[Out][Left ]         C[Out][Right]
                 *               O     O
                 *                ^   ^
                 *                 \ /
                 *                  X c
                 *                 ^ ^
                 *                /   \
                 *               O     O
                 * C[In ][Left ]         C[In ][Right]
                 */
                
                dA_left[ in_darcs[Out][Left ] ] = ReverseDarc( in_darcs[Out][Right] );
                dA_left[ in_darcs[Out][Right] ] = ReverseDarc( in_darcs[In ][Right] );
                dA_left[ in_darcs[In ][Left ] ] = ReverseDarc( in_darcs[Out][Left ] );
                dA_left[ in_darcs[In ][Right] ] = ReverseDarc( in_darcs[In ][Left ] );
                
                PD_ASSERT( dA_left[ in_darcs[Out][Left ] ] == LeftDarc( in_darcs[Out][Left ] ) );
                PD_ASSERT( dA_left[ in_darcs[Out][Right] ] == LeftDarc( in_darcs[Out][Right] ) );
                PD_ASSERT( dA_left[ in_darcs[In ][Left ] ] == LeftDarc( in_darcs[In ][Left ] ) );
                PD_ASSERT( dA_left[ in_darcs[In ][Right] ] == LeftDarc( in_darcs[In ][Right] ) );
            }
        }
        
        this->SetCache(tag,std::move(A_left_buffer));
    }
    
    return this->GetCache<ArcContainer_T>(tag);
}


bool CheckArcLeftDarcs() const
{
    [[maybe_unused]] auto tag = [](){ return MethodName("CheckArcLeftDarcs"); };
    
    TOOLS_PTIMER(timer,tag());
    
    mptr<Int> dA_left = ArcLeftDarcs().data();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        for( bool headtail : {false, true} )
        {
            const Int da      = ToDarc(a,headtail);
            const Int db      = dA_left[da];
            const Int dc      = LeftDarc(da);
            
            const bool passedQ = (db == dc);
            
            if( !passedQ )
            {
                eprint(tag() + " failed at " + ArcString(a) + " ("  + (headtail ? "Head" : "Tail") + ").");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(dc);
                TOOLS_DUMP(dc / Int(2));
                TOOLS_DUMP(dc % Int(2));
                return false;
            }
        }
    }
    
    logprint(tag() + " passed.");
    
    return true;
}


//#########################################################
//###       RightDarc
//#########################################################

Int RightDarc( const Int da ) const
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

            return ToDarc(b,Tail);
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

            return ToDarc(C_arcs(c,Out,Right),Head);
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

            return ToDarc(b,Head);
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

            return ToDarc(C_arcs(c,In,Left),Tail);
        }
    }
}

mref<ArcContainer_T> ArcRightDarcs() const
{
    // Return value needs to be mutable so that StrandSimplifier can update it.
    
    std::string tag ("ArcRightArcs");
    
    if( !this->InCacheQ(tag) )
    {
        ArcContainer_T A_right_buffer ( max_arc_count );
        
        mptr<Int> dA_right = A_right_buffer.data();
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                const C_Arcs_T C = CopyCrossing(c);
                const C_Arcs_T in_darcs  {
                    { ToDarc(C[Out][Left ],Tail), ToDarc(C[Out][Right],Tail) },
                    { ToDarc(C[In ][Left ],Head), ToDarc(C[In ][Right],Head) }
                };
                
                /* C[Out][Left ]         C[Out][Right]
                 *               O     O
                 *                ^   ^
                 *                 \ /
                 *                  X c
                 *                 ^ ^
                 *                /   \
                 *               O     O
                 * C[In ][Left ]         C[In ][Right]
                 */

                dA_right[ in_darcs[Out][Left ] ] = ReverseDarc( in_darcs[In ][Left ] );
                dA_right[ in_darcs[Out][Right] ] = ReverseDarc( in_darcs[Out][Left ] );
                dA_right[ in_darcs[In ][Left ] ] = ReverseDarc( in_darcs[In ][Right] );
                dA_right[ in_darcs[In ][Right] ] = ReverseDarc( in_darcs[Out][Right] );
                
                PD_ASSERT( dA_right[ in_darcs[Out][Left ] ] == RightDarc( in_darcs[Out][Left ] ) );
                PD_ASSERT( dA_right[ in_darcs[Out][Right] ] == RightDarc( in_darcs[Out][Right] ) );
                PD_ASSERT( dA_right[ in_darcs[In ][Left ] ] == RightDarc( in_darcs[In ][Left ] ) );
                PD_ASSERT( dA_right[ in_darcs[In ][Right] ] == RightDarc( in_darcs[In ][Right] ) );
            }
        }
        
        this->SetCache(tag,std::move(A_right_buffer));
    }
    
    return this->GetCache<ArcContainer_T>(tag);
}

bool CheckRightDarc() const
{
    std::string tag ("RightDarc");
    
    TOOLS_PTIMER(timer,MethodName(tag));
    
    cptr<Int> dA_right = ArcRightDarcs().data();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        for( bool headtail : {false, true} )
        {
            const Int da      = ToDarc(a,headtail);
            const Int db      = dA_right[da];
            const Int dc      = RightDarc(da);
            
            const bool passedQ = (db == dc);
            
            if( !passedQ )
            {
                eprint(MethodName(tag) + " failed at " + ArcString(a) + " ("  + (headtail ? "Head" : "Tail") + ").");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(dc);
                TOOLS_DUMP(dc / Int(2));
                TOOLS_DUMP(dc % Int(2));
                return false;
            }
        }
    }
    
    logprint(tag + " passed.");
    
    return true;
}
