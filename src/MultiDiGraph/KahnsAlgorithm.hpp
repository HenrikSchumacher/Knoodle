public:

cref<Tensor1<VInt,VInt>> TopologicalOrdering() const
{
    std::string tag = "TopologicalOrdering";
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)){ KahnsAlgorithm(); }
    return this->template GetCache<Tensor1<VInt,VInt>>("TopologicalOrdering");
}

cref<Tensor1<VInt,VInt>> TopologicalNumbering() const
{
    std::string tag = "TopologicalNumbering";
    TOOLS_PTIMER(timer,MethodName(tag));
    if(!this->InCacheQ(tag)){ KahnsAlgorithm(); }
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}

protected:

void KahnsAlgorithm() const
{
    TOOLS_PTIMER(timer,MethodName("KahnsAlgorithm"));
    
    const std::string tag_ord = "TopologicalOrdering";
    const std::string tag_num = "TopologicalNumbering";
    
    if( proven_cyclicQ )
    {
        this->SetCache(tag_ord,Tensor1<VInt,VInt>());
        this->SetCache(tag_num,Tensor1<VInt,VInt>());
        return;
    }
    
    this->template RequireIncidenceMatrices<0,1,1>();
    
    VV_Vector_T p ( vertex_count ); // topological ordering.
    VV_Vector_T q ( vertex_count ); // topological numbering.
    VInt p_front = 0;
    VInt p_back  = 0;
    
    // We have to initialize to account for disconnected vertices.
//    V_scratch.SetZero();
    mptr<VInt> indegree = V_scratch.data();
    
    cptr<EInt> V_In_ptr = InIncidenceMatrix().Outer().data();
//    cptr<VInt> V_In_idx = InIncidenceMatrix().Inner().data();
    
    for( VInt v = 0; v < vertex_count; ++v )
    {
        const VInt d = V_In_ptr[v+VInt(1)] - V_In_ptr[v];
        
        indegree[v] = d;
        
        if( d == VInt(0) )
        {
            // Push back.
            p[p_back++] = v;
        }
    }
    
    cptr<EInt> V_Out_ptr = OutIncidenceMatrix().Outer().data();
    cptr<VInt> V_Out_idx = OutIncidenceMatrix().Inner().data();
    
    VInt level = 0;

    while( p_front < p_back )
    {
        // Process all vertices in the fresh part of q at once.
        const VInt i_begin = p_front;
        const VInt i_end   = p_back;
        
        for( VInt i = i_begin; i < i_end; ++i )
        {
            const VInt v = p[i];
            
            q[v] = level;
            
            const EInt j_begin = V_Out_ptr[v          ];
            const EInt j_end   = V_Out_ptr[v + VInt(1)];
            
            // Cycle over all edges e that go out of v
            for( EInt j = j_begin; j < j_end; ++j )
            {
                const EInt e = V_Out_idx[j];
                const VInt w = edges(e,Head);
                
                --indegree[w];
                
                if( indegree[w] == VInt(0) )
                {
                    // Push back.
                    p[p_back++] = w;
                }
            }
        }
        
        ++level;
        p_front = i_end;
        
    } // while( !S.EmptyQ() )
    
    if( p_back != vertex_count )
    {
        wprint(ClassName()+"::KahnsAlgorithm: Graph is proven to be cyclic. Returning empty list."
        );
        
        proven_cyclicQ = true;
        this->SetCache(tag_ord,Tensor1<VInt,VInt>());
        this->SetCache(tag_num,Tensor1<VInt,VInt>());
        return;
    }

    proven_acyclicQ = true;
    
    Tensor1<VInt,VInt> perm (VertexCount());
    
    for( VInt i = 0; i < VertexCount(); ++i )
    {
        perm[p[i]] = i;
    }
    
    this->SetCache(tag_ord,std::move(perm));
    this->SetCache(tag_num,std::move(q   ));
}
