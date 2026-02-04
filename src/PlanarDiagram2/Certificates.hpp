public:

bool AlternatingQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (ArcOverQ(a,Tail) == ArcOverQ(a,Head)) )
        {
            return false;
        }
    }

    return true;
}

bool LoopFreeQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( ArcActiveQ(a) && (A_cross(a,Tail) == A_cross(a,Head)) )
        {
            return false;
        }
    }

    return true;
}

bool AlternatingAndLoopFreeQ() const
{
    for( Int a = 0; a < max_arc_count; ++a )
    {
        if( !ArcActiveQ(a) ) { continue; }
        
        if( A_cross(a,Tail) == A_cross(a,Head) )
        {
            return false;
        }
        
        if( ArcOverQ(a,Tail) == ArcOverQ(a,Head) )
        {
            return false;
        }
    }

    return true;
}

bool IsthmusFreeQ() const
{
    TOOLS_PTIMER(timer,MethodName("IsthmusFreeQ"));
    
    auto & A_F = ArcFaces();
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( !CrossingActiveQ(c) ) { continue; }
        
        const C_Arcs_T C = CopyCrossing(c);
        
        const Int f_w = A_F(C[Out][Left ],0);
        const Int f_n = A_F(C[Out][Left ],1);
        const Int f_e = A_F(C[In ][Right],1);
        const Int f_s = A_F(C[In ][Right],0);
        
        if( (f_w == f_e) || (f_n == f_s) )
        {
            return false;
        }
    }
    
    return true;
}

Tensor1<Int,Int> FindIsthmi() const
{
    TOOLS_PTIMER(timer,MethodName("FindIsthmi"));
    
    Aggregator<Int,Int> agg (1);
    
    auto & A_F = ArcFaces();
    
    for( Int c = 0; c < max_crossing_count; ++c )
    {
        if( !CrossingActiveQ(c) ) { continue; }
        
        const C_Arcs_T C = CopyCrossing(c);
        
        const Int f_w = A_F(C[Out][Left ],0);
        const Int f_n = A_F(C[Out][Left ],1);
        const Int f_e = A_F(C[In ][Right],1);
        const Int f_s = A_F(C[In ][Right],0);
        
        if( (f_w == f_e) || (f_n == f_s) )
        {
            agg.Push(c);
        }
    }
    
    return agg.Disband();
}

bool ReducedQ() const
{
    return LoopFreeQ() && IsthmusFreeQ();
}


bool MinimalQ() const
{
    return AlternatingAndLoopFreeQ() && IsthmusFreeQ();
}

bool CheckProvenMinimalQ() const
{
    if( proven_minimalQ )
    {
        PD_T pd = this->CachelessCopy();
        
        bool alternatingQ = AlternatingQ();
        
        if( !alternatingQ )
        {
            wprint(MethodName("CheckProvenMinimalQ") + ": Diagram is not alternating.");
        }
        
        bool loop_freeQ = LoopFreeQ();
        
        if( !loop_freeQ )
        {
            wprint(MethodName("CheckProvenMinimalQ") + ": Diagram is not loop free.");
        }
        
        bool isthmus_freeQ = IsthmusFreeQ();
        
        if( !isthmus_freeQ )
        {
            wprint(MethodName("CheckProvenMinimalQ") + ": Diagram is not isthmus free.");
        }
        
        return alternatingQ && loop_freeQ && isthmus_freeQ;
    }
    
    return true;
}
