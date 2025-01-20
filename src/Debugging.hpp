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

    #define PD_ASSERT(c)                                                \
        if(!(c))                                                        \
        {                                                               \
            pd_eprint( "PD_ASSERT failed: " + std::string(#c) );        \
        }

    #define PD_DPRINT( s ) Tools::logprint((s));

    #define PD_TIC(s) ptic((s))

    #define PD_TOC(s) ptoc((s))

#else

    #define PD_ASSERT(c)

    #define PD_DPRINT(s)

    #define PD_TIC(s)

    #define PD_TOC(s)

#endif

namespace KnotTools
{
    Size_T PD_max_error_count =  4;
    Size_T PD_error_counter   =  0;

    
    void pd_eprint( const std::string & s )
    {
        ++KnotTools::PD_error_counter;
        
        Tools::eprint(s);
        
        if( KnotTools::PD_error_counter >= KnotTools::PD_max_error_count )
        {
            Tools::eprint("Too many errors. Aborting program.");
            
            exit(-1);
        }
    }

} // namespace KnotTools
