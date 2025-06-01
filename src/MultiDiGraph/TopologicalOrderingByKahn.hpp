public:


cref<Tensor1<VInt,VInt>> TopologicalOrderingByKahn_Stack() const
{
    TOOLS_PTIC( ClassName() + "::TopologicalOrderingByKahn_Stack" );
    
    std::string tag = "TopologicalOrderingByKahn_Stack";
    
    if( !this->InCacheQ(tag) )
    {
        if( proven_cyclicQ )
        {
            this->SetCache(tag,Tensor1<VInt,VInt>());
        }
        else
        {
            this->template RequireIncidenceMatrices<0,1,1>();
            
            VV_Vector_T Q (vertex_count );
            VInt Q_front = 0;
            VInt Q_back = 0;
            VV_Vector_T p (vertex_count );
            VInt p_ptr = 0;
            
            // We have to initialize by to account for disconnected vertices.
            V_scratch.SetZero();
            mptr<VInt> indegree = V_scratch.data();
            
            auto & V_In = InIncidenceMatrix();
            cptr<EInt> V_In_ptr = V_In.Outer().data();
            
            for( VInt v = 0; v < vertex_count; ++v )
            {
                const VInt d = V_In_ptr[v+VInt(1)] - V_In_ptr[v];
                
                indegree[v] = d;
                
                if( d == VInt(0) )
                {
                    // Push back.
                    Q[Q_back++] = v;
                }
            }

            auto & V_Out = OutIncidenceMatrix();
            cptr<EInt> V_Out_ptr = V_Out.Outer().data();
            cptr<VInt> V_Out_idx = V_Out.Inner().data();
            
            while( Q_front < Q_back )
            {
                // Pop back.
                const VInt v = Q[--Q_back];
                // Push back.
                p[p_ptr++] = v;

                const EInt j_begin = V_Out_ptr[v  ];
                const EInt j_end   = V_Out_ptr[v+1];
            
                // Cycle over all edges e that go out of v
                for( EInt j = j_begin; j < j_end; ++j )
                {
                    const EInt e = V_Out_idx[j];
                    const VInt w = edges(e,Head);
                    
                    --indegree[w];
                    
                    if( indegree[w] == VInt(0) )
                    {
                        // Push back.
                        Q[Q_back++] = w;
                    }
                }
                
            } // while( !S.EmptyQ() )
            
            if( p_ptr != vertex_count )
            {
                wprint(ClassName() + "::TopologicalOrderingQByKahn_Stack: Graph is proven to be cyclic. Returning empty list."
                );
                
                proven_cyclicQ = true;
                this->SetCache(tag,Tensor1<VInt,VInt>());
            }
            else
            {
                proven_acyclicQ = true;
                this->SetCache(tag,std::move(p));
            }
        }
    }
    
    TOOLS_PTOC( ClassName() + "::TopologicalOrderingByKahn_Stack" );
    
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}

protected:

void KahnsAlgorithm() const
{
    TOOLS_PTIC( ClassName() + "::KahnsAlgorithm" );
    
    if( proven_cyclicQ )
    {
        this->SetCache("TopologicalOrderingByKahn",Tensor1<VInt,VInt>());
        this->SetCache("TopologicalLevelsByKahn"  ,Tensor1<VInt,VInt>());
    }
    else
    {
        this->template RequireIncidenceMatrices<0,1,1>();
        
        VV_Vector_T p (vertex_count );
        VInt p_front = 0;
        VInt p_back  = 0;
        
        VV_Vector_T L ( vertex_count ); // The levels for each vertex.
        
        // We have to initialize to account for disconnected vertices.
        V_scratch.SetZero();
        mptr<VInt> indegree = V_scratch.data();
        
        cptr<EInt> V_In_ptr = InIncidenceMatrix().Outer().data();
        
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
        
        auto & V_Out = OutIncidenceMatrix();
        cptr<EInt> V_Out_ptr = V_Out.Outer().data();
        cptr<VInt> V_Out_idx = V_Out.Inner().data();
        
        VInt level = 0;
        
        while( p_front < p_back )
        {
            // Process all vertices in the fresh part of L ay once.
            const VInt i_begin = p_front;
            const VInt i_end   = p_back;
            
            for( VInt i = i_begin; i < i_end; ++i )
            {
                const VInt v = p[i];
                
                L[v] = level;
                
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
            wprint(ClassName() + "::TopologicalOrderingQByKahn_Queue: Graph is proven to be cyclic. Returning empty list."
            );
            
            proven_cyclicQ = true;
            this->SetCache("TopologicalOrderingByKahn",Tensor1<VInt,VInt>());
            this->SetCache("TopologicalLevelsByKahn"  ,Tensor1<VInt,VInt>());
        }
        else
        {
            proven_acyclicQ = true;
            this->SetCache("TopologicalOrderingByKahn",std::move(p));
            this->SetCache("TopologicalLevelsByKahn"  ,std::move(L));
        }
    }
    
    TOOLS_PTOC( ClassName() + "::KahnsAlgorithm");
}

public:

cref<Tensor1<VInt,VInt>> TopologicalOrdering() const
{
    std::string tag = "TopologicalOrderingByKahn";
    
    if( !this->InCacheQ(tag) ){ KahnsAlgorithm(); }
    
    return this->template GetCache<Tensor1<VInt,VInt>>("TopologicalOrderingByKahn");
}

cref<Tensor1<VInt,VInt>> Levels() const
{
    
    std::string tag = "TopologicalLevelsByKahn";
    
    if( !this->InCacheQ(tag) ){ KahnsAlgorithm(); }
    
    return this->template GetCache<Tensor1<VInt,VInt>>(tag);
}
