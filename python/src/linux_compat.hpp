#ifndef LINUX_COMPAT_HPP
#define LINUX_COMPAT_HPP

#include <cstdint>
#include <cstddef>


#ifndef FastInt8
typedef std::int8_t FastInt8;
#endif

#ifndef FastInt16
typedef std::int16_t FastInt16;
#endif

#ifndef FastInt32
typedef std::int32_t FastInt32;
#endif

#ifndef FastInt64
typedef std::int64_t FastInt64;
#endif

#ifndef TOOLS_PTIMER
#define TOOLS_PTIMER(name, description) do {} while(0)
#endif


#ifndef TOOLS_MAKE_FP_FAST
#define TOOLS_MAKE_FP_FAST() do {} while(0)
#endif

#ifndef TOOLS_MAKE_FP_STRICT
#define TOOLS_MAKE_FP_STRICT() do {} while(0)
#endif

#endif 