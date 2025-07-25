public:

template<bool undirQ, bool inQ, bool outQ>
void RequireIncidenceMatrices() const
{
    bool undirQ_missedQ = (undirQ && !this->InCacheQ("OrientedIncidenceMatrix"));
    bool inQ_missedQ    = (inQ    && !this->InCacheQ("InIncidenceMatrix"));
    bool outQ_missedQ   = (outQ   && !this->InCacheQ("OutIncidenceMatrix"));
    
    int missing_count = undirQ_missedQ + inQ_missedQ + outQ_missedQ;
    
    if( missing_count <= 0 )
    {
        return;
    }
    else if( missing_count == 1 )
    {
        if( undirQ_missedQ )
        {
            this->ComputeIncidenceMatrices<1,0,0>();
        }
        if( inQ_missedQ )
        {
            this->ComputeIncidenceMatrices<0,1,0>();
        }
        if( outQ_missedQ )
        {
            this->ComputeIncidenceMatrices<0,0,1>();
        }
    }
    else if( missing_count > 1 )
    {
        this->ComputeIncidenceMatrices<undirQ,inQ,outQ>();
    }

}

private:

template<bool undirQ, bool inQ, bool outQ>
void ComputeIncidenceMatrices() const
{
    TOOLS_PTIMER(timer,ClassName()+"::ComputeIncidenceMatrices"
        + "<" + ToString(undirQ)
        + "," + ToString(inQ)
        + "," + ToString(outQ)
        + ">"
    );

    const EInt edge_count = edges.Dim(0);
    TripleAggregator<EInt,EInt,Sign_T,EInt> agg_undir;
    PairAggregator<EInt,EInt,EInt> agg_out;
    PairAggregator<EInt,EInt,EInt> agg_in;
    
    if constexpr( undirQ )
    {
        agg_undir = TripleAggregator<EInt,EInt,Sign_T,EInt> ( EInt(2) * edge_count );
    }
    if constexpr( outQ )
    {
        agg_out = PairAggregator<EInt,EInt,EInt> ( edge_count );
    }
    if constexpr( inQ )
    {
        agg_in  = PairAggregator<EInt,EInt,EInt> ( edge_count );
    }

    for( EInt e = 0; e < edge_count; ++e )
    {
        Edge_T E ( edges.data(e) );
        
        if constexpr( undirQ )
        {
            agg_undir.Push( E[Tail], e, Sign_T(-1) );
            agg_undir.Push( E[Head], e, Sign_T( 1) );
        }
        if constexpr( outQ )
        {
            agg_out.Push( E[Tail], e );
        }
        if constexpr( inQ )
        {
            agg_in .Push( E[Head], e );
        }
    }
    
    const EInt v_count = static_cast<EInt>(vertex_count);

    if constexpr( undirQ )
    {
        this->SetCache(
            "OrientedIncidenceMatrix",
            SignedMatrix_T<EInt,1>(agg_undir,v_count,edge_count,EInt(1),true,false)
//                                                                        ^
//                        Compression needed in case of loop edges. ------+
        );
    }
    if constexpr( outQ )
    {
        this->SetCache(
            "OutIncidenceMatrix",
            SignedMatrix_T<EInt,0>(agg_out,v_count,edge_count,EInt(1),false,false)
//                                                                      ^
//                            No compression needed because here. ------+
        );
    }
    if constexpr( inQ )
    {
        this->SetCache(
            "InIncidenceMatrix",
            SignedMatrix_T<EInt,0>(agg_in,v_count,edge_count,EInt(1),false,false)
//                                                                     ^
//                           No compression needed because here. ------+
        );
    }
}

public:

cref<SignedMatrix_T<EInt,1>> OrientedIncidenceMatrix() const
{
    std::string tag ("OrientedIncidenceMatrix");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) )
    {
        // TODO: It should actually be faster to merge OrientedIncidenceMatrix from InIncidenceMatrix and OutIncidenceMatrix.
        this->template ComputeIncidenceMatrices<1,0,0>();
    }
    return this->template GetCache<SignedMatrix_T<EInt,1>>(tag);
}


cref<SignedMatrix_T<EInt,0>> InIncidenceMatrix() const
{
    std::string tag ("InIncidenceMatrix");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) )
    {
        this->template ComputeIncidenceMatrices<0,1,0>();
    }
    return this->template GetCache<SignedMatrix_T<EInt,0>>(tag);
}

cref<SignedMatrix_T<EInt,0>> OutIncidenceMatrix() const
{
    std::string tag ("OutIncidenceMatrix");
    TOOLS_PTIMER(timer,MethodName(tag));
    if( !this->InCacheQ(tag) )
    {
        this->template ComputeIncidenceMatrices<0,0,1>();
    }
    return this->template GetCache<SignedMatrix_T<EInt,0>>(tag);
}
