public:

static constexpr Int ToDarc( const Int a, const HeadTail_T d )
{
    return Int(2) * a + d;
}

static constexpr std::pair<Int,HeadTail_T> FromDarc( Int da )
{
    return std::pair( da / Int(2), da % Int(2) );
}

static constexpr Int FlipDarc( const Int da )
{
    return da ^ Int(1);
}

std::string DarcString( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    
    return "darc " + Tools::ToString(da) + " = { "
        + Tools::ToString(A_cross.data()[FlipDarc(da)]) + ", "
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
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int  c    = A_cross(a,d);
    const bool side = ArcSide(a,d);
    AssertCrossing(c);
    
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,side,!d);
    AssertArc(b);
    
    return ToDarc(b,!side);
}

Int LeftDarc_Reference( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int c = A_cross(a,d);
    AssertCrossing(c);
    
    const bool side = ArcSide_Reference(a,d);
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,side,!d);
    AssertArc(b);
    
    return ToDarc(b,!side);
}

mref<ArcContainer_T> ArcLeftDarc() const
{
    // Return value needs to be mutable so that StrandSimplifier can update it.
    
    std::string tag ("ArcLeftDarc");
    
    if( !this->InCacheQ(tag) )
    {
        TOOLS_PTIMER(timer,tag);
        
        ArcContainer_T A_left_buffer ( max_arc_count );
        
        mptr<Int> dA_left = A_left_buffer.data();

        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                Int A [2][2];
                
                copy_buffer<4>( C_arcs.data(c), &A[0][0] );
                
                const Int in_darcs [2][2] =
                {
                    {
                        ToDarc(A[Out][Left ],Tail), ToDarc(A[Out][Right],Tail)
                    },{
                        ToDarc(A[In ][Left ],Head), ToDarc(A[In ][Right],Head)
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
                
                dA_left[ in_darcs[Out][Left ] ] = FlipDarc( in_darcs[Out][Right] );
                dA_left[ in_darcs[Out][Right] ] = FlipDarc( in_darcs[In ][Right] );
                dA_left[ in_darcs[In ][Left ] ] = FlipDarc( in_darcs[Out][Left ] );
                dA_left[ in_darcs[In ][Right] ] = FlipDarc( in_darcs[In ][Left ] );

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


bool CheckLeftDarc() const
{
    std::string tag = MethodName("CheckLeftDarc");
    
    TOOLS_PTIMER(timer,tag);
    
    mptr<Int> dA_left = ArcLeftDarc().data();
    
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
            const Int dc_slow = LeftDarc_Reference(da);
            
            const bool passedQ = (db == dc);
            
            if( !passedQ )
            {
                eprint(tag + " failed at " + ArcString(a) + " ("  + (headtail ? "Head" : "Tail") + ").");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(dc);
                TOOLS_DUMP(dc / Int(2));
                TOOLS_DUMP(dc % Int(2));
                TOOLS_DUMP(dc_slow);
                TOOLS_DUMP(dc_slow / Int(2));
                TOOLS_DUMP(dc_slow % Int(2));
                return false;
            }
        }
    }
    
    logprint(tag + " passed.");
    
    return true;
}


//#########################################################
//###       RightDarc
//#########################################################

Int RightDarc( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int  c    = A_cross(a,d);
    const bool side = ArcSide(a,d);
    AssertCrossing(c);
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,!side,d);
    AssertArc(b);
    
    return ToDarc(b,side);
}

Int RightDarc_Reference( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int c = A_cross(a,d);
    AssertCrossing(c);
    
    const bool side = ArcSide_Reference(a,d);
    
    /*
     *   O     O    d    == Head  == Right
     *    ^   ^     side == Right == In == Head
     *     \ /
     *      X c
     *     / \
     *    /   \
     *   O     O a
     */
    
    const Int b = C_arcs(c,!side,d);
    AssertArc(b);
    
    return ToDarc(b,side);
}


mref<ArcContainer_T> ArcRightDarc() const
{
    // Return value needs to be mutable so that StrandSimplifier can update it.
    
    std::string tag ("ArcRightArc");
    
    if( !this->InCacheQ(tag) )
    {
        ArcContainer_T A_right_buffer ( max_arc_count );
        
        mptr<Int> dA_right = A_right_buffer.data();
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                Int A [2][2];
                
                copy_buffer<4>( C_arcs.data(c), &A[0][0] );
                
                const Int in_darcs [2][2] =
                {
                    {
                        ToDarc(A[Out][Left ],Tail), ToDarc(A[Out][Right],Tail)
                    },{
                        ToDarc(A[In ][Left ],Head), ToDarc(A[In ][Right],Head)
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

                dA_right[ in_darcs[Out][Left ] ] = FlipDarc( in_darcs[In ][Left ] );
                dA_right[ in_darcs[Out][Right] ] = FlipDarc( in_darcs[Out][Left ] );
                dA_right[ in_darcs[In ][Left ] ] = FlipDarc( in_darcs[In ][Right] );
                dA_right[ in_darcs[In ][Right] ] = FlipDarc( in_darcs[Out][Right] );
                
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
    std::string tag = MethodName("RightDarc");
    
    TOOLS_PTIMER(timer,tag);
    
    cptr<Int> dA_right = ArcRightDarc().data();
    
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
            const Int dc_slow = RightDarc_Reference(da);
            
            const bool passedQ = (db == dc) && (db == dc_slow);
            
            if( !passedQ )
            {
                eprint(tag + " failed at " + ArcString(a) + " ("  + (headtail ? "Head" : "Tail") + ").");
                
                TOOLS_DUMP(a);
                TOOLS_DUMP(da);
                TOOLS_DUMP(db);
                TOOLS_DUMP(db / Int(2));
                TOOLS_DUMP(db % Int(2));
                TOOLS_DUMP(dc);
                TOOLS_DUMP(dc / Int(2));
                TOOLS_DUMP(dc % Int(2));
                TOOLS_DUMP(dc_slow);
                TOOLS_DUMP(dc_slow / Int(2));
                TOOLS_DUMP(dc_slow % Int(2));
                return false;
            }
        }
    }
    
    logprint(tag + " passed.");
    
    return true;
}
