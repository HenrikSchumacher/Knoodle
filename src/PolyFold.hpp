#pragma once

#include <boost/program_options.hpp>
#include <exception>

#ifdef POLYFOLD_SIGNPOSTS
#include <os/signpost.h>
#endif

// TODO: Write polygon to file every xxx steps.

namespace KnotTools
{
    template<typename Real_, typename Int_, typename LInt_, typename BReal_>
    class PolyFold
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(SignedIntQ<Int_>,"");
        static_assert(SignedIntQ<LInt_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using SInt   = Int8;
        using LInt   = LInt_;
        using BReal  = BReal_;
        
        static constexpr Int AmbDim = 3;
        
        static constexpr Size_T a = 24; // Alignment for command line output.

        using Clisby_T = ClisbyTree<AmbDim,Real,Int,LInt,
            1, // use_clang_matrixQ
            1, // use_quaternionsQ
            0, // countersQ
            0, // use_manual_stackQ
            0  // collect_witnessesQ
        >;
        
        using PolygonContainer_T        = Tensor2<Real,Int>;
        using Vector_T                  = Tiny::Vector<AmbDim,Real,Int>;
//        using Link_T                    = Link_2D<Real,Int,Int,BReal>;
        using Link_T                    = Knot_2D<Real,Int,Int,BReal>;
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
        
        std::ofstream log;
        std::ofstream pds;
        
        PolygonContainer_T x;
        
        LInt total_attempt_count = 0;
        LInt total_accept_count = 0;
        LInt burn_in_attempt_count = 0;
        LInt steps_between_print = 0;
        LInt print_ctr = 0;

        std::pair<Real,Real> e_dev;

        IntersectionFlagCounts_T acc_intersection_flag_counts;
        
        TimeInterval T_run;
        
        double total_timing  = 0;
        double burn_in_time  = 0;
        double total_sampling_time = 0;
        double total_analysis_time = 0;
        
        double allocation_time = 0;
        double deallocation_time = 0;
        
        PRNG_T prng;
        PRNG_State_T prng_init { "", "", "" };

        bool prng_multiplierQ   = false;
        bool prng_incrementQ    = false;
        bool prng_stateQ        = false;
        bool force_deallocQ     = false;
        bool do_checksQ         = true;
        bool squared_gyradiusQ  = false;
        bool pdQ                = false;
        bool printQ             = false;
        
        // Witness checking
        std::ofstream witness_stream;
        std::ofstream pivot_stream;
        
    public:
        
        PolyFold() = delete;
        
        PolyFold( int argc, char** argv )
        {
            print("\nWelcome to PolyFold.\n");
            
            HandleOptions( argc, argv );
            
            Initialize<0>();
            
            Run();
            
            print("Done.");
            valprint<28>("Time elapsed during burn-in",burn_in_time);
            valprint<28>("Time elapsed during sampling",total_sampling_time);
            valprint<28>("Time elapsed during analysis",total_analysis_time);
            print(std::string(24 + 24,'-'));
            valprint<28>("Time elapsed all together",total_timing);
            
        }
        
//        PolyFold(
//            Real radius_,
//            Int n_,
//            LInt N_,
//            LInt burn_in_accept_count_,
//            LInt skip_,
//            std::filesystem::path & path_,
//            int verbosity_
//        )
//        :   radius                  ( radius_                   )
//        ,   n                       ( n_                        )
//        ,   N                       ( N_                        )
//        ,   burn_in_accept_count    ( burn_in_accept_count_     )
//        ,   skip                    ( skip_                     )
//        ,   verbosity               ( verbosity_                )
//        ,   path                    ( path_                     )
//        {
//            print("\n\nWelcome to PolyFold.\n");
//            
//            Initialize<0>();
//            
//            Run();
//        }
        
        ~PolyFold()
        {
            log << "\n|>";
        }
        
    private:

#ifdef POLYFOLD_SIGNPOSTS
        
        os_log_t log_handle = os_log_create("PolyFols", OS_LOG_CATEGORY_POINTS_OF_INTEREST);
        
        os_signpost_id_t clisby_signpost  = os_signpost_id_generate(log_handle);
        os_signpost_id_t link_signpost    = os_signpost_id_generate(log_handle);
        os_signpost_id_t pd_signpost      = os_signpost_id_generate(log_handle);
        
        os_signpost_id_t sample_signpost  = os_signpost_id_generate(log_handle);
        os_signpost_id_t analyze_signpost = os_signpost_id_generate(log_handle);
        
        void clisby_begin()
        {
            os_signpost_interval_begin(log_handle, clisby_signpost, "Clisby_T" );
        }
        
        void clisby_end()
        {
            os_signpost_interval_end( log_handle, clisby_signpost, "Clisby_T" );
        }
        
        void link_begin()
        {
            os_signpost_interval_begin( log_handle, link_signpost, "Link_T" );
        }
        
        void link_end()
        {
            os_signpost_interval_end( log_handle, link_signpost, "Link_T" );
        }
        
        void pd_begin()
        {
            os_signpost_interval_begin( log_handle, pd_signpost, "PD_T" );
        }
        
        void pd_end()
        {
            os_signpost_interval_end( log_handle, pd_signpost, "PD_T" );
        }
        
        void sample_begin()
        {
            os_signpost_interval_begin( log_handle, sample_signpost, "Sample" );
        }
        
        void sample_end()
        {
            os_signpost_interval_end( log_handle, sample_signpost, "Sample" );
        }
        
        void analyze_begin()
        {
            os_signpost_interval_begin( log_handle, analyze_signpost, "Analyze" );
        }
        
        void analyze_end()
        {
            os_signpost_interval_end( log_handle, analyze_signpost, "Analyze" );
        }

#else
        void clisby_begin() {}
        
        void clisby_end() {}
        
        void link_begin() {}
        
        void link_end() {}
        
        void pd_begin() {}
        
        void pd_end() {}
        
        void sample_begin() {}
        
        void sample_end() {}
        
        void analyze_begin() {}
        
        void analyze_end() {}
#endif
        
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
    
    public:

#include "PolyFold/Barycenter.hpp"
#include "PolyFold/SquaredGyradius.hpp"
        
        int Run()
        {
            burn_in_time = 0;
            total_sampling_time = 0;
            total_analysis_time = 0;
            total_timing = 0;
            
            switch( verbosity )
            {
                case 1:
                {
                    if( do_checksQ )
                    {
                        return Run_impl<0,1,true>();
                    }
                    else
                    {
                        return Run_impl<0,1,false>();
                    }
                    break;
                }
                case 2:
                {
                    if( do_checksQ )
                    {
                        return Run_impl<0,2,true>();
                    }
                    else
                    {
                        return Run_impl<0,2,false>();
                    }
                    break;
                }
                default:
                {
                    if( do_checksQ )
                    {
                        return Run_impl<0,0,true>();
                    }
                    else
                    {
                        return Run_impl<0,0,false>();
                    }
                    break;
                }
            }
        } // Run
        
    private:
        
        template<Size_T tab_count = 0, int my_verbosity, bool checksQ>
        int Run_impl()
        {
            T_run.Tic();
            
            BurnIn<tab_count+1,my_verbosity,checksQ>();
            
            int err = Sample<tab_count+1,my_verbosity,checksQ>();
            
            T_run.Toc();
            
            total_timing = T_run.Duration();
            
            FinalReport<tab_count+1>();
            
            if( err )
            {
                eprint(ClassName() + "::Run: Aborted because of error flag " + ToString(err) + ".");
                
                std::ofstream file ( path / "Aborted_Polygon.txt" );
                
                file << PolygonString(x);
            }
            
            return err;
            
        } // Run_impl
            
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

    
} // KnotTools

