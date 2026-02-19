public:

template<typename ExtInt>
PD_T CreateRelabeled(
     cptr<ExtInt> c_map, const ExtInt c_max, cptr<ExtInt> a_map, const ExtInt a_max, bool surjectiveQ = false
)  const
{
    static_assert(IntQ<ExtInt>,"");
    
    [[maybe_unused]] auto tag = [](){ return MethodName("CreateRelabeled"); };
    
    if( InvalidQ() ) { return InvalidDiagram(); }

    TOOLS_PTIMER(timer,tag());
    
//    if( std::cmp_not_equal(max_crossing_count,c_map.Size()) )
//    {
//        eprint(tag()+": Size " + Tools::ToString(c_map.Size()) + " does not match number of elements " + Tools::ToString(max_crossing_count) + " in Crossings(). Returning invalid diagram.");
//        return InvalidDiagram();
//    }
//    
//    if( std::cmp_not_equal(max_crossing_count,a_map.Size()) )
//    {
//        eprint(tag()+": Size " + Tools::ToString(a_map.Size()) + " does not match number of elements " + Tools::ToString(max_arc_count) + " in Arcs(). Returning invalid diagram.");
//        return InvalidDiagram();
//    }

    // TODO: Check that c_max and a_max fit into Int.
    
    const Int n = int_cast<Int>( Max( c_max, ExtInt((a_max + ExtInt(1))/ExtInt(2) )) );
    
    PD_T pd = surjectiveQ ? PD_T(n,true) : PD_T(n);
    
    pd.crossing_count         = this->crossing_count;
    pd.arc_count              = this->arc_count;
    pd.proven_minimalQ        = this->proven_minimalQ;
    pd.last_color_deactivated = this->last_color_deactivated;
    
    for( Int s = 0; s < MaxCrossingCount(); ++s )
    {
        if( !this->CrossingActiveQ(s) ) { continue; }
        
        const Int t = c_map[s];

        if( t < Int(0) )
        {
            eprint(tag() + ": Found negative mapped crossing index. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }
        
        if( t > c_max )
        {
            eprint(tag() + ": Found mapped crossing index bigger than c_max. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }

        pd.C_state[t] = this->C_state[s];

        C_Arcs_T C_s = this->CopyCrossing(s);
        C_Arcs_T C_t {
            {static_cast<Int>(a_map[C_s(Out,Left )]), static_cast<Int>(a_map[C_s(Out,Right)])},
            {static_cast<Int>(a_map[C_s(In ,Left )]), static_cast<Int>(a_map[C_s(In ,Right)])}
        };
        C_t.Write( pd.C_arcs.data(t) );
    }
    
    for( Int s = 0; s < MaxArcCount(); ++s )
    {
        if( !this->ArcActiveQ(s) ) { continue; }
        
        const ExtInt t = a_map[s];
        
        if( t < ExtInt(0) )
        {
            eprint(tag() + ": Found negative mapped arc index. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }
        
        if( t > a_max )
        {
            eprint(tag() + ": Found mapped arc index bigger than a_max. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }
        
        pd.A_state[t] = this->A_state[s];
        pd.A_color[t] = this->A_color[s];
        
        A_Cross_T A_s = this->CopyArc(s);
        A_Cross_T A_t { {static_cast<Int>(c_map[A_s(Tail)]), static_cast<Int>(c_map[A_s(Head)])} };
        A_t.Write(pd.A_cross.data(t) );
    }
    
    return pd;
}

template<typename ExtInt>
void  Relabel(
    cptr<ExtInt> c_map, const ExtInt c_max, cptr<ExtInt> a_map, const ExtInt a_max, bool surjectiveQ = false
)
{
    PD_T pd = CreateRelabeled( c_map, c_max, a_map, a_max, surjectiveQ );
    
    if( ValidQ() == pd.ValidQ() ) { *this = std::move(pd); }
}



void WritePackedCrossingIndices( mptr<Int> c_map ) const
{
    Int c_label = 0;
    
    for( Int c = 0; c < MaxCrossingCount(); ++c )
    {
        if( CrossingActiveQ(c) )
        {
            c_map[c] = c_label;
            ++c_label;
        }
        else
        {
            c_map[c] = Uninitialized;
        }
    }
}

void ScratchPackedCrossingIndices() const
{
    WritePackedCrossingIndices( C_scratch.data() );
}

Tensor1<Int,Int> PackedCrossingIndices() const
{
    Tensor1<Int,Int> c_map ( MaxCrossingCount() );
    
    WritePackedCrossingIndices(c_map.data());
    
    return c_map;
}

void WritePackedArcIndices( mptr<Int> a_map ) const
{
    Int a_label = 0;
    
    for( Int a = 0; a < MaxArcCount(); ++a )
    {
        if( ArcActiveQ(a) )
        {
            a_map[a] = a_label;
            ++a_label;
        }
        else
        {
            a_map[a] = Uninitialized;
        }
    }
}

void ScratchPackedArcIndices() const
{
    WritePackedArcIndices( A_scratch.data() );
}

Tensor1<Int,Int> PackedArcIndices() const
{
    Tensor1<Int,Int> a_map ( MaxArcCount() );
    
    WritePackedArcIndices(a_map.data());
    
    return a_map;
}


bool PackedQ() const
{
    return CrossingCount() == MaxCrossingCount();
}


PD_T CreatePacked() const
{
    if( PackedQ() )
    {
        PD_T pd = *this;
        
        return pd;
    }
    
    ScratchPackedCrossingIndices();
    ScratchPackedArcIndices();
    return CreateRelabeled( C_scratch.data(), CrossingCount(), A_scratch.data(), arc_count, true );
}

void Pack()
{
    if( PackedQ() ) { return; }
    PD_T pd = CreatePacked();
    if( ValidQ() == pd.ValidQ() ) { *this = std::move(pd); }
}

template<typename PRNG_T>
void WriteRandomPackedCrossingIndices( mref<PRNG_T> random_engine, mptr<Int> c_map ) const
{
    Int c_label = 0;
    Permutation<Int> perm = Permutation<Int>::RandomPermutation( crossing_count, Int(1), random_engine );
    cptr<Int> p = perm.GetPermutation().data();
    for( Int c = 0; c < MaxCrossingCount(); ++c )
    {
        if( CrossingActiveQ(c) )
        {
            c_map[c] = p[c_label];
            ++c_label;
        }
        else
        {
            c_map[c] = Uninitialized;
        }
    }
}

void ScratchRandomPackedCrossingIndices( mref<PRNG_T> random_engine )  const
{
    WriteRandomPackedCrossingIndices( random_engine, C_scratch.data() );
}

template<typename PRNG_T>
Tensor1<Int,Int> RandomPackedCrossingIndices( mref<PRNG_T> random_engine ) const
{
    std::string tag (MethodName("RandomPackedCrossingIndices"));
    Tensor1<Int,Int> c_map ( MaxCrossingCount() );
    WriteRandomPackedCrossingIndices( random_engine, c_map.data() );
    return c_map;
}

template<typename PRNG_T>
void WriteRandomPackedArcIndices( mref<PRNG_T> random_engine, mptr<Int> a_map ) const
{
    Int a_label = 0;
    
    Permutation<Int> perm = Permutation<Int>::RandomPermutation( arc_count, Int(1), random_engine );

    cptr<Int> p = perm.GetPermutation().data();

    for( Int a = 0; a < MaxArcCount(); ++a )
    {
        if( ArcActiveQ(a) )
        {
            a_map[a] = p[a_label];
            ++a_label;
        }
        else
        {
            a_map[a] = Uninitialized;
        }
    }
}

void ScratchRandomPackedArcIndices( mref<PRNG_T> random_engine ) const
{
    WriteRandomPackedArcIndices( random_engine, A_scratch.data() );
}

template<typename PRNG_T>
Tensor1<Int,Int> RandomPackedArcIndices( mref<PRNG_T> random_engine ) const
{
    Tensor1<Int,Int> a_map ( max_arc_count );
    WriteRandomPackedArcIndices( random_engine, a_map.data() );
    return a_map;
}


PD_T CreatePermutedRandom( mref<PRNG_T> random_engine ) const
{
    ScratchRandomPackedCrossingIndices( random_engine );
    ScratchRandomPackedArcIndices( random_engine );
    return CreateRelabeled( C_scratch.data(), CrossingCount(), A_scratch.data(), arc_count, true );
}


void PermuteRandom( mref<PRNG_T> random_engine )
{
    PD_T pd = CreatePermutedRandom( random_engine );
    if( ValidQ() == pd.ValidQ() ) { *this = std::move(pd); }
}

