public:

Size_T RattleIterations() const
{
    return rattle_counter;
}

double RattleTiming() const
{
    return rattle_timing;
}

template<bool verboseQ = false, typename ExtInt>
std::vector<PD_T> Rattle( cref<PD_T> pd, const ExtInt target_iter )
{
    static_assert(IntQ<ExtInt>,"");
    std::vector<PD_T> input;
    input.push_back(pd); // Using copy constructor here!
    
    return this->template Rattle<verboseQ>(input,target_iter);
}

template<bool verboseQ = false, typename ExtInt>
std::vector<PD_T> Rattle( PD_T && pd, const ExtInt target_iter )
{
    static_assert(IntQ<ExtInt>,"");
    std::vector<PD_T> input;
    input.push_back(std::move(pd));
    
    return this->template Rattle<verboseQ>(input,target_iter);
}

template<bool verboseQ = false, typename ExtInt>
std::vector<PD_T> Rattle(
    mref<std::vector<PD_T>> input, const ExtInt target_iter
)
{
    static_assert(IntQ<ExtInt>,"");
    
    TOOLS_PTIMER(timer,MethodName("Rattle"));
    
    TimeInterval rattle_timer;
    
    rattle_timer.Tic();
    
    rattle_counter = 0;

    if( target_iter <= ExtInt(0) ) { return input; }
    
    std::vector<PD_T> summands;
    std::vector<PD_T> stack;
    std::vector<PD_T> output;
    
    auto rattle_push = [&stack,&output]( PD_T && pd )
    {
        if( pd.CrossingCount() == Int(0) )
        {
            if constexpr ( verboseQ )
            {
                logprint(MethodName("Rattle") + "::rattle_push : No crossings left; discarding summand.");
            }
            return;
        }
        
        if( pd.ProvenMinimalQ() )
        {
            if constexpr ( verboseQ )
            {
                logprint(MethodName("Rattle") + "::rattle_push : Proven minimal; moving summand to output.");
            }
            output.push_back(std::move(pd));
        }
        else
        {
            if constexpr ( verboseQ )
            {
                logprint(MethodName("Rattle") + "::rattle_push : Pushing summand back to stack.");
            }
            stack.push_back(std::move(pd));
        }
    };
    
    // First we simplify all inputs...
    for( auto & pd : input )
    {
        // TODO: Why not directly pushing to stack?
        pd.Simplify5( summands );
        summands.push_back( std::move(pd) );
    }
    // ... and push them to the stack (or to output).
    for( auto & pd : summands )
    {
        // We should delete pd from summands.
        rattle_push( std::move(pd) );
    }
    
    // Now process the stack.
    while( !stack.empty() )
    {
//                print("=======================");
//                TOOLS_DUMP(stack.size());
        
        PD_T pd_0 = std::move(stack.back());
        stack.pop_back();
        
//                TOOLS_DUMP(pd_0.CrossingCount());
        
        if( pd_0.ProvenMinimalQ() )
        {
            wprint(MethodName("Rattle") + ": Found a diagram on the stack that is proven to be minimal.");
            output.push_back( std::move(pd_0) );
            continue;
        }

        Tensor1<Int,Int> comp_ptr;
        Tensor1<Point_T,Int> x;
        Link_T L;
        
        const bool randomizeQ = (
            permute_randomQ
            ||
            ortho_draw_settings.randomize_virtual_edgesQ
            ||
            (ortho_draw_settings.randomize_bends > 0)
        );
        
        if ( !randomizeQ )
        {
            std::tie(comp_ptr,x) = Embedding(pd_0).Disband();
            L = Link_T( std::move(comp_ptr) );
        }
        
        bool successQ = false;
        
        for( ExtInt iter = 0; iter < target_iter; ++iter )
        {
            if constexpr ( verboseQ )
            {
                logvalprint(MethodName("Rattle") + ": Starting iteration",iter);
            }
            
            if ( randomizeQ )
            {
                std::tie(comp_ptr,x) = Embedding(pd_0).Disband();
                L = Link_T( std::move(comp_ptr) );
            }

            ++rattle_counter;
            L.SetTransformationMatrix( RandomRotation() );
            L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );

            int flag = L.FindIntersections();
            
            if( flag != 0 )
            {
                wprint(MethodName("Rattle") + ": " + L.MethodName("FindIntersections") + " returned status flag " + ToString(flag) + " != 0. Check the results carefully.");
            }
            
            PD_T pd_1 ( L );
            
            if constexpr ( verboseQ )
            {
                logvalprint(MethodName("Rattle") + ": crossing count before simplification",pd_1.CrossingCount());
            }
            summands.clear();
            pd_1.Simplify5( summands );
            
            if constexpr ( verboseQ )
            {
                logvalprint(MethodName("Rattle") + ": crossing count after simplification",pd_1.CrossingCount());
            }
            
            // We count it as success if we found some (nontrivial) summand.
            if( !summands.empty() )
            {
                if constexpr ( verboseQ )
                {
                    logprint(MethodName("Rattle") + ": Found " + ToString(summands) + " new summands.");
                }
                
                for( auto & pd_2 : summands )
                {
                    rattle_push( std::move(pd_2) );
                }
                
                rattle_push( std::move(pd_1) );
                successQ = true;
                break;
            }
            
            if( pd_1.CrossingCount() < pd_0.CrossingCount() )
            {
                if constexpr ( verboseQ )
                {
                    logprint(MethodName("Rattle") + ": No summands, but smaller crossing count.");
                }
                rattle_push( std::move(pd_1) );
                successQ = true;
                break;
            }
            
        } // for( Int iter = 0; iter < target_iter; ++iter )
        
        // If we made no success after target_iter attempts, we give up and move pd_0 to the output list.
        if( !successQ )
        {
            if constexpr ( verboseQ )
            {
                logprint(MethodName("Rattle") + " : Giving up on summand with " + ToString(pd_0.CrossingCount()) + "; moving it to output.");
            }
            
            output.push_back( std::move(pd_0) );
        }
    }
    
    Sort(
        &*output.begin(),
        &*output.end(),
        []( cref<PD_T> x, cref<PD_T> y )
        {
            return ( x.CrossingCount() > y.CrossingCount() );
        }
    );
    
    rattle_timer.Toc();
    
    rattle_timing = rattle_timer.Duration();
    
    return output;
}
