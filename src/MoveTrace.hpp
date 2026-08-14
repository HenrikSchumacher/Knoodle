#pragma once

#include <charconv>
#include <cstddef>
#include <istream>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Knoodle
{
    /*!@brief Reader for the move-trace record streams of
     * `docs/move-descriptor.md`.
     *
     * A trace is a sequence of records separated by blank lines. Each record
     * is a run of `#`-headed lines carrying a snapshot of a diagram and,
     * usually, a move descriptor written against that snapshot. It is the
     * format a simplifier emits to say what it did, and the format
     * `knoodledraw --trace` renders and checks.
     *
     * This lives in `src/`, not in a tool, for one reason: it is a
     * **contract between two programs**, and the whole class of bug it has to
     * rule out is two implementations of the same grammar disagreeing. A
     * writer and a reader that call the same header get that for free. Any
     * simplifier -- `PassSimplifier`, a downstream one, one merged in later --
     * should emit through this, never through its own printf.
     *
     * ## The two carriers
     *
     * **v0** carries the snapshot as a 5-column signed PD code. That is a
     * fine archival format and a poor comparison format: `PDCode()` renumbers
     * everything, so a reader that wants to check a claim about the snapshot
     * can only ask label-free questions of it.
     *
     * **v1** carries the snapshot as `PlanarDiagram::WriteToOutString` output
     * in a `#state lines=N` block. That is label-preserving, so a crossing
     * the move promised not to touch has the *same index* on both sides of
     * the exchange, and a disagreement can name a crossing and a port instead
     * of only "different knot". `#result lines=M` optionally carries what the
     * emitter's own applier produced, which is what makes the cross-process
     * comparison possible at all.
     *
     * `lines=N` is an explicit count rather than a self-delimiting block, so
     * a field added to the state format upstream changes N and a reader that
     * knows nothing about the layout still slices correctly -- or fails loud.
     *
     * ## Fail loud
     *
     * A `#state` block that will not parse is an error, never a fall-back to
     * the `#pd` annotation: the annotation is redundant by construction, and
     * silently preferring it would turn a version skew into a wrong answer.
     *
     * When a record carries BOTH, reconciling them is the consumer's job and
     * the comparison can only be **up to relabelling** -- a PD code renumbers
     * on the way out, so `PDCode(state)` need not be the annotation's bytes
     * even when both are right, and a literal comparison would fire on
     * perfectly good records. Identifying the diagram up to relabelling is
     * exactly what the spec asks the annotation to do, and no more.
     */

    template<class PD_T_>
    class MoveTrace
    {
    public:

        using PD_T = PD_T_;
        using Int  = typename PD_T::Int;

        /*!@brief One record of a trace stream. */
        struct Record
        {
            // Header lines verbatim and in order, for captioning. Stream-level
            // headers (`#trace`, `#knoodle`) are not part of any record.
            std::vector<std::string> headers;

            // Payload of `#move ` (the header line minus that token), if any.
            // Terminal records carry no move.
            std::optional<std::string> move;

            // `#view exterior=<da>`.
            std::optional<Int> exterior_da;

            // `#candidate`: evaluated but not applied. The diagram does NOT
            // advance across such a record, so a consumer checking that a
            // move produces the next snapshot must not carry this one's claim
            // forward.
            bool candidateQ = false;

            // `#spinoffs`: crossingless components the emitter's applier split
            // off. A `PlanarDiagram` cannot hold one beside crossings, so they
            // are reported rather than represented -- on both sides.
            std::optional<Int> spinoffs;
            std::vector<Int>   spinoff_colors;

            // The before diagram: `#state` in v1, the PD rows in v0.
            std::optional<PD_T> state;

            // What the emitter's applier produced (`#result`), if supplied.
            std::optional<PD_T> result;

            // The `#pd` annotation, flat, five entries per row.
            std::vector<Int> pd_rows;

            // True when `state` was BUILT from `pd_rows` -- the v0 carrier, or
            // a v1 record that shipped only the annotation. When false and
            // `pd_rows` is non-empty the record carries both, and the two are
            // the consumer's to reconcile (see the note on `#pd` above).
            bool state_from_pd = false;

            // Line number the record started on, for diagnostics.
            std::size_t line = 0;

            bool EmptyQ() const
            {
                return headers.empty() && !state && pd_rows.empty();
            }
        };

        enum class Status
        {
            Record,   // a record was read
            Eof,      // the stream ended cleanly
            Error     // `why` says what went wrong, at `LineNo()`
        };

        /*!@brief Pull-based record reader over an input stream. */
        class Reader
        {
        public:

            explicit Reader( std::istream & in ) : in_(in) {}

            /*!@brief Read the next record. */
            Status Next( Record & rec, std::string & why )
            {
                rec = Record();

                std::string line;
                bool in_record = false;

                while( std::getline(in_,line) )
                {
                    ++line_no_;
                    Chomp(line);

                    if( line.empty() )
                    {
                        if( in_record ) { return Finish(rec,why); }
                        continue;
                    }

                    if( line[0] == '#' )
                    {
                        // ---- stream-level headers, not part of a record ----
                        if( line.starts_with("#trace") )
                        {
                            if( !ParseVersion(line,why) ) { return Status::Error; }
                            continue;
                        }
                        if( line.starts_with("#knoodle") )
                        {
                            knoodle_version_ = ValueOf(line,"version=");
                            continue;
                        }

                        if( !in_record ) { rec.line = line_no_; in_record = true; }

                        if( !ReadHeader(line,rec,why) ) { return Status::Error; }
                        continue;
                    }

                    // ---- a bare 5-column signed PD row (the v0 snapshot) ----
                    if( !in_record ) { rec.line = line_no_; in_record = true; }

                    if( !ReadPDRow(line,rec.pd_rows,why) ) { return Status::Error; }
                }

                if( in_record ) { return Finish(rec,why); }

                return Status::Eof;
            }

            Int                 Version()        const { return version_;         }
            const std::string & KnoodleVersion() const { return knoodle_version_; }
            std::size_t         LineNo()         const { return line_no_;         }

        private:

            std::istream & in_;
            std::size_t    line_no_        = 0;
            Int            version_        = Int(0);
            std::string    knoodle_version_;

            static void Chomp( std::string & s )
            {
                while( !s.empty() && ((s.back() == '\r') || (s.back() == ' ')) )
                {
                    s.pop_back();
                }
            }

            static bool ParseInt( std::string_view tok, Int & out )
            {
                std::int64_t v = 0;
                auto [p,ec] = std::from_chars(tok.data(),tok.data()+tok.size(),v);
                if( (ec != std::errc{}) || (p != tok.data() + tok.size()) )
                {
                    return false;
                }
                out = static_cast<Int>(v);
                return true;
            }

            /*!@brief The whitespace-delimited value of `key` in `line`. */
            static std::string ValueOf( const std::string & line,
                                        std::string_view    key )
            {
                const auto pos = line.find(key);
                if( pos == std::string::npos ) { return std::string(); }

                const auto beg = pos + key.size();
                auto       end = line.find(' ',beg);
                if( end == std::string::npos ) { end = line.size(); }

                return line.substr(beg,end-beg);
            }

            static bool FieldInt( const std::string & line,
                                  std::string_view    key,
                                  Int &               out )
            {
                const std::string v = ValueOf(line,key);
                return !v.empty() && ParseInt(v,out);
            }

            bool ParseVersion( const std::string & line, std::string & why )
            {
                Int v = Int(0);
                if( !FieldInt(line,"v=",v) )
                {
                    why = "bad '#trace' header '" + line + "': want v=<n>";
                    return false;
                }
                if( (v < Int(0)) || (v > Int(1)) )
                {
                    // Refusing beats guessing: a later version may move the
                    // snapshot again, and a reader that carries on would be
                    // reporting confident nonsense.
                    why = "trace version v=" + std::to_string(v) + " is newer"
                          " than this reader understands (v=0 and v=1)";
                    return false;
                }
                version_ = v;
                return true;
            }

            /*!@brief Read exactly `n` further lines verbatim. */
            bool ReadBlock( Int n, std::string & text, std::string & why )
            {
                text.clear();
                std::string line;

                for( Int i = 0; i < n; ++i )
                {
                    if( !std::getline(in_,line) )
                    {
                        why = "EOF after " + std::to_string(i) + " of "
                            + std::to_string(n) + " block lines";
                        return false;
                    }
                    ++line_no_;
                    while( !line.empty() && (line.back() == '\r') )
                    {
                        line.pop_back();
                    }
                    text += line;
                    text += '\n';
                }
                return true;
            }

            /*!@brief Read a `lines=N` block and parse it as internal state. */
            bool ReadStateBlock( const std::string &   line,
                                 std::optional<PD_T> & out,
                                 const char *          what,
                                 std::string &         why )
            {
                Int n = Int(0);
                if( !FieldInt(line,"lines=",n) || (n < Int(0)) )
                {
                    why = std::string("bad '") + what + "' header '" + line
                        + "': want lines=<n>";
                    return false;
                }

                std::string text;
                if( !ReadBlock(n,text,why) )
                {
                    why = std::string(what) + ": " + why;
                    return false;
                }

                Tools::InString s ( text );
                PD_T pd = PD_T::FromInString(s);

                if( pd.InvalidQ() )
                {
                    why = std::string(what) + " block did not parse as"
                          " PlanarDiagram internal state (see the message"
                          " above). The '#pd' annotation is never a fall-back.";
                    return false;
                }

                out = std::move(pd);
                return true;
            }

            static bool ReadPDRow( const std::string & line,
                                   std::vector<Int> &  rows,
                                   std::string &       why )
            {
                std::size_t pos   = 0;
                int         count = 0;

                while( pos < line.size() )
                {
                    while( (pos < line.size())
                        && ((line[pos] == ' ') || (line[pos] == '\t')) )
                    {
                        ++pos;
                    }
                    if( pos >= line.size() ) { break; }

                    auto end = pos;
                    while( (end < line.size())
                        && (line[end] != ' ') && (line[end] != '\t') )
                    {
                        ++end;
                    }

                    Int v = Int(0);
                    const std::string_view tok (line.data()+pos,end-pos);
                    if( !ParseInt(tok,v) )
                    {
                        why = "bad PD entry '" + std::string(tok) + "'";
                        return false;
                    }
                    rows.push_back(v);
                    ++count;
                    pos = end;
                }

                if( count != 5 )
                {
                    why = "PD row has " + std::to_string(count)
                        + " columns, want 5";
                    return false;
                }
                return true;
            }

            bool ReadHeader( const std::string & line,
                             Record &            rec,
                             std::string &       why )
            {
                if( line.starts_with("#state") )
                {
                    if( rec.state )
                    {
                        why = "record carries a second '#state' block";
                        return false;
                    }
                    return ReadStateBlock(line,rec.state,"#state",why);
                }

                if( line.starts_with("#result") )
                {
                    if( rec.result )
                    {
                        why = "record carries a second '#result' block";
                        return false;
                    }
                    // Not echoed, for the same reason `#state` is not: the
                    // block headers are carrier plumbing, and a caption above
                    // a drawing should say what the record MEANS.
                    return ReadStateBlock(line,rec.result,"#result",why);
                }

                if( line.starts_with("#pd") )
                {
                    Int rows = Int(0);
                    if( !FieldInt(line,"rows=",rows) || (rows < Int(0)) )
                    {
                        why = "bad '#pd' header '" + line + "': want rows=<n>";
                        return false;
                    }
                    rec.headers.push_back(line);

                    std::string row;
                    for( Int r = 0; r < rows; ++r )
                    {
                        if( !std::getline(in_,row) )
                        {
                            why = "EOF inside '#pd' block";
                            return false;
                        }
                        ++line_no_;
                        Chomp(row);
                        if( !ReadPDRow(row,rec.pd_rows,why) ) { return false; }
                    }
                    return true;
                }

                if( line.starts_with("#embedding") )
                {
                    // A redraw witness. Rendering the lift/rotate/flatten
                    // animation is a later backend's job; skip the rows.
                    Int rows = Int(0);
                    if( !FieldInt(line,"rows=",rows) || (rows < Int(0)) )
                    {
                        why = "bad '#embedding' header '" + line + "'";
                        return false;
                    }
                    std::string text;
                    if( !ReadBlock(rows,text,why) ) { return false; }

                    rec.headers.push_back("#embedding (" + std::to_string(rows)
                        + " rows, not rendered)");
                    return true;
                }

                rec.headers.push_back(line);

                if( line.starts_with("#move ") )
                {
                    rec.move = line.substr(6);
                }
                else if( line.starts_with("#view ") )
                {
                    Int da = Int(0);
                    if( !FieldInt(line,"exterior=",da) )
                    {
                        why = "bad '#view' header '" + line + "'";
                        return false;
                    }
                    rec.exterior_da = da;
                }
                else if( line.starts_with("#candidate") )
                {
                    rec.candidateQ = true;
                }
                else if( line.starts_with("#spinoffs") )
                {
                    // Two spellings are in flight between the two sides:
                    // `n=<count>` and `colors=<c1,c2,...>`. Both say the same
                    // thing, and refusing one on grammar grounds would only
                    // stall a check that works.
                    const std::string colors = ValueOf(line,"colors=");
                    if( !colors.empty() )
                    {
                        std::size_t pos = 0;
                        while( pos <= colors.size() )
                        {
                            auto end = colors.find(',',pos);
                            if( end == std::string::npos ) { end = colors.size(); }

                            Int c = Int(0);
                            const std::string_view tok (colors.data()+pos,end-pos);
                            if( tok.empty() || !ParseInt(tok,c) )
                            {
                                why = "bad '#spinoffs' colour list '" + colors + "'";
                                return false;
                            }
                            rec.spinoff_colors.push_back(c);
                            pos = end + 1;
                        }
                        rec.spinoffs = static_cast<Int>(rec.spinoff_colors.size());
                    }
                    else
                    {
                        Int n = Int(0);
                        if( !FieldInt(line,"n=",n) || (n < Int(0)) )
                        {
                            why = "bad '#spinoffs' header '" + line
                                + "': want n=<count> or colors=<list>";
                            return false;
                        }
                        rec.spinoffs = n;
                    }
                }

                return true;
            }

            /*!@brief Reconcile the snapshot carriers and hand the record over. */
            Status Finish( Record & rec, std::string & why )
            {
                const Int rows = static_cast<Int>(rec.pd_rows.size()) / Int(5);

                if( rec.state )
                {
                    // A record carrying both carriers is left to the consumer
                    // to reconcile: the comparison can only be up to
                    // relabelling (see the note on `#pd` above), and the
                    // machinery for that is a verifier's, not a parser's.
                }
                else if( rows > Int(0) )
                {
                    PD_T pd = PD_T::FromSignedPDCode(rec.pd_rows.data(),rows);
                    if( pd.CrossingCount() <= Int(0) )
                    {
                        why = "PD snapshot did not parse into a valid diagram";
                        return Status::Error;
                    }
                    rec.state        = std::move(pd);
                    rec.state_from_pd = true;
                }
                else if( version_ >= Int(1) )
                {
                    // In v1 the snapshot is mandatory, so a record without one
                    // is a truncated stream, not a 0-crossing summand.
                    why = "v1 record carries no '#state' block";
                    return Status::Error;
                }

                return Status::Record;
            }

        }; // class Reader

        /*!@brief Write `pd` as a `#state`/`#result`-style block.
         *
         * The counterpart of `ReadStateBlock`, here so that an emitter and a
         * reader in the same release cannot drift: the header's line count is
         * computed from the very bytes it introduces.
         */
        static bool WriteStateBlock( const PD_T &  pd,
                                     const char *  header,
                                     std::string & out )
        {
            Tools::OutString s;

            if( !pd.WriteToOutString(s) ) { return false; }

            std::string text ( s.begin(), static_cast<std::size_t>(s.Size()) );

            if( text.empty() || (text.back() != '\n') ) { text += '\n'; }

            std::size_t lines = 0;
            for( char ch : text ) { lines += (ch == '\n'); }

            out  = std::string(header) + " lines=" + std::to_string(lines) + "\n";
            out += text;

            return true;
        }

    }; // class MoveTrace

} // namespace Knoodle
