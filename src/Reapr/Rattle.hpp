public:

int RattleIterations() const
{
    return rattle_counter;
}

double RattleTiming() const
{
    return rattle_timing;
}

template<bool verboseQ = false, typename Int>
std::vector<PlanarDiagram<Int>> Rattle( cref<PlanarDiagram<Int>> pd, const Int target_iter )
{
    std::vector<PlanarDiagram<Int>> input;
    input.push_back(pd); // Using copy constructor here!
    
    return this->template Rattle<verboseQ>(input,target_iter);
}

template<bool verboseQ = false, typename Int>
std::vector<PlanarDiagram<Int>> Rattle( PlanarDiagram<Int> && pd, const Int target_iter )
{
    std::vector<PlanarDiagram<Int>> input;
    input.push_back(std::move(pd));
    
    return this->template Rattle<verboseQ>(input,target_iter);
}

template<bool verboseQ = false, typename Int>
std::vector<PlanarDiagram<Int>> Rattle(
    mref<std::vector<PlanarDiagram<Int>>> input, const Int target_iter
)
{
    TOOLS_PTIMER(timer,MethodName("Rattle"));
    
    TimeInterval rattle_timer;
    
    rattle_timer.Tic();
    
    rattle_counter = 0;
    
    if( target_iter <= Int(0) ) { return input; }
    
    using PD_T = PlanarDiagram<Int>;
    
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
        pd.Simplify5( summands );
        summands.push_back( std::move(pd) );
    }
    // ... and push them to the stack (or to output).
    for( auto & pd : summands )
    {
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

        auto [comp_ptr,x] = Embedding(pd_0).Disband();
        
//                // DEBUGGING
//                Tiny::VectorList_AoS<3,Real,Int> x_;
//                x_.Read( &x.data()[0][0] );
//                TOOLS_PDUMP(comp_ptr);
//                TOOLS_PDUMP(x_);
        
        Link_2D<Real,Int> L ( comp_ptr );
        
        bool successQ = false;
        
        for( Int iter = 0; iter < target_iter; ++iter )
        {
            if constexpr ( verboseQ )
            {
                logvalprint(MethodName("Rattle") + ": Starting iteration",iter);
            }
            
            ++rattle_counter;
            L.SetTransformationMatrix( RandomRotation<Int>() );
            L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );

            int flag = L.FindIntersections();
            
            if( flag != 0 )
            {
                wprint(MethodName("Rattle") + ": Link_2D::FindIntersections return status flag " + ToString(flag) + " != 0. Check the results carefully.");
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


// This rotation must be orientation preserving.
template<typename Int>
Tiny::Matrix<3,3,Real,Int> RandomRotation()
{
    using Vector_T = Tiny::Vector<3,Real,Int>;
    using Matrix_T = Tiny::Matrix<3,3,Real,Int>;
    
    std::normal_distribution<Real> gaussian {Real(0),Real(1)};
    std::uniform_real_distribution<Real> angle_dist {Real(0), Scalar::Pi<Real>};
    
    Vector_T u;
    Real u_squared;
    do
    {
        u[0] = gaussian(random_engine);
        u[1] = gaussian(random_engine);
        u[2] = gaussian(random_engine);
        u_squared = u.NormSquared();
    }
    while( u_squared <= Real(0) );
        
    u /= Sqrt(u_squared);
    
    // Code copied from ClisbyTree.
    const Real angle = angle_dist(random_engine);
    
    const Real cos = std::cos(angle);
    const Real sin = std::sin(angle);
    
    Matrix_T A;
    
    const Real d = Real(1) - cos;
    
    A[0][0] = u[0] * u[0] * d + cos       ;
    A[0][1] = u[0] * u[1] * d - sin * u[2];
    A[0][2] = u[0] * u[2] * d + sin * u[1];
    
    A[1][0] = u[1] * u[0] * d + sin * u[2];
    A[1][1] = u[1] * u[1] * d + cos       ;
    A[1][2] = u[1] * u[2] * d - sin * u[0];
    
    A[2][0] = u[2] * u[0] * d - sin * u[1];
    A[2][1] = u[2] * u[1] * d + sin * u[0];
    A[2][2] = u[2] * u[2] * d + cos       ;
 
    return A;
}
