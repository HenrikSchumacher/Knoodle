template<typename T>
struct hash
{
    inline Size_T operator()( cref<T> x_0 ) const
    {
        Size_T x = static_cast<Size_T>(x_0);
        x = (x ^ (x >> 30)) * Size_T(0xbf58476d1ce4e5b9);
        x = (x ^ (x >> 27)) * Size_T(0x94d049bb133111eb);
        x =  x ^ (x >> 31);
        return static_cast<Size_T>(x);
    }
};

template<typename T, typename I>
struct Tensor1Hash
{
    inline Size_T operator()( cref<Tensor1<T,I>> v )  const
    {
        using namespace std;
        
        Size_T seed = 0;
        
        const I n = v.Size();
        
        for( I i = 0; i < n; ++i )
        {
            Tools::hash_combine(seed,v[i]);
        }
        
        return seed;
    }
};

using Code_T          = Tensor1<UInt64,Int>;
using Hash_T          = Tensor1Hash<UInt64,Int>;
using CodeContainer_T = std::unordered_set<Code_T,Hash_T>;

//Tensor2<UInt64,Size_T> Generate(
//    mref<PlanarDiagram<Int>> pd_0,
//    const Size_T crossing_threshold,
//    const Size_T attempt_count,
//    const Size_T randomize_combinatoricsQ = false
//)
//{
//    TOOLS_PTIMER(timer,MethodName("Generate"));
//    
//    std::unordered_set<Code_T,Hash_T> codes;
//    
//    auto insert = [&codes,crossing_threshold]( mref<PD_T> pd )
//    {
//        if( std::cmp_less_equal( pd.CrossingCount(), crossing_threshold ) )
//        {
//            codes.insert( pd.template MacLeodCode<UInt64>() );
//        }
//    };
//    
//    insert(pd_0);
//    
//    Tensor1<Int,Int>     comp_ptr;
//    Tensor1<Point_T,Int> x;
//    Link_2D<Real,Int>    L;
//
//    if ( !randomize_combinatoricsQ )
//    {
//        std::tie(comp_ptr,x) = Embedding( pd_0, false ).Disband();
//        L = Link_2D<Real,Int>( comp_ptr );
//    }
//    
//    for( Size_T attempt = 0; attempt < attempt_count; ++attempt )
//    {
//        if ( randomize_combinatoricsQ )
//        {
//            std::tie(comp_ptr,x) = Embedding( pd_0, true ).Disband();
//            L = Link_2D<Real,Int>( comp_ptr );
//        }
//        
//        L.SetTransformationMatrix( RandomRotation() );
//        
//        L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
//        
//        int flag = L.FindIntersections();
//        
//        if( flag != 0 )
//        {
//            wprint(MethodName("Generate") + ": Link_2D::FindIntersections return status flag " + ToString(flag) + " != 0. Check the results carefully.");
//        }
//        
//        PD_T pd ( L );
//        
//        pd.Simplify4();
//        
//        insert(pd);
//    }
//    
//    Tensor2<UInt64,Size_T> result (
//        codes.size(), 2 * crossing_threshold, PD_T::UninitializedIndex()
//    );
//    
//    Size_T i = 0;
//    
//    for( auto & v : codes )
//    {
//        v.Write( result.data(i) );
//        ++i;
//    }
//    
//    return result;
//}
//
//Tensor2<UInt64,Size_T> Generate2(
//    mref<PlanarDiagram<Int>> pd_0,
//    const Size_T crossing_threshold,
//    const Size_T restart_count,
//    const Size_T mixing_count,
//    const Size_T randomize_combinatoricsQ = false,
//    const Size_T simplify_during_mixingQ = true
//)
//{
//    TOOLS_PTIMER(timer,MethodName("Generate2"));
//    
//    std::unordered_set<Code_T,Hash_T> codes;
//    
//    auto conditional_insert = [&codes,crossing_threshold]( mref<PD_T> pd )
//    {
//        if( std::cmp_less_equal( pd.CrossingCount(), crossing_threshold ) )
//        {
//            codes.insert( pd.template MacLeodCode<UInt64>() );
//        }
//    };
//    
//    auto mix = [randomize_combinatoricsQ,this]( mref<PD_T> pd )
//    {
//        auto [comp_ptr,x] = Embedding( pd, randomize_combinatoricsQ ).Disband();
//        Link_2D<Real,Int> L ( comp_ptr );
//        L.SetTransformationMatrix( RandomRotation() );
//        L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
//        
//        int flag = L.FindIntersections();
//        if( flag != 0 )
//        {
//            wprint(MethodName("Generate") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
//            return pd;
//        }
//        else
//        {
//            return PD_T(L);
//        }
//    };
//    
//    conditional_insert(pd_0);
//    
//    TOOLS_DUMP(crossing_threshold);
//    TOOLS_DUMP(restart_count);
//    TOOLS_DUMP(mixing_count);
//    TOOLS_DUMP(randomize_combinatoricsQ);
//    TOOLS_DUMP(simplify_during_mixingQ);
//    
//    for( Size_T restart = 0; restart < restart_count; ++restart )
//    {
//        PD_T pd = pd_0;
//        
//        TOOLS_LOGDUMP(restart);
//        
//        for( Size_T mixing = 0; mixing < mixing_count; ++mixing )
//        {
//            TOOLS_LOGDUMP(mixing);
//            TOOLS_LOGDUMP(pd.CrossingCount());
//            pd = mix(pd);
//            
//            if ( simplify_during_mixingQ )
//            {
//                pd.Simplify4();
//                conditional_insert(pd);
//            }
//            else
//            {
//                PD_T pd_simplified = pd;
//                pd_simplified.Simplify4();
//                conditional_insert(pd_simplified);
//            }
//        }
//    }
//    
//    Tensor2<UInt64,Size_T> result (
//        codes.size(), 2 * crossing_threshold, PD_T::UninitializedIndex()
//    );
//    
//    Size_T i = 0;
//    
//    for( auto & v : codes )
//    {
//        v.Write( result.data(i) );
//        ++i;
//    }
//    
//    return result;
//}

std::pair<Size_T,Tensor2<UInt64,Size_T>> Generate(
    mref<PlanarDiagram<Int>> pd,
    const Int  collection_threshold,
    const Int  simplification_threshold,
    const Int  branch_count,
    const Int  depth_count,
    const bool randomize_combinatoricsQ = false
)
{
    TOOLS_PTIMER(timer,MethodName("Generate"));

    std::unordered_set<Code_T,Hash_T> codes;
    
    if( pd.CrossingCount() <= collection_threshold )
    {
        codes.insert( pd.template MacLeodCode<UInt64>() );
    }
    
    Size_T iter_count = Generate_Recursive(
        codes,
        pd,
        collection_threshold,
        simplification_threshold,
        branch_count,
        Int(0),
        depth_count,
        randomize_combinatoricsQ
    );
    
    Tensor2<UInt64,Size_T> result (
        codes.size(), 2 * collection_threshold, PD_T::UninitializedIndex()
    );
    
    Size_T i = 0;
    
    for( auto & v : codes )
    {
        v.Write( result.data(i) );
        ++i;
    }
    
    return std::pair(iter_count,result);
}

private:

Size_T Generate_Recursive(
    mref<std::unordered_set<Code_T,Hash_T>> codes,
    cref<PlanarDiagram<Int>> pd,
    const Int    collection_threshold,
    const Int    simplification_threshold,
    const Int    branch_count,
    const Int    depth,
    const Int    depth_count,
    const bool   randomize_combinatoricsQ
)
{
    Size_T iter_counter = 0;
    
    if( depth >= depth_count )
    {
        return iter_counter;
    }
    
    Tensor1<Int,Int> comp_ptr;
    Tensor1<Point_T,Int> x;
    Link_T L;
    
    if ( !randomize_combinatoricsQ )
    {
        std::tie(comp_ptr,x) = Embedding( pd, false ).Disband();
        L = Link_T( comp_ptr );
    }
    
    for( Int branch = 0; branch < branch_count; ++branch )
    {
        if ( randomize_combinatoricsQ )
        {
            std::tie(comp_ptr,x) = Embedding( pd, true ).Disband();
            L = Link_T( comp_ptr );
        }

        L.SetTransformationMatrix( RandomRotation() );
        L.template ReadVertexCoordinates<1,0>( &x.data()[0][0] );
        
        PD_T pd_mixed;
        
        int flag = L.FindIntersections();
        
        if( flag != 0 )
        {
            wprint(MethodName("Generate_Recursive") + ": Link_2D::FindIntersections returned status flag " + ToString(flag) + " != 0. Skipping result.");
            pd_mixed = pd;
        }
        else
        {
            ++iter_counter;
            pd_mixed = PD_T(L);
        }
        
        if ( pd_mixed.CrossingCount() > simplification_threshold )
        {
            pd_mixed.Simplify4();
            if( pd_mixed.CrossingCount() <= collection_threshold )
            {
                codes.insert( pd_mixed.template MacLeodCode<UInt64>() );
            }
        }
        else
        {
            PD_T pd_simplified = pd_mixed;
            pd_simplified.Simplify4();
            if( pd_simplified.CrossingCount() <= collection_threshold )
            {
                codes.insert( pd_simplified.template MacLeodCode<UInt64>() );
            }
        }
        
        
        iter_counter += Generate_Recursive(
            codes, pd_mixed,
            collection_threshold,
            simplification_threshold,
            branch_count,
            depth + Int(1),
            depth_count,
            randomize_combinatoricsQ
        );
    }
    
    return iter_counter;
}
