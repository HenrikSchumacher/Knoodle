private:

void ComputeConstraintGraphs()
{
    TOOLS_PTIC(ClassName() + "::ComputeConstraintGraphs");
 
    // cf. Klau, Mutzel - Optimal Compaction of Orthogonal Grid Drawings
    
    // TODO: Segments are numbered this way:
    // TODO: S_h = {s_0,s_1,...,s_k}; S_v = {s_{k+1},...,s_n}
    // TODO: Alas, my MultiDiGraph class does not like it!
    
 
    Aggregator<Int,Int> S_h_ptr_agg ( tre_count );
    Aggregator<Int,Int> S_h_idx_agg ( tre_count );
    Aggregator<Int,Int> S_v_ptr_agg ( tre_count );
    Aggregator<Int,Int> S_v_idx_agg ( tre_count );
    
    E_S_h = Tensor1<Int,Int>( tre_count, Uninitialized );
    E_S_v = Tensor1<Int,Int>( tre_count, Uninitialized );
    V_S_h = Tensor1<Int,Int>( vertex_count  , Uninitialized );
    V_S_v = Tensor1<Int,Int>( vertex_count  , Uninitialized );
    
    Aggregator<Int,Int> D_h_agg ( Int(2) * tre_count );
    Aggregator<Int,Int> D_v_agg ( Int(2) * tre_count );
    
//    Int counter_h = 0;
//    Int counter_v = 0;
    
    S_h_ptr_agg.Push(Int(0));
    S_v_ptr_agg.Push(Int(0));
    
//    TOOLS_DUMP(S_h_ptr_agg.Size());
//    TOOLS_DUMP(S_v_ptr_agg.Size());
    
//    TOOLS_DUMP(vertex_count);
    
    for( Int v_0 = 0; v_0 < vertex_count; ++v_0 )
    {
//        print("========");
//        TOOLS_DUMP(v_0);
//        TOOLS_DUMP(V_state[v_0]);
//        if( !VertexActiveQ(v_0) )
//        {
//            print("Skipping vertex " + ToString(v_0));
//            continue;
//        };
//        
//        TOOLS_DUMP(V_dTRE(v_0,West));
//        TOOLS_DUMP(V_dTRE(v_0,South));
        
        // TODO: Segments are also allowed to contain exactly one vertex.
        if( V_dTRE(v_0,West) == Uninitialized )
        {
//            print("Eastward");
            // Collect horizontal segment.
            const Int s = S_h_ptr_agg.Size() - Int(1);
            
//            TOOLS_DUMP(s);
            
            Int w  = v_0;
            Int de = V_dTRE(w,East);
            Int v;
//            TOOLS_DUMP(w);
            V_S_h[w] = s;
            
//            TOOLS_DUMP(de);
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDiEdge(de);
                S_h_idx_agg.Push(e);
                E_S_h[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,East);
                w  = v;
//                TOOLS_DUMP(w);
                V_S_h[w] = s;
            }

            S_h_ptr_agg.Push( S_h_idx_agg.Size() );
        }
        
        if( V_dTRE(v_0,South) == Uninitialized )
        {
//            print("Northward");
            // Collect vertical segment.
            const Int s = S_v_ptr_agg.Size() - Int(1);
            
//            TOOLS_DUMP(s);
            
            Int  w = v_0;
            Int de = V_dTRE(w,North);
            Int v;
//            TOOLS_DUMP(w);
            V_S_v[w] = s;
            
//            TOOLS_DUMP(de);
            
            while( de != Uninitialized )
            {
                auto [e,dir] = FromDiEdge(de);
                S_v_idx_agg.Push(e);
                E_S_v[e] = s;
                
                v  = TRE_V.data()[de];
                de = V_dTRE(v,North);
                w  = v;
//                TOOLS_DUMP(w);
                V_S_v[w] = s;
            }

            S_v_ptr_agg.Push( S_v_idx_agg.Size() );
        }
    }
    
//    TOOLS_DUMP(V_S_h);
//    TOOLS_DUMP(V_S_v);
    
    S_h_ptr = S_h_ptr_agg.Get();
    S_h_idx = S_h_idx_agg.Get();
    
    S_v_ptr = S_v_ptr_agg.Get();
    S_v_idx = S_v_idx_agg.Get();
    
    // This pushes only the directly obvious relations stemming from edges in H.
    // These correspond to the sets A_h and A_v from the paper.
    for( Int e = 0; e < tre_count; ++e )
    {
//        TOOLS_DUMP(e);
        if( !ValidIndexQ(e) )
        {
            print(ClassName()+"::ComputeConstraintGraphs: invalid edge index " + ToString(e) + ".");
            continue;
        }
        
        // All virtual edges are active, so we do not have to check them.
        // But the other edges need some check here.
        if( (e < edge_count) && !EdgeActiveQ(e) ) { continue; }
        
        const Int c_0 = TRE_V(e,Tail);
        const Int c_1 = TRE_V(e,Head);
        
//        TOOLS_DUMP(TRE_dir[e]);
//        TOOLS_DUMP(c_0);
//        TOOLS_DUMP(c_1);
        
        switch( TRE_dir[e] )
        {
            case East:
            {
                const Int v_0 = V_S_v[c_0];
                const Int v_1 = V_S_v[c_1];
                
//                if(v_0 == v_1)
//                {
//                    eprint("East!!!");
//                }
                D_v_agg.Push(v_0);
                D_v_agg.Push(v_1);
                break;
            }
            case North:
            {
                const Int v_0 = V_S_h[c_0];
                const Int v_1 = V_S_h[c_1];
                
//                if(v_0 == v_1)
//                {
//                    eprint("North!!!");
//                }
                D_h_agg.Push(v_0);
                D_h_agg.Push(v_1);
                break;
            }
            case West:
            {
                const Int v_0 = V_S_v[c_0];
                const Int v_1 = V_S_v[c_1];
                
//                if(v_0 == v_1)
//                {
//                    eprint("West!!!");
//                }
                D_v_agg.Push(v_1);
                D_v_agg.Push(v_0);
                break;
            }
            case South:
            {
                const Int v_0 = V_S_h[c_0];
                const Int v_1 = V_S_h[c_1];
                
//                if(v_0 == v_1)
//                {
//                    eprint("South!!!");
//                }
                D_h_agg.Push(v_1);
                D_h_agg.Push(v_0);
                break;
            }
            default:
            {
                eprint( ClassName() + "::ComputeConstraintGraphs: Invalid entry of edge direction array TRE_dir detected.");
                break;
            }
        }
    }
    
    D_h = DiGraph_T( S_h_ptr.Size() - Int(1), D_h_agg.data(), D_h_agg.Size()/2 );
    D_v = DiGraph_T( S_v_ptr.Size() - Int(1), D_v_agg.data(), D_v_agg.Size()/2 );
    
    TOOLS_PTOC(ClassName() + "::ComputeConstraintGraphs");
}

struct Segment
{
    Int  s;
    bool horizontalQ;
};

// This is l(s) from the paper, at least for maximal segments.
Segment LeftSegment( const Segment s ) const
{
    return s.horizontalQ
           ? Segment(V_S_v[ S_h_ptr[s] - 1],false)
           : s;
}

// This is r(s) from the paper, at least for maximal segments.
Segment RightSegment( const Segment s ) const
{
    return s.horizontalQ
           ? Segment(V_S_v[ S_h_ptr[s+1] - 1],false)
           : s;
}

// This is b(s) from the paper, at least for maximal segments.
Segment BottomSegment( const Segment s ) const
{
    return !s.horizontalQ
           ? Segment(V_S_h[ S_v_ptr[s] - 1],false)
           : s;
}

// This is t(s) from the paper, at least for maximal segments.
Segment TopSegment( const Segment s ) const
{
    return !s.horizontalQ
           ? Segment(V_S_h[ S_v_ptr[s+1] - 1],false)
           : s;
}


