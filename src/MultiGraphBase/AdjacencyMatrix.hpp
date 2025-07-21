template<
    bool undirQ, typename Scal = ToSigned<EInt>, typename ExtScal = Scal
>
Sparse::MatrixCSR<Scal,VInt,EInt> CreateAdjacencyMatrix(
    const ExtScal * edge_weights = nullptr,
    const EInt      thread_count = EInt(1)
) const
{
    TOOLS_PTIMER(timer, ClassName()+"::CreateAdjacencyMatrix"
        + "<" + ToString(undirQ)
        + "," + TypeName<Scal>
        + ">"
    );
    
    const EInt thread_count_ = Max( thread_count, EInt(1) );
    
    using Agg_T = TripleAggregator<VInt,VInt,Scal,EInt>;
    
    std::vector<Agg_T> agg ( ToSize_T(thread_count_) );
    
    if( edge_weights == nullptr )
    {
        ParallelDo(
            [this,thread_count_,&agg]( const EInt thread )
            {
                const EInt E_count = this->EdgeCount();
                const EInt e_begin = JobPointer(E_count,thread_count_,thread  );
                const EInt e_end   = JobPointer(E_count,thread_count_,thread+1);
                
                Agg_T a ( ToSize_T(e_end - e_begin) );
                
                for( EInt e = e_begin; e < e_end; ++e )
                {
                    const VInt i = edges(e,Tail);
                    const VInt j = edges(e,Head);
                    
                    a.Push( i, j, Scal(1) );
                }
                
                agg[ToSize_T(thread)] = std::move(a);
            },
            thread_count
        );

    }
    else
    {
        ParallelDo(
            [this,thread_count_,edge_weights,&agg]( const EInt thread )
            {
                const EInt E_count = this->EdgeCount();
                const EInt e_begin = JobPointer(E_count,thread_count_,thread  );
                const EInt e_end   = JobPointer(E_count,thread_count_,thread+1);
                
                Agg_T a ( ToSize_T(e_end - e_begin) );
                
                for( EInt e = e_begin; e < e_end; ++e )
                {
                    const VInt i = edges(e,Tail);
                    const VInt j = edges(e,Head);
                    
                    a.Push( i, j, edge_weights[e] );
                }
                
                agg[ToSize_T(thread)] = std::move(a);
            },
            thread_count
        );
    }
    
    return Sparse::MatrixCSR<Scal,VInt,EInt>(
        agg, vertex_count, vertex_count, thread_count, true, true
    );
}


template<typename Scal = ToSigned<EInt>>
cref<Sparse::MatrixCSR<Scal,VInt,EInt>> DirectedAdjacencyMatrix() const
{
    const std::string tag = std::string("DirectedAdjacencyMatrix<") + TypeName<Scal> + ">";
    
    if(!this->InCacheQ(tag))
    {
        this->SetCache(tag,CreateAdjacencyMatrix<false,Scal>());
    }
    
    return this->template GetCache<cref<Sparse::MatrixCSR<Scal,VInt,EInt>>>(tag);
}

template<typename Scal = ToSigned<EInt>>
cref<Sparse::MatrixCSR<Scal,VInt,EInt>> UndirectedAdjacencyMatrix() const
{
    const std::string tag = std::string("UndirectedAdjacencyMatrix<") + TypeName<Scal> + ">";
    
    if(!this->InCacheQ(tag))
    {
        this->SetCache(tag,CreateAdjacencyMatrix<true,Scal>());
    }
    
    return this->template GetCache<cref<Sparse::MatrixCSR<Scal,VInt,EInt>>>(tag);
}
