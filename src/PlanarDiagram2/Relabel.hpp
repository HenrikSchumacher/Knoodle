public:

/*!@brief Create a new diagram my mapping each active crossing index to `c_map[c]` (unless that value is invalid; then that crossing is ignored) and by mapping each active arc index to `a_map[a]` (unless that value is invalid; then that arc is ignored).
 *
 * @param c_map A lookup table for the new crossing labels. The size must be at least `this->MaxCrossingCount()` and the maximum value must be nonnegative and less than `max_c_label_plus_one`.
 *
 * @param max_c_label_plus_one The maximum crossing label plus one. This is the minimum contiguous space required to store all the labels in `c_map` and `0`.
 *
 * @param a_map A lookup table for the new arc labels. The size must be at least `this->MaxArcCount()` and the maximum value must be nonnegative and less than `max_a_label_plus_one`.
 *
 * @param max_a_label_plus_one The maximum arc label plus one. This is the minimum contiguous space required to store all the labels in `a_map` and `0`.
 *
 * @param surjectiveQ If set to true, then we assume that all crossing labels in the range [0,max_c_label_plus_one[ and all arc labels in the range [0,max_a_label_plus_one[ are present in c_map and a_map. (So the gaps need not be filled.)
 */

template<IntQ ExtInt>
PD_T CreateRelabeled(
    cptr<ExtInt> c_map, const ExtInt max_c_label_plus_one,
    cptr<ExtInt> a_map, const ExtInt max_a_label_plus_one,
    bool surjectiveQ = false
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

    // TODO: Check that `max_c_label_plus_one - 1`  and `max_a_label_plus_one - 1` fit into Int.
    
    const Int n = int_cast<Int>( Max( max_c_label_plus_one, ExtInt((max_a_label_plus_one + ExtInt(1))/ExtInt(2) )) );
    
    if( n == Int(0) )
    {
        return PD_T::InvalidDiagram();
    }
    
    PD_T pd = surjectiveQ ? PD_T(n,true) : PD_T(n);
    
    pd.proven_minimalQ        = this->proven_minimalQ;
    pd.last_color_deactivated = this->last_color_deactivated;
    
    for( Int s = 0; s < MaxCrossingCount(); ++s )
    {
        if( !this->CrossingActiveQ(s) ) { continue; }
        
        const Int t = c_map[s];

        if( !ValidIndexQ(t) ) { continue; }
        
        if( t < Int(0) )
        {
            eprint(tag() + ": Found negative mapped crossing index. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }
        
        if( t >= max_c_label_plus_one )
        {
            eprint(tag() + ": Found mapped crossing index greater or equal to max_c_label_plus_one. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }

        pd.C_arcs(t,Out,Left ) = static_cast<Int>(a_map[C_arcs(s,Out,Left )]);
        pd.C_arcs(t,Out,Right) = static_cast<Int>(a_map[C_arcs(s,Out,Right)]);
        pd.C_arcs(t,In ,Left ) = static_cast<Int>(a_map[C_arcs(s,In ,Left )]);
        pd.C_arcs(t,In ,Right) = static_cast<Int>(a_map[C_arcs(s,In ,Right)]);
        
        pd.C_state[t] = C_state[s];
    }
    
    for( Int s = 0; s < MaxArcCount(); ++s )
    {
        if( !this->ArcActiveQ(s) ) { continue; }
        
        const ExtInt t = a_map[s];
        
        if( !ValidIndexQ(t) ) { continue; }
        
        if( t < ExtInt(0) )
        {
            eprint(tag() + ": Found negative mapped arc index. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }
        
        if( t >= max_a_label_plus_one )
        {
            eprint(tag() + ": Found mapped arc index greater or equal to max_a_label_plus_one. Aborting and returning invalid diagram.");
            return InvalidDiagram();
        }

        pd.A_cross(t,Tail) = static_cast<Int>(c_map[A_cross(s,Tail)]);
        pd.A_cross(t,Head) = static_cast<Int>(c_map[A_cross(s,Head)]);
        
        pd.A_state[t] = A_state[s];
        pd.A_color[t] = A_color[s];
    }
    
    pd.crossing_count = pd.CountActiveCrossings();
    pd.arc_count      = pd.CountActiveArcs();
    
    return pd;
}

/*!@briefSame as `CreateRelabeled`, but as in-place version. It effectively calls `CreateRelabeled`, so it is not really more efficient than that.
 */

template<IntQ ExtInt>
void  Relabel(
    cptr<ExtInt> c_map, const ExtInt max_c_label_plus_one,
    cptr<ExtInt> a_map, const ExtInt max_a_label_plus_one,
    bool surjectiveQ = false
)
{
    PD_T pd = CreateRelabeled( c_map, max_c_label_plus_one, a_map, max_a_label_plus_one, surjectiveQ );
    
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

#ifdef PD_ALLOCATE_SCRATCH
void ScratchPackedCrossingIndices() const
{
    WritePackedCrossingIndices( C_scratch.data() );
}
#endif // PD_ALLOCATE_SCRATCH

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

#ifdef PD_ALLOCATE_SCRATCH
void ScratchPackedArcIndices() const
{
    WritePackedArcIndices( A_scratch.data() );
}
#endif // PD_ALLOCATE_SCRATCH

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

/*!@brief Create a new diagram in which the gaps between the active crossings and active arcs are removed, but their ordering is preserved. Hence, in contrast to `CreateCompressed`, the crossings and arcs won't be in canonical ordering. (Which means that the ordering is not as cache-friendly as `CreateCompressed`'s.
 */

PD_T CreatePacked() const
{
    if( PackedQ() )
    {
        PD_T pd = *this;
        
        return pd;
    }
#ifdef PD_ALLOCATE_SCRATCH
    ScratchPackedCrossingIndices();
    ScratchPackedArcIndices();
#else
    Tensor1<Int,Int> C_scratch ( max_crossing_count );
    Tensor1<Int,Int> A_scratch ( max_arc_count );
#endif
    
    return CreateRelabeled( C_scratch.data(), CrossingCount(), A_scratch.data(), arc_count, true );
}

/*!@brief In-place version of `CreatePacked`. This effectively calls the latter, so this not really more efficient.
 */
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

#ifdef PD_ALLOCATE_SCRATCH
void ScratchRandomPackedCrossingIndices( mref<PRNG_T> random_engine )  const
{
    WriteRandomPackedCrossingIndices( random_engine, C_scratch.data() );
}
#endif // PD_ALLOCATE_SCRATCH

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

#ifdef PD_ALLOCATE_SCRATCH
void ScratchRandomPackedArcIndices( mref<PRNG_T> random_engine ) const
{
    WriteRandomPackedArcIndices( random_engine, A_scratch.data() );
}
#endif // PD_ALLOCATE_SCRATCH

template<typename PRNG_T>
Tensor1<Int,Int> RandomPackedArcIndices( mref<PRNG_T> random_engine ) const
{
    Tensor1<Int,Int> a_map ( max_arc_count );
    WriteRandomPackedArcIndices( random_engine, a_map.data() );
    return a_map;
}

/*!@brief Create a new diagram in which the crossings and arcs of this diagram are permuted randomly.
 *
 * @param random_engine Some seeded random engine conforming to the C++ Standard Library.
 */

PD_T CreatePermutedRandom( mref<PRNG_T> random_engine ) const
{
#ifdef PD_ALLOCATE_SCRATCH
    ScratchRandomPackedCrossingIndices( random_engine );
    ScratchRandomPackedArcIndices( random_engine );
#else
    auto C_scratch = RandomPackedCrossingIndices( random_engine );
    auto A_scratch = RandomPackedArcIndices( random_engine );
#endif
    
    return CreateRelabeled( C_scratch.data(), CrossingCount(), A_scratch.data(), arc_count, true );
}


/*!@brief Permute the crossings and arcs randomly. In-place version of `CreatePermutedRandom`. The latter is effectively called by this; so this is not more efficient.
 *
 * @param random_engine Some seeded random engine conforming to the C++ Standard Library.
 */

void PermuteRandom( mref<PRNG_T> random_engine )
{
    PD_T pd = CreatePermutedRandom( random_engine );
    if( ValidQ() == pd.ValidQ() ) { *this = std::move(pd); }
}

