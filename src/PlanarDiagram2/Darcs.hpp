public:

    
// TODO: These things would be way faster if Int is unsigned.

static constexpr std::pair<Int,HeadTail_T> FromDarc( Int da )
{
    return std::pair( da / Int(2), da % Int(2) );
}

static constexpr Int ToDarc( const Int a, const HeadTail_T d )
{
    return Int(2) * a + d;
}

template<HeadTail_T d>
static constexpr Int ToDarc( const Int a )
{
    return Int(2) * a + d;
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

//#########################################################
//###       LeftDarc
//#########################################################

Int LeftDarc( const Int da ) const
{
    auto [a,d] = FromDarc(da);
    AssertArc(a);
    
    const Int c = A_cross(a,d);
    AssertCrossing(c);
    
    const bool side = A_state[a].Side(d);
    
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
        
        ArcContainer_T A_left ( max_arc_count );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                Int A [2][2];
                
                copy_buffer<4>( C_arcs.data(c), &A[0][0] );
                
                const Int darcs [2][2] =
                {
                    {
                        ToDarc<Head>(A[Out][Left ]), ToDarc<Head>(A[Out][Right])
                    },{
                        ToDarc<Tail>(A[In ][Left ]), ToDarc<Tail>(A[In ][Right])
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
                
                A_left(A[Out][Left ],Tail) = darcs[Out][Right];
                A_left(A[Out][Right],Tail) = darcs[In ][Right];
                A_left(A[In ][Left ],Head) = darcs[Out][Left ];
                A_left(A[In ][Right],Head) = darcs[In ][Left ];
                
                PD_ASSERT( A_left(A[Out][Left ],Tail) == LeftDarc(darcs[Out][Left ]) );
                PD_ASSERT( A_left(A[Out][Right],Tail) == LeftDarc(darcs[Out][Right]) );
                PD_ASSERT( A_left(A[In ][Left ],Head) == LeftDarc(darcs[In ][Left ]) );
                PD_ASSERT( A_left(A[In ][Right],Head) == LeftDarc(darcs[In ][Right]) );
            }
        }
        
        this->SetCache(tag,std::move(A_left));
    }
    
    return this->GetCache<ArcContainer_T>(tag);
}


bool CheckLeftDarc() const
{
    std::string tag = MethodName("CheckLeftDarc");
    
    TOOLS_PTIMER(timer,tag);
    
    cptr<Int> darc_left_darc = ArcLeftDarc().data();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        for( bool headtail : {false, true} )
        {
            const Int da = ToDarc(a,headtail);
            const Int db = darc_left_darc[da];
            const Int dc = LeftDarc(dc);
            auto [c,dir] = NextLeftArc(a,headtail);
            
            const bool passedQ = (db == dc) && (db == ToDarc(c,dir));
            
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
                TOOLS_DUMP(c);
                TOOLS_DUMP(dir);
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
    
    const Int c = A_cross(a,d);
    AssertCrossing(c);
    
    const bool side = A_state[a].Side(d);
    
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
        ArcContainer_T A_right ( max_arc_count );
        
        for( Int c = 0; c < max_crossing_count; ++c )
        {
            if( CrossingActiveQ(c) )
            {
                Int A [2][2];
                
                copy_buffer<4>( C_arcs.data(c), &A[0][0] );
                
                const Int darcs [2][2] =
                {
                    {
                        ToDarc<Head>(A[Out][Left ]), ToDarc<Head>(A[Out][Right])
                    },{
                        ToDarc<Tail>(A[In ][Left ]), ToDarc<Tail>(A[In ][Right])
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

                A_right(A[Out][Left ],Tail) = darcs[In ][Left ];
                A_right(A[Out][Right],Tail) = darcs[Out][Left ];
                A_right(A[In ][Left ],Head) = darcs[In ][Right];
                A_right(A[In ][Right],Head) = darcs[Out][Right];
                
                PD_ASSERT( A_right(A[Out][Left ],Tail) == RightDarc(darcs[Out][Left ]) );
                PD_ASSERT( A_right(A[Out][Right],Tail) == RightDarc(darcs[Out][Right]) );
                PD_ASSERT( A_right(A[In ][Left ],Head) == RightDarc(darcs[In ][Left ]) );
                PD_ASSERT( A_right(A[In ][Right],Head) == RightDarc(darcs[In ][Right]) );
            }
        }
        
        this->SetCache(tag,std::move(A_right));
    }
    
    return this->GetCache<ArcContainer_T>(tag);
}

bool CheckNextRightArc() const
{
    std::string tag = MethodName("CheckLeftDarc");
    
    TOOLS_PTIMER(timer,tag);
    
    cptr<Int> darc_right_darc = ArcRightDarc().data();
    
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) )
        {
            continue;
        }
        
        for( bool headtail : {false, true} )
        {
            const Int da = ToDarc(a,headtail);
            const Int db = darc_right_darc(da);
            const Int dc = RightDarc(dc);
            auto [c,dir] = NextRightArc(a,headtail);
            
            const bool passedQ = (db == dc) && (db == ToDarc(c,dir));
            
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
                TOOLS_DUMP(c);
                TOOLS_DUMP(dir);
                return false;
            }
        }
    }
    
    logprint(tag + " passed.");
    
    return true;
}
