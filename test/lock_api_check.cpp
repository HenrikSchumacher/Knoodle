// lock_api_check -- exercise the parts of the locking API that had no user.
//
// `PlanarDiagramComplex(const PD_T &)` delegated to a constructor taking
// `PD_T &&`, so an lvalue could not bind and the overload could not be
// instantiated at all. Because it is a delegation inside a template member,
// that is diagnosed at the point of USE, not at the declaration -- so it sat
// there compiling fine for as long as nothing selected it, and the first thing
// that did (test/klut_identify_check) simply stopped building.
//
// That is the failure mode worth guarding against: a declaration with no user
// rots without anything going red. So this test is the missing user. It also
// covers the lock/unlock behaviour around Push, which is the operation whose
// refusal is easiest to miss -- it warns on stderr and returns, so a caller
// that does not check gets an empty complex and no error.
//
// Build: `make lock_api_check` in test/.
#include "../Knoodle.hpp"
#include <cstdio>
#include <ostream>
#include <sstream>
#include <type_traits>

using Int   = std::int64_t;
using PD_T  = Knoodle::PlanarDiagram<Int>;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;

// Detect the RETURN TYPE of `w << os` without odr-using the operator, so that
// a body which cannot compile is never instantiated. See the WideInt section
// in main() for why this has to be done at arm's length.
template<typename W, typename OS, typename = void>
struct member_shift : std::false_type { using type = void; };

template<typename W, typename OS>
struct member_shift<
    W, OS,
    std::void_t<decltype( std::declval<const W &>() << std::declval<OS &>() )>
> : std::true_type
{
    using type = decltype( std::declval<const W &>() << std::declval<OS &>() );
};

static int checks = 0;
static int fails  = 0;
static void check( bool ok, const char * what )
{
    std::printf("  %-64s %s\n", what, ok ? "OK" : "FAILED");
    ++checks;
    if( !ok ) { ++fails; }
}

int main()
{
    // ---- the const-lvalue PDC constructor is instantiable -------------------
    {
        PD_T pd = PD_T::Unknot(Int(0));
        const PD_T & ref = pd;
        PDC_T a { ref };                 // const lvalue -> the repaired overload
        PDC_T b { pd };                  // non-const lvalue -> same overload
        check(true, "PDC constructible from a const PD_T & (compiles at all)");
        check(a.DiagramCount() == b.DiagramCount(),
              "the copy overload agrees with itself");
        // It must copy, not consume: that is the difference from the PD_T &&
        // overload sitting next to it.
        check(pd.ValidQ(), "the source diagram is untouched by the copy");
    }

    // ---- lock state around Push --------------------------------------------
    {
        PDC_T pdc;
        check(pdc.LockedQ(), "a fresh complex is locked");

        // A locked complex REFUSES Push. It says so on stderr and returns; it
        // does not throw and does not stop the caller. Assert the refusal, so
        // that if the guard ever stops guarding we hear about it here.
        pdc.Push(PD_T::Unknot(Int(0)));
        check(pdc.DiagramCount() == Int(0),
              "Push on a locked complex is refused (nothing landed)");

        pdc.Unlock();
        check(!pdc.LockedQ(), "Unlock() unlocks");
        pdc.Push(PD_T::Unknot(Int(0)));
        pdc.Push(PD_T::Unknot(Int(1)));
        check(pdc.DiagramCount() == Int(2),
              "Push on an unlocked complex lands its diagrams");

        pdc.Lock();
        check(pdc.LockedQ(), "Lock() locks again");
    }

    // ---- the same triple exists on PlanarDiagram ----------------------------
    {
        PD_T pd = PD_T::Unknot(Int(0));
        check(pd.LockedQ(), "a fresh diagram is locked");
        pd.Unlock();
        check(!pd.LockedQ(), "PlanarDiagram unlocks");
        pd.Lock();
        check(pd.LockedQ(), "and locks again");
    }

    // ---- WideInt's member operator<< must be instantiable ------------------
    //
    // The same failure mode as the PDC constructor above, one header over: a
    // member template with no user, ill-formed on instantiation, compiling
    // only because nothing selects it.
    //
    //     template<typename CharT,typename Traits>
    //     std::stringstream & operator<<( mref<std::basic_ostream<CharT,Traits>&> s ) const
    //     { return s << ToString(*this); }
    //
    // `s << ToString(*this)` yields std::basic_ostream<CharT,Traits>&, and
    // binding that to the declared std::stringstream& return type is a
    // base-to-derived conversion, which never succeeds. No specialization is
    // valid, so the template is ill-formed NDR and the first caller gets a
    // hard error rather than a diagnostic anyone sees today.
    //
    // THIS TEST CANNOT BE THAT CALLER. Writing `w << std::cout` here would
    // instantiate the body and break the BUILD -- a broken tier, not a failing
    // test, and `make all` would stop before any driver ran. So the check
    // reads the declaration instead: decltype forms the call's type without
    // odr-using the function, so the body is never instantiated, and we ask
    // whether the declared return type is one the body's own expression could
    // actually initialize.
    //
    // It goes green under either repair -- removing the function (nothing left
    // to detect) or retyping the return to basic_ostream<CharT,Traits>&
    // (reachable from the body). It is red only while the defect is present.
    {
        using W  = Knoodle::WideInt<
            2,Knoodle::DefaultLimb_T,Knoodle::DefaultComp_T,false
        >;
        using OS = std::ostream;
        using D  = member_shift<W,OS>;

        if constexpr( !D::value )
        {
            check(true, "WideInt has no member operator<< (nothing to instantiate)");
        }
        else
        {
            // The body can only ever produce `OS &`; the declared return type
            // has to be initializable from that or no call can compile.
            check(std::is_convertible_v<OS &, typename D::type>,
                  "WideInt member operator<< return type is reachable from its body");
        }
    }

    // The count is what manifest.tsv's work pattern matches: a run that
    // asserted nothing must not read as a pass.
    std::printf("\n%s (%d checks, %d failed)\n",
                fails == 0 ? "LOCK API CHECK OK" : "LOCK API CHECK FAILED",
                checks, fails);
    return fails == 0 ? 0 : 1;
}
