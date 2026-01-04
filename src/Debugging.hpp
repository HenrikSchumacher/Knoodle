#pragma  once

#include <cassert>


#ifdef PD_VERBOSE
    #define PD_PRINT( s ) Tools::logprint((s));
    
    #define PD_VALPRINT( key, val ) Tools::logvalprint( (key), (val) )
    
    #define PD_WPRINT( s ) Tools::wprint((s));
    
#else
    #define PD_PRINT( s )
    
    #define PD_VALPRINT( key, val )
    
    #define PD_WPRINT( s )
#endif

#ifdef PD_DEBUG

    #define PD_ASSERT(c)                                                    \
        if(!(c))                                                            \
        {                                                                   \
            pd_eprint( "PD_ASSERT failed: " + std::string(#c) );            \
        }

    #define PD_ASSERT2(c,s)                                                 \
        if(!(c))                                                            \
        {                                                                   \
            pd_eprint( "PD_ASSERT failed: " + std::string(#c) + ": " + s ); \
        }

    #define PD_DPRINT( s ) Tools::logprint((s));

    #define PD_TIC(s) TOOLS_PTIC((s))

    #define PD_TOC(s) TOOLS_PTOC((s))

#else

    #define PD_ASSERT(c)

    #define PD_ASSERT2(c,s)

    #define PD_DPRINT(s)

    #define PD_TIC(s)

    #define PD_TOC(s)

#endif

namespace Knoodle
{
    static constexpr Size_T PD_max_error_count =  4;
    static           Size_T PD_error_counter   =  0;

    
    static inline void pd_eprint( const std::string & s )
    {
        ++Knoodle::PD_error_counter;
        
        Tools::eprint(s);
        
        if( Knoodle::PD_error_counter >= Knoodle::PD_max_error_count )
        {
            Tools::eprint("Too many errors. Aborting program.");
            
            exit(-1);
        }
    }

} // namespace Knoodle
