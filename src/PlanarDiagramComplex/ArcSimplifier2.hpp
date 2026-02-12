#pragma once


/*! TODO: Rewrite a_is_2loop:
 *  We can either
 *      - split an unlink and disconnect-sum a knot or
 *      - disconnect-sum a Hopf link and a knot.
 */

// TODO: Add counters.

namespace Knoodle
{
    template<typename Int> class PlanarDiagramComplex;

    template<
        typename Int_,
        UInt8 optimization_level_ = 4,
        bool mult_compQ_          = true,
        bool forwardQ_            = true
    >
    class alignas( ObjectAlignment ) ArcSimplifier2 final
    {
    public:
        
        using Int  = Int_;
        
        using PDC_T = PlanarDiagramComplex<Int>;
        using PD_T  = PDC_T::PD_T;
        
        using CrossingContainer_T       = typename PD_T::CrossingContainer_T;
        using CrossingStateContainer_T  = typename PD_T::CrossingStateContainer_T;
        using ArcContainer_T            = typename PD_T::ArcContainer_T;
        using ArcColorContainer_T       = typename PD_T::ArcColorContainer_T;
        using ArcStateContainer_T       = typename PD_T::ArcStateContainer_T;
        
        static constexpr bool  mult_compQ          = mult_compQ_;
        static constexpr bool  forwardQ            = forwardQ_;
        static constexpr UInt8 optimization_level  = optimization_level_;
        
        static constexpr bool  search_two_triangles_same_u = true;
        
        static constexpr bool  use_loop_removerQ    = true;
        
        static constexpr bool  Head  = PD_T::Head;
        static constexpr bool  Tail  = PD_T::Tail;
        static constexpr bool  Left  = PD_T::Left;
        static constexpr bool  Right = PD_T::Right;
        static constexpr bool  In    = PD_T::In;
        static constexpr bool  Out   = PD_T::Out;
        
        struct Settings_T
        {
            Size_T max_iter              = Scalar::Max<Size_T>;
            Int    compression_threshold = 0;
            bool   compressQ             = true;
        };
        
        friend std::string ToString( cref<Settings_T> settings_ )
        {
            return std::string("{ ")
                    +   ".max_iter = " + ToString(settings_.max_iter)
                    + ", .compression_threshold = " + ToString(settings_.compression_threshold)
                    + ", .compressQ = " + ToString(settings_.compressQ)
            + " }";
        }
        
    private:
        
        PDC_T & restrict pdc;
        PD_T  & restrict pd;
        
        CrossingContainer_T      & restrict C_arcs;
        CrossingStateContainer_T & restrict C_state;
    
        ArcContainer_T           & restrict A_cross;
        ArcStateContainer_T      & restrict A_state;
        ArcColorContainer_T      & restrict A_color;
        
        Settings_T settings;
        
    private:
        
        Int a;
        Int c_0;
        Int c_1;
        Int c_2;
        Int c_3;
        
        // For crossing c_0
        Int n_0;
        //        Int e_0; // always = a.
        Int s_0;
        Int w_0;
        
        // For crossing c_1
        Int n_1;
        Int e_1;
        Int s_1;
        //        Int w_1; // always = a.
        
        // For crossing c_2
        //        Int n_2; // always = s_0.
        Int e_2;
        Int s_2;
        Int w_2;
        
        // For crossing c_3
        Int n_3;
        Int e_3;
        //        Int s_3; // always = n_0.
        Int w_3;
        
        Size_T test_count = 0;

        // Whether the crossing is right-handed
        CrossingState_T c_0_state;
        CrossingState_T c_1_state;
        CrossingState_T c_2_state;
        CrossingState_T c_3_state;
        
        // Whether the vertical strand at corresponding crossing goes over.
        bool o_0;
        bool o_1;
        bool o_2;
        bool o_3;
        
        // Whether the vertical strand at corresponding crossing goes up.
        bool u_0;
        bool u_1;
        
    public:
        
        ArcSimplifier2( mref<PDC_T> pdc_, mref<PD_T> pd_, cref<Settings_T> settings_ )
        :   pdc      { pdc_       }
        ,   pd       { pd_        }
        ,   C_arcs   { pd.C_arcs  }
        ,   C_state  { pd.C_state }
        ,   A_cross  { pd.A_cross }
        ,   A_state  { pd.A_state }
        ,   A_color  { pd.A_color }
        ,   settings { settings_  }
        {}
        
        // Default constructor
        ArcSimplifier2() = delete;
        
    public:

        Size_T operator()()
        {
            if( pd.InvalidQ() ) { return 0; }
            
            if( pd.ProvenMinimalQ() ) { return 0; }
            
            if( pd.crossing_count <= Int(1) )
            {
                pdc.CreateUnlink(pd.last_color_deactivated);
                pd = PD_T::InvalidDiagram();
                return 0;
            }
            
            TOOLS_PTIMER(timer,ClassName() + "(" + ToString(settings) + ")");
            
            Size_T old_counter = 0;
            Size_T counter = 0;
            Size_T iter = 0;
            
            if( settings.compressQ ) { pd.ConditionalCompress( settings.compression_threshold ); }
            
            if( iter < settings.max_iter )
            {
                do
                {
                    ++iter;
                    old_counter = counter;
                    
                    const Int max_arc_count = pd.MaxArcCount();
                   
                    for( Int arc = 0; arc < max_arc_count; ++arc )
                    {
                        if( pd.arc_count <= Int(0) ) { break; }
                        
                        counter += ProcessArc(arc);
                    }
                    
                    if( pd.ArcCount() <= Int(0) ) { break; }
        
                    // We could recompress also here...
                    if( settings.compressQ ) { pd.ConditionalCompress( settings.compression_threshold ); }
                }
                while( (counter != old_counter) && (iter < settings.max_iter) );
            }
            
            if( pd.InvalidQ() ) { return counter; }
            
            if( counter > Size_T(0) )
            {
                if( settings.compressQ ) { pd.ConditionalCompress( settings.compression_threshold ); }
                
                pd.ClearCache();
            }
                
            if( pd.ValidQ() && (pd.CrossingCount() == Int(0)) )
            {
                pd.proven_minimalQ = true;
            }
            
            return counter;
        }
        
        Size_T TestCount() const
        {
            return test_count;
        }
        
        void CreateUnlinkFromArc( const Int a_ )
        {
//            PD_NOTE(MethodName("CreateUnlinkFromArc")+" (crossing_count = " + ToString(pd.CrossingCount()) + ")");
            pdc.CreateUnlinkFromArc(pd,a_);
        }
        
        void CreateHopfLinkFromArcs(
            const Int a_0, const Int a_1, const CrossingState_T handedness
        )
        {
//            PD_NOTE(MethodName("CreateHopfLinkFromArcs")+" (crossing_count = "  + ToString(pd.CrossingCount()) + ")");
            pdc.CreateHopfLinkFromArcs(pd,a_0,a_1,handedness);
        }
        
        void CreateTrefoilKnotFromArc( const Int a_, const CrossingState_T handedness )
        {
//            PD_NOTE(MethodName("CreateTrefoilKnotFromArc")+" (crossing_count = "  + ToString(pd.CrossingCount()) + ")");
            pdc.CreateTrefoilKnotFromArc(pd,a_,handedness);
        }
        
        void CreateFigureEightKnotFromArc( const Int a_ )
        {
//            PD_NOTE(MethodName("CreateFigureEightKnotFromArc")+" (crossing_count = "  + ToString(pd.CrossingCount()) + ")");
            pdc.CreateFigureEightKnotFromArc(pd,a_);
        }
        
    private:
  
#include "ArcSimplifier/Helpers.hpp"
#include "ArcSimplifier/ProcessArc.hpp"
#include "ArcSimplifier/load.hpp"
#include "ArcSimplifier/a_is_2loop.hpp"
#include "ArcSimplifier/twist_at_a.hpp"
#include "ArcSimplifier/strands_same_o.hpp"
#include "ArcSimplifier/strands_diff_o.hpp"
#include "ArcSimplifier/R_I_center.hpp"
#include "ArcSimplifier/R_I_left.hpp"
#include "ArcSimplifier/R_I_right.hpp"
#include "ArcSimplifier/R_Ia_above.hpp"
#include "ArcSimplifier/R_Ia_below.hpp"
#include "ArcSimplifier/R_II_above.hpp"
#include "ArcSimplifier/R_II_below.hpp"
#include "ArcSimplifier/two_triangles_same_u.hpp"
#include "ArcSimplifier/R_IIa_same_o_same_u.hpp"
#include "ArcSimplifier/R_IIa_same_o_diff_u.hpp"
#include "ArcSimplifier/R_IIa_diff_o_same_u.hpp"
#include "ArcSimplifier/R_IIa_diff_o_diff_u.hpp"
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("ArcSimplifier2")
                + "<" + TypeName<Int>
                + "," + ToString(optimization_level)
                + "," + ToString(mult_compQ)
                + "," + ToString(forwardQ) + ">";
        }

    }; // class ArcSimplifier
}
