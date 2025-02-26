#pragma once

#include <boost/program_options.hpp>
#include <exception>

// TODO: Write polygon to file every xxx steps.
// TODO: Removed 0-initialization in Clisby tree.

namespace KnotTools
{
    template<typename Real_, typename Int_, typename LInt_, typename BReal_>
    class PolyFold
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(IntQ<LInt_>,"");
        static_assert(FloatQ<BReal_>,"");
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using SInt   = Int8;
        using LInt   = LInt_;
        using BReal  = BReal_;
        
        static constexpr Int  AmbDim = 3;

        using Vector_T                  = Tiny::Vector<AmbDim,Real,Int>;
        using Clisby_T                  = ClisbyTree<AmbDim,Real,Int,LInt>;
        using Link_T                    = Link_2D<Real,Int,Int,BReal>;
        using PD_T                      = PlanarDiagram<Int>;
        using PRNG_T                    = typename Clisby_T::PRNG_T;
        using PRNG_FullState_T          = typename Clisby_T::PRNG_FullState_T;
        using IntersectionFlagCounts_T  = typename Link_T::IntersectionFlagCounts_T;
        
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
        
        Tensor2<Real,Int> x;
        
        LInt total_attempt_count = 0;
        LInt total_accept_count = 0;
        LInt burn_in_attempt_count = 0;

        std::pair<Real,Real> e_dev;

        IntersectionFlagCounts_T acc_intersection_flag_counts;
        
        std::vector<PD_T> PD_list;
        
        TimeInterval T_run;
        
        double total_timing  = 0;
        double burn_in_time  = 0;
        double total_sampling_time = 0;
        double total_analysis_time = 0;
        
        
        PRNG_T random_engine;
        
        PRNG_FullState_T random_engine_state {
            "47026247687942121848144207491837523525",
            "332379478798780862241265428819319690027",
            "91245575879834525841473801137169063431"
        };

        bool random_engine_multiplierQ = false;
        bool random_engine_incrementQ  = false;
        bool random_engine_stateQ      = false;
        bool force_deallocQ            = false;
        bool squared_gyradiusQ         = false;
        bool pdQ                       = false;

        
    public:
        
        PolyFold() = delete;
        
        PolyFold( int argc, char** argv )
        {
            print("\n\nWelcome to PolyFold.\n");
            
            HandleOptions( argc, argv );

            Initialize<0>();

            Run();
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

#include "PolyFold/HandleOptions.hpp"
#include "PolyFold/Initialize.hpp"
#include "PolyFold/Helpers.hpp"
#include "PolyFold/BurnIn.hpp"
#include "PolyFold/Sample.hpp"
#include "PolyFold/Analyze.hpp"
#include "PolyFold/FinalReport.hpp"
    
    public:

#include "PolyFold/Barycenter.hpp"
#include "PolyFold/SquaredGyradius.hpp"
        
        template<Size_T tab_count = 0>
        int Run()
        {
            T_run.Tic();
            
            switch( verbosity )
            {
                case 1:
                {
                    BurnIn<tab_count+1,1>();
                    break;
                }
                case 2:
                {
                    BurnIn<tab_count+1,2>();
                    break;
                }
                default:
                {
                    BurnIn<tab_count+1,0>();
                    break;
                }
            }
            
            int err;
            
            switch( verbosity )
            {
                case 1:
                {
                    err = Sample<tab_count+1,1>();
                    break;
                }
                case 2:
                {
                    err = Sample<tab_count+1,2>();
                    break;
                }
                default:
                {
                    err = Sample<tab_count+1,0>();
                    break;
                }
            }
            
            T_run.Toc();
            
            total_timing = T_run.Duration();
            
            FinalReport<tab_count+1>();
            
            if( err )
            {
                eprint(ClassName() + "::Run: Aborted because of error flag " + ToString(err) + ".");
                
                std::ofstream file ( path / "Aborted_Polygon.txt" );
                
                file << x;
            }
            
            return err;
            
        } // Run
            
    public:
        
        static std::string ClassName()
        {
            return std::string("PolyFold")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + "," + TypeName<LInt>
                + "," + TypeName<BReal>
                + ">";
        }
        
    }; // PolyFold

    
} // KnotTools

