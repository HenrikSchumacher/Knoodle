public:

//##########################################################################
//##    Internal-state serialization (debugging aid)
//##########################################################################

/*!@brief **EXPERIMENTAL** Serialize the diagram's full internal state as text.
 *
 * PD codes (and everything built on them) are produced by `Traverse` and
 * therefore renumber crossings and arcs, drop inactive slots, and lose the
 * distinction between `max_crossing_count` and `crossing_count`. That is
 * exactly the information one needs when debugging a routine that mutates a
 * diagram in place: which labels were recycled, which slots were deactivated,
 * whether a crossing that was supposed to stay put actually did.
 *
 * The text produced here is the same `key = value` grammar that `PrintInfo`
 * writes to the log, so a dump captured from a log file is valid input to
 * `ReadFromInString` (and, as before, is a valid Mathematica list). The
 * fields are exactly the arguments of the "construct from internal data"
 * constructor, so the round trip is lossless up to the object cache, which is
 * derived data and is not written.
 *
 * This is a debugging format, not an interchange format: the only contract is
 * that the reader and writer shipped in the *same* release agree. Anything
 * that has to survive a version change should use a PD code.
 */

[[deprecated]]
std::string InternalStateString() const
{
    std::string s;

    s += "PlanarDiagram\n";
    s += "max_crossing_count = " + Tools::ToString(max_crossing_count) + "\n";
    s += "crossing_count = "     + Tools::ToString(crossing_count)     + "\n";
    s += "max_arc_count = "      + Tools::ToString(max_arc_count)      + "\n";
    s += "arc_count = "          + Tools::ToString(arc_count)          + "\n";
    s += "C_arcs = "             + ToString(C_arcs)                    + "\n";
    s += "C_state = "            + ToString(C_state)                   + "\n";
    s += "A_cross = "            + ToString(A_cross)                   + "\n";
    s += "A_state = "            + ToString(A_state)                   + "\n";
    s += "A_color = "            + ToString(A_color)                   + "\n";
    s += "last_color_deactivated = "
         + Tools::ToString(last_color_deactivated) + "\n";
    s += "proven_minimalQ = "
         + Tools::ToString(int(proven_minimalQ)) + "\n";

    return s;
}

/*!@brief **EXPERIMENTAL** Append `InternalStateString` to an `OutString`. */

[[deprecated("Better use WriteToOutString.")]]
void WriteToOutString0( mref<Tools::OutString> s ) const
{
    const std::string str = InternalStateString();

    s.PutChars( str.data(), str.size() );
}

/*!@brief **EXPERIMENTAL** Write the internal state to a file. Returns `false` if the file
 * could not be opened.
 */

[[deprecated("Better use WriteToFile.")]]
bool WriteToFile0( cref<std::filesystem::path> file ) const
{
    std::ofstream stream;

    stream.open(file, std::ofstream::out);

    if( !stream )
    {
        Msgr::eprint("WriteToFile0", "Could not open file ", file.string(), ". Aborting.");
        return false;
    }

    stream << InternalStateString();

    return true;
}
