public:


struct SimplifyPasses_Args
{
    Int  max_dist              = Scalar::Max<Int>;
    bool overQ                 = true;
    bool compressQ             = true;
    Int  compression_threshold = 100;
};

friend std::string ToString( cref<SimplifyPasses_Args> args )
{
    return std::string("{ ")
         +   "max_dist = " + ToString(args.max_dist)
         + ", overQ = " + ToString(args.overQ)
         + ", compressQ = " + ToString(args.compressQ)
         + ", compression_threshold = " + ToString(args.compression_threshold)
         + " }";
}

struct SimplifyPasses_TArgs
{
    bool restart_after_successQ = true;
    bool restart_after_failureQ = true;
    bool restart_walk_backQ     = true;
    bool interleave_over_underQ = true;
    bool R_II_blockingQ         = true;
    bool R_II_forwardQ          = false;
};

friend std::string ToString( cref<SimplifyPasses_TArgs> targs )
{
    return "{ restart_after_successQ = " + ToString(targs.restart_after_successQ)
         + ", restart_after_failureQ = " + ToString(targs.restart_after_failureQ)
         + ", restart_walk_backQ = " + ToString(targs.restart_walk_backQ)
         + ", interleave_over_underQ = " + ToString(targs.interleave_over_underQ)
         + ", R_II_blockingQ = " + ToString(targs.R_II_blockingQ)
         + ", R_II_forwardQ = " + ToString(targs.R_II_forwardQ)
         + " }";
}


// TODO: If a loops is removed that does not finish a component, we can try to restart as well.

/*!@brief This is the main routine of the class. It is supposed to reroute all over/understrands to shorter strands, if possible. It does so by traversing the diagram and looking for over/understrand. When a complete strand is detected, it runs Dijkstra's algorithm in the dual graph of the diagram _without the currect strand_. If a shorter path is detected, the strand is rerouted. The returned integer is a rough(!) indicator of how many changes accoured. 0 is returned only of no changes have been made and if there is no need to call this function again. A positive value indicates that it would be worthwhile to call this function again (maybe after some further simplifications).
 *
 *
 */

template<SimplifyPasses_TArgs targs>
Size_T SimplifyPasses( mref<PD_T> pd_input, cref<SimplifyPasses_Args> args )
{
    [[maybe_unused]] auto tag = [this]() { return this->MethodName("SimplifyPasses"); };
    
    TOOLS_PTIMER(timer,tag());
    
#ifdef TOOLS_ENABLE_PROFILER
    logvalprint("targs",ToString(targs));
    logvalprint("args ",ToString(args) );
#endif

    if( args.max_dist <= Int(0) ) { return 0; }
    
    PD_PRINT(tag()+ ": Initial number of crossings = " + ToString(pd_input.CrossingCount()) );
    
    if( args.compressQ ) { pd_input.ConditionalCompress(args.compression_threshold); }

    if constexpr ( !targs.interleave_over_underQ )
    {
        SetStrandMode(args.overQ);
    }
    LoadDiagram(pd_input);
    NewStrand();
    
    Int a_ptr = 0;
    change_counter   = 0;

    const Int pd_max_arc_count = pd->MaxArcCount();
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
    while( a_ptr < pd_max_arc_count )
    {
        // Search for next arc that is active and has not yet been handled.
        while(
            ( a_ptr < pd_max_arc_count ) && (!ArcActiveQ(a_ptr) || ArcRecentlyMarkedQ(a_ptr) )
        )
        {
            ++a_ptr;
        }

        if( a_ptr >= pd_max_arc_count ) [[unlikely]]
        {
            PD_PRINT("Breaking many loop because we marked all active arcs; we ar done for this round.");
            break;
        }
        
        PD_VALPRINT("a_ptr",a_ptr);
        
        Pass_T pass;
        
        if constexpr ( targs.interleave_over_underQ )
        {
            const bool overQ_0 = ArcOverQ(a_ptr,Tail);
            const bool overQ_1 = ArcOverQ(a_ptr,Head);
            
            SetStrandMode(overQ_1);
            NewStrand();
            
            if( overQ_0 != overQ_1 )
            {
                /* Definitely the start of an (overQ_1 ? over : under)-strand.
                 * Bad idea to walk backwards.
                 *
                 *          |        |
                 *          | a_ptr  |
                 *  ------->|------->-------->
                 *          |        |
                 *          |        |
                 */
                
                FindPass<false,targs.R_II_blockingQ,targs.R_II_forwardQ>(pass, a_ptr, overQ, current_mark);
            }
            else
            {
                /* Somewhere in the middle of an (overQ_1 ? over : under)-strand.
                 * We can (should?) move back to the start.
                 *
                 *          |        |
                 *          | a_ptr  |
                 *  ------->-------->-------->
                 *          |        |
                 *          |        |
                 */
                
                if constexpr ( targs.restart_walk_backQ )
                {
                    FindPass<true,targs.R_II_blockingQ,targs.R_II_forwardQ>(
                        pass, NextArc(a_ptr,Tail), overQ, current_mark
                    );
                }
                else
                {
                    FindPass<false,targs.R_II_blockingQ,targs.R_II_forwardQ>(pass, a_ptr, overQ, current_mark);
                }
            }
        }
        else
        {
            NewStrand();
            FindPass<true,targs.R_II_blockingQ,targs.R_II_forwardQ>(pass, a_ptr, overQ, current_mark);
        }
        
        
        PD_VALPRINT("pass",ToString(pass));
        PD_VALPRINT("pass",PassString(pass));
        
        while( pass.activeQ )
        {
            PD_ASSERT(CheckPass(pass,true));
            
            // Now try to reroute!
            bool changedQ = false;
            
            if( (pass.arc_count > Int(2)) && (args.max_dist > Int(0)) )
            {
#ifdef PD_DEBUG
                Int C_0 = pd->crossing_count;
//                pd->PrintInfo();
#endif // PD_DEBUG
                
                changedQ = RerouteToShortestPath(
                    pass,
                    Min(static_cast<Int>(pass.arc_count-Int(1)),args.max_dist),
                    path_0
                );
#ifdef PD_DEBUG
                Int C_1 = pd->crossing_count;
                PD_ASSERT( !changedQ || (C_1 < C_0) );
#endif // PD_DEBUG
            }
            
            if( changedQ )
            {
                if constexpr( targs.restart_after_successQ )
                {
                    PD_PRINT("Attempting to start new strand after successful rerouting.");
                }
                else
                {
                    PD_PRINT("Breaking after successful rerouting because of targs.restart_after_successQ == false.");
                    break;
                }
                
                // We may try to start at the and of the strand, but `pass.next` must be active for this.
                if( !ArcActiveQ(pass.next) ) [[unlikely]]
                {
                    PD_PRINT("Breaking because " + ArcString(pass.next)+ " is inactive.");
                    break; // Search new arc by incrementing a_ptr.
                }
//                
//                // Caution: setting current arc to _new_ last arc of strand.
//                pass.last = pd->NextArc(pass.next,Tail);
            }
            else
            {
                if constexpr( targs.restart_after_failureQ )
                {
                    PD_PRINT("Attempting to start new strand after aborted rerouting.");
                }
                else
                {
                    PD_PRINT("Breaking after aborted rerouting because of targs.restart_after_failureQ == false.");
                    break;
                }
            }
            
            PD_ASSERT(ArcActiveQ(pass.last));
            PD_ASSERT(pass.last == pd->NextArc(pass.next,Tail));
            PD_ASSERT(pd->ArcOverQ(pass.last,Head) != overQ);
            PD_ASSERT(pd->ArcOverQ(pass.next,Tail) != overQ);
            
            // Rerouting arcs that we have been marked recently is a bad idea.
            if( ArcRecentlyMarkedQ(pass.next) ) [[unlikely]]
            {
                PD_PRINT("Breaking because " + ArcString(pass.next)+ " has been visited recently.");
                break; // Search new arc by incrementing a_ptr.
            }
            
            if constexpr ( targs.interleave_over_underQ )
            {
                // We start a strand of opposite type.
                SetStrandMode(!overQ);
                PD_PRINT("Changed strand type to " + OverQString() + "strand.");
                NewStrand();
                FindPass<targs.restart_walk_backQ, targs.R_II_blockingQ, targs.R_II_forwardQ>(
                    pass, pass.last, overQ, current_mark
                );
            }
            else // if constexpr ( !targs.interleave_over_underQ )
            {
                PD_PRINT("Strand type remains at " + OverQString() + "strand.");
                NewStrand();
                FindPass< targs.restart_walk_backQ, targs.R_II_blockingQ, targs.R_II_forwardQ>(
                    pass, pass.next, overQ, current_mark
                );
            }
        }
        
        PD_PRINT("Searching for new arc by incrementing a_ptr.");
        // Go back to top and start a new strand.
        ++a_ptr;
    }
    
    PD_VALPRINT("pd->crossing_count",pd->crossing_count);
    
    Cleanup();
    
    if( pd_input.ValidQ() && (pd_input.CrossingCount() <= Int(1)) )
    {
        pdc.CreateUnlink( pd_input.LastColorDeactivated() );
        pd_input = PD_T::InvalidDiagram();
    }
    
    return change_counter;
    
} // Int SimplifyPasses



// TODO: Add fobidden and ignored mark.
bool RerouteToShortestPath( mref<Pass_T> pass, const Int max_dist, mref<Path_T> path  )
{
    PD_TIMER(timer,MethodName("RerouteToShortestPath"));
    
    PD_VALPRINT("change_counter",change_counter);
    
    PD_ASSERT(pd->CheckAll());
    PD_ASSERT(CheckPass(pass,true));
    
    
    // We don't like loops of any kind here.
    PD_ASSERT(pd->A_cross(pass.first,Tail) != pd->A_cross(pass.first,Head));
    PD_ASSERT(pd->A_cross(pass.last ,Tail) != pd->A_cross(pass.last ,Head));
    PD_ASSERT(pd->A_cross(pass.first,Tail) != pd->A_cross(pass.last ,Tail));
    PD_ASSERT(pd->A_cross(pass.first,Tail) != pd->A_cross(pass.last ,Head));
    PD_ASSERT(pd->A_cross(pass.first,Head) != pd->A_cross(pass.last ,Head));
    
#ifdef PD_DEBUG
    if( pd->A_cross(pass.first,Head) == pd->A_cross(pass.last ,Tail) )
    {
        TOOLS_LOGDUMP(pass.arc_count);
        TOOLS_LOGDUMP(CountArcsInRange(pass.first,pass.last));
        logvalprint("pass",ShortArcRangeString(pass.first,pass.last));
    }
#endif // PD_DEBUG
    
    FindShortestPath( pass.first, pass.last, max_dist, path );
    
#ifdef PD_COUNTERS
    RecordPreStrandSize(pass.arc_count);
    RecordPostStrandSize(path_arc_count);
#endif
    
    if( (path.Size() <= Int(0)) || (path.CrossingCount()) >= pass.CrossingCount() )
    {
        PD_DPRINT("No improvement detected. (path.CrossingCount() = " + ToString(path.CrossingCount()) + ", pass.CrossingCount() = " + ToString(pass.CrossingCount()) + ", max_dist = " + ToString(max_dist) + ")");
        return false;
    }
    
    bool successQ = Reroute(pass,path);
    
    return successQ;
}
