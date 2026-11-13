// knoodle_version.cpp -- the version and build time of one tool, as an object file.
//
// tools/Makefile compiles this once per tool, with -DGIT_VERSION=..., and links
// it in. It is the ONLY file compiled with the version on its command line, so
// a new commit recompiles this (instantly: it includes nothing) and relinks,
// where it used to change every tool's compile. tools/knoodle_io.hpp reads the
// two values below when it is compiled with -DKNOODLE_VERSION_LINKED.
//
// The build time is here for the same reason: a tool object that ccache hands
// back was compiled some other day, and __DATE__ inside it would say so.

extern const char * const knoodle_linked_version;
extern const char * const knoodle_linked_build_time;

#if defined(GIT_VERSION)
const char * const knoodle_linked_version = GIT_VERSION;
#else
const char * const knoodle_linked_version = "";
#endif

const char * const knoodle_linked_build_time = __DATE__ " " __TIME__;
