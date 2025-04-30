#pragma once

// Fix for some warnings because boost uses std::numeric_limits<T>::infinity;  compiler migth throw warnings if we are in -ffast-math mode.
#pragma float_control(precise, on, push)
#include <boost/program_options.hpp>
#pragma float_control(pop)

#include <exception>

namespace Knoodle
{    
    template<typename Real_, typename Int_, typename LInt_, typename BReal_ = Real_>
    class PolyFold
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(SignedIntQ<Int_>,"");
        static_assert(SignedIntQ<LInt_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using LInt   = LInt_;
        using BReal  = BReal_;
        
        static constexpr Int AmbDim = 3;
        
        static constexpr Size_T a = 24; // Alignment for command line output.
        
        using Clisby_T = ClisbyTree<AmbDim,Real,Int,LInt,
            ClisbyTree_TArgs{
                .clang_matrixQ = 1, // use_clang_matrixQ
#ifdef POLYFOLD_NO_QUATERNIONS
                .quaternionsQ = 0,
#else
                .quaternionsQ = 1,
#endif
#ifdef POLYFOLD_COUNTERS
                .countersQ = 1,
#else
                .countersQ = 0,
#endif
#ifdef POLYFOLD_WITNESSES
                .witnessesQ = 1,
#else
                .witnessesQ = 0,
#endif
                .manual_stackQ = 0
            }
        >;
        
        using PolygonContainer_T        = Tensor2<Real,Int>;
        using Vector_T                  = Tiny::Vector<AmbDim,Real,Int>;
//        using Link_T                    = Link_2D<Real,Int,BReal>;
        using Link_T                    = Knot_2D<Real,Int,BReal>;
        using PD_T                      = PlanarDiagram<Int>;
        using IntersectionFlagCounts_T  = typename Link_T::IntersectionFlagCounts_T;
        using FoldFlagCounts_T          = Clisby_T::FoldFlagCounts_T;
        using PRNG_T                    = typename Clisby_T::PRNG_T;
        
        struct PRNG_State_T
        {
            std::string multiplier;
            std::string increment;
            std::string state;
        };
        
    private:
        
        Real hard_sphere_diam         = 1;
        Real hard_sphere_squared_diam = 1;
        Real prescribed_edge_length   = 1;
        
        Int  n = 1;
        LInt N = 1;
        LInt burn_in_accept_count = 1;
        LInt skip = 1;

        int verbosity = 1;
        
        std::filesystem::path path;
        std::filesystem::path log_file;
        std::filesystem::path pds_file;
        std::filesystem::path input_file;
        std::filesystem::path witness_file;
        std::filesystem::path pivot_file;
        
        std::ofstream log;
        std::ofstream pds;
        
        PolygonContainer_T x;
        
        Tensor1<LInt,Int> curvature_hist;
        Tensor1<LInt,Int> torsion_hist;
        
        Real edge_length_tolerance = 1;
        Real reflection_probability = 1;
        LInt total_attempt_count = 0;
        LInt total_accept_count = 0;
        LInt burn_in_attempt_count = 0;
        LInt steps_between_print = -1;
        LInt print_ctr = 0;
        Int  bin_count = 0;

        std::pair<Real,Real> e_dev;

        IntersectionFlagCounts_T acc_intersec_counts;
        
        TimeInterval T_run;
        
        double total_timing  = 0;
        double burn_in_time  = 0;
        double total_sampling_time = 0;
        double total_analysis_time = 0;
        double total_snapshot_time = 0;
        
        double allocation_time = 0;
        double deallocation_time = 0;
        
        PRNG_T prng;
        PRNG_State_T prng_init { "", "", "" };

        bool prng_multiplierQ   = false;
        bool prng_incrementQ    = false;
        bool prng_stateQ        = false;
        bool force_deallocQ     = false;
        bool checksQ            = true;
        bool anglesQ            = false;
        bool squared_gyradiusQ  = false;
        bool pdQ                = false;
        bool printQ             = false;
        bool inputQ             = false;
        bool hierarchicalQ      = false;
        bool bounding_boxesQ    = false;
        bool shiftQ             = false;
        bool recenterQ          = false;
        
        // Witness checking
        std::ofstream witness_stream;
        std::ofstream pivot_stream;
        
    public:
        
        PolyFold() = delete;
        
        PolyFold( int argc, char** argv )
        {
            // https://patorjk.com/software/taag/#p=testall&f=Big%20Money-ne&t=PolyFold
            
print(R"(
 .--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--. 
/ .. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \
\ \/\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ \/ /
 \/ /`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´\/ / 
 / /\   _  (`-´)                                                              _(`-´)     / /\ 
/ /\ \  \-.(OO )     .->      <-.        .->      <-.          .->      <-.  ( (OO ).-> / /\ \
\ \/ /  _.´    \(`-´)----.  ,--. )   ,--.´  ,-.(`-´)-----.(`-´)----.  ,--. )  \    .´_  \ \/ /
 \/ /  (_...--´´( OO).-.  ` |  (`-´)(`-´)´.´  /(OO|(_\---´( OO).-.  ` |  (`-´)´`´-..__)  \/ / 
 / /\  |  |_.´ |( _) | |  | |  |OO )(OO \    /  / |  ´--. ( _) | |  | |  |OO )|  |  ` |  / /\ 
/ /\ \ |  .___.´ \|  |)|  |(|  ´__ | |  /   /)  \_)  .--´  \|  |)|  |(|  ´__ ||  |  / : / /\ \
\ \/ / |  |       `  `-´  ´ |     |´ `-/   /`    `|  |_)    `  `-´  ´ |     |´|  `-´  / \ \/ /
 \/ /  `--´        `-----´  `-----´    `--´       `--´       `-----´  `-----´ `------´   \/ / 
 / /\.--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--..--./ /\ 
/ /\ \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \.. \/\ \
\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´\ `´ /
 `--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´`--´ 
)");

            try
            {
                print("");
                
                HandleOptions( argc, argv );
                
                if constexpr ( Clisby_T::countersQ )
                {
                    wprint("\nOperation counters are active. You probably do not want to use this build in production!\n");
                }
                
                if constexpr ( Clisby_T::witnessesQ )
                {
                    wprint("\nCollection of pivots and witnesses is active. You almost certainly do not want to use this build in production as it gathers A LOT of data!\n");
                }
                
                Initialize<0>();
                
                Run();
                
                print("Done.");
                valprint<30>("Time elapsed during burn-in",burn_in_time);
                valprint<30>("Time elapsed during sampling",total_sampling_time);
                valprint<30>("Time elapsed during analysis",total_analysis_time);
                valprint<30>("Time elapsed during snapshots",total_snapshot_time);
                print(std::string(26 + 24,'-'));
                valprint<30>("Time elapsed all together",total_timing);
            }
            catch( const std::exception & e )
            {
                eprint(e.what());
                throw;
            }
            catch(...)
            {
                eprint(ClassName()+"(): Unknown exception.");
                throw;
            }
        }
        
        ~PolyFold()
        {
            log << "\n|>";
        }
        
    private:

#include "PolyFold/HandleOptions.hpp"
#include "PolyFold/RandomEngine.hpp"
#include "PolyFold/Initialize.hpp"
#include "PolyFold/Helpers.hpp"
#include "PolyFold/Polygon.hpp"
#include "PolyFold/BurnIn.hpp"
#include "PolyFold/Sample.hpp"
#include "PolyFold/Analyze.hpp"
#include "PolyFold/FinalReport.hpp"
#include "PolyFold/Run.hpp"
        
    public:

#include "PolyFold/Barycenter.hpp"
#include "PolyFold/SquaredGyradius.hpp"
//#include "PolyFold/CurvatureTorsion.hpp"

    public:
        
        static std::string ClassName()
        {
            return ct_string("PolyFold")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + TypeName<BReal>
                + ">";
        }
        
    }; // PolyFold

    
} // Knoodle

