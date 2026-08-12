public:

/*!@brief Write the internal state to a file. Return `false` if the file
 * could not be opened or written to.
 *
 * CAUTION: The internal data layout may change over time. We give little guarantees that this i/o interface is stable over various versions of `Knoodle`. It is mostly meant for debugging. Use `PDCode` or `MacLeodCode` to store/retrieve data suitable for long-term storage.
 */
bool WriteToFile( cref<std::filesystem::path> file, bool preallocateQ = true ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out);

    if( !stream )
    {
        eprint(MethodName("WriteToFile") + ": Could not open file "
               + file.string() + ". Aborting.");
        return false;
    }
    
    // This might be suboptimal from a pure memory point of view: We create and populate a potentially very large `OutString` and then stream it to the file stream. Instead, we could send the data to `steam` chunk-by-chunk. There are several drawbacks of the latter approach:
    //  (i)  It would make us fuse `WriteToOutString` and `WriteToFile` leading to less separation of responsibility and -- worse -- code duplication. As `WriteToOutString` is useful on its own, think of sending this data to the `Profiler` log file.
    //  (ii)  Several writes to `stream` could be interleaved by other threads. There is some hope that `<<` is implemented as atomic operation.
    //  (iii) `std::ofstream` is typically not as fast as `OutString` performed in RAM.
    //  (iv)  This kind of output format is oversized anyways, as it contains a lot of redundant information to make processing faster.
    //  (v)   The output will have roughly the size of the diagram in memory. If you cannot hold two of such diagrams in memory at the same time, then you have much bigger problems than not being able to write it to file.
    //  (vi)  The main purpose of this routine is debugging. In that scenario, the diagrams to check are not very big.
    
    OutString s;
    WriteToOutString(s,preallocateQ);
    stream << s;
    
    if( !stream )
    {
        eprint(MethodName("WriteToFile") + ": Could not write to file "
               + file.string() + ".");
        return false;
    }

    return true;
}

/*!@brief Write the internal state to an `OutString`.
 *
 * CAUTION: The internal data layout may change over time. We give little guarantees that this i/o interface is stable over various versions of `Knoodle`. It is mostly meant for debugging. Use `PDCode` or `MacLeodCode` to store/retrieve data suitable for long-term storage.
 *
 * @param s Target `OutString`.
 *
 * @param preallocateQ If set to `true`, then a worst-case estimate of the string size is computed and allocated by calling `OutString::RequireFreeSpace` to make sure that the output is complete. If set to `false`, then allocation is done more granular.
 */

bool WriteToOutString( mref<Tools::OutString> s, bool preallocateQ = true ) const
{
    // needs to know all member variables
    
    TOOLS_PTIMER(timer, MethodName("WriteToOutString"));
    
    if( preallocateQ )
    {
        return this->template WriteToOutString_impl<true>(s);
    }
    else
    {
        return this->template WriteToOutString_impl<false>(s);
    }
    
}
private:

template<bool preallocateQ>
bool WriteToOutString_impl( mref<Tools::OutString> s ) const
{
    // needs to know all member variables
   
    // Using 1-byte separators that are compatible with Mathematica and that minimize string size..
    constexpr char prefix [2] = "{";
    constexpr char infix  [2] = ",";
    constexpr char suffix [2] = "}";
    
    const Int n = max_crossing_count;
    const Int m = max_arc_count;

    if constexpr ( preallocateQ )
    {
        // Computing an upper bound for the size of the string to avoid reallocation and copying.
        
        // CAUTION: The following sizes need to be recomputed if anything in the body of the function changes.
        
        // `string_lit_size` is obtained by counting the number of characters in `PutChars` and `PutChar` with Mathematica. Escape sequences are counted as two characters, so this is an upper bound.
        constexpr Size_T string_lit_size = 182;
        // For the class name.
        constexpr Size_T name_size = ClassName().size();
        // Contribution of the four `Put` commands.
        constexpr Size_T put_size =  Size_T(4) * ToChars<PD_T::Int>::char_count;
        constexpr Size_T size_init = string_lit_size + name_size + put_size;

        Size_T size = size_init;
        size += OutString::ArrayCharCount<typename PD_T::CrossingContainer_T::Scal>(
            n     , prefix, infix, suffix,
            Int(2), prefix, infix, suffix,
            Int(2), prefix, infix, suffix
        );
        size += OutString::ArrayCharCount<typename PD_T::CrossingStateContainer_T::Scal>(
            n     , prefix, infix, suffix
        );
        size += OutString::ArrayCharCount<typename PD_T::ArcContainer_T::Scal>(
            n     , prefix, infix, suffix,
            Int(2), prefix, infix, suffix
        );
        size += OutString::ArrayCharCount<typename PD_T::ArcStateContainer_T::Scal>(
            n     , prefix, infix, suffix
        );
        size += OutString::ArrayCharCount<typename PD_T::ArcColorContainer_T::Scal>(
            n     , prefix, infix, suffix
        );
        
        s.RequireFreeSpace(size);
    }
        
    constexpr bool check_sizeQ = !preallocateQ;
    
    
//    // DEBUGGING
//    valprint("s.Capacity() before write", s.Capacity());
//    valprint("s.Size()     before write", s.Size());
    
    // We put `ClassName()` on top of the string as it contains the integer type to use for indices.
    // It depends on this integer type what `PD_T::Uninitialized` is: for signed integers, we have `PD_T::Uninitialized == -1`; but for unsigned integer types `PD_T::Uninitialized` is the largest possible value. So knowing the integer type is crucial for reconstructing a diagram that contain inactive crossings or inactive vertices.
    // This also means that signed types will typically lead to smaller files.
    s.template PutChars<check_sizeQ>(ClassName());
    
    // I made it deliberately hard to write without size check. That makes the following a bit more ugly than I like it.
    s.template PutChars<check_sizeQ>("\nmax_crossing_count = ");
    s.template Put<check_sizeQ>(max_crossing_count);
    
    s.template PutChars<check_sizeQ>("\ncrossing_count = ");
    s.template Put<check_sizeQ>(crossing_count);
    s.template PutChars<check_sizeQ>("\nmax_arc_count = ");
    s.template Put<check_sizeQ>(max_arc_count);
    s.template PutChars<check_sizeQ>("\narc_count = ");
    s.template Put<check_sizeQ>(arc_count);
    
    s.template PutChars<check_sizeQ>("\nC_arcs = ");
    s.PutArray(C_arcs.ReadAccess(), check_sizeQ,
        n     , prefix, infix, suffix,
        Int(2), prefix, infix, suffix,
        Int(2), prefix, infix, suffix
    );
    s.template PutChars<check_sizeQ>("\nC_state = ");
    s.PutArray(C_state.ReadAccess(), check_sizeQ, n, prefix, infix, suffix);
    
    s.template PutChars<check_sizeQ>("\nA_cross = ");
    s.PutArray(A_cross.ReadAccess(), check_sizeQ,
        m     , prefix, infix, suffix,
        Int(2), prefix, infix, suffix
    );
    s.template PutChars<check_sizeQ>("\nA_state = ");
    s.PutArray(A_state.ReadAccess(), check_sizeQ, m, prefix, infix, suffix);
    s.template PutChars<check_sizeQ>("\nA_color = ");
    s.PutArray(A_color.ReadAccess(), check_sizeQ, m, prefix, infix, suffix);
    s.template PutChars<check_sizeQ>("\nlast_color_deactivated = ");
    s.Put(last_color_deactivated);
    s.template PutChars<check_sizeQ>("\nproven_minimalQ = ");
    s.Put(proven_minimalQ);
    s.template PutChar<check_sizeQ>('\n');
    
//    // DEBUGGING
//    valprint("s.Capacity() after write", s.Capacity());
//    valprint("s.Size()     after write", s.Size());
    
    return true;
}
