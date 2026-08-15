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

using Int   = std::int64_t;
using PD_T  = Knoodle::PlanarDiagram<Int>;
using PDC_T = Knoodle::PlanarDiagramComplex<Int>;

static int fails = 0;
static void check( bool ok, const char * what )
{
    std::printf("  %-64s %s\n", what, ok ? "OK" : "FAILED");
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

    std::printf("\n%s\n", fails == 0 ? "LOCK API CHECK OK" : "LOCK API CHECK FAILED");
    return fails == 0 ? 0 : 1;
}
