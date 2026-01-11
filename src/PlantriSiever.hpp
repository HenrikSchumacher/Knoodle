#pragma once

#include "Reapr.hpp"
#include <boost/unordered/unordered_set.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/unordered/unordered_flat_set.hpp>

namespace Knoodle
{
    template<typename Real_, typename Int_, typename CodeInt_>
    class PlantriSiever final
    {
    public:
        
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        static_assert(UnsignedIntQ<CodeInt_>,"");
        
        using Real      = Real_;
        using Int       = Int_;
        using CodeInt   = CodeInt_;
        
        using PD_T      = PlanarDiagram<Int>;
        using Reapr_T   = Reapr<Real,Int>;
        
        static constexpr int max_crossing_count = 16;
        static constexpr int code_length        = 2 * max_crossing_count;
        
        using Code_T  = std::array<CodeInt,code_length>;
//        using Code_T  = Tensor1<CodeInt,Size_T>;
        
        static_assert(max_crossing_count <= 64, "");
        static_assert(
            (max_crossing_count * sizeof(CodeInt)) % sizeof(Size_T) == 0, ""
        );

//        using CodeHash = array_hash<CodeInt,code_length>;
        
        struct CodeHash
        {
        using is_avalanching [[maybe_unused]] = std::true_type; // instruct Boost.Unordered to not use post-mixing
            
            inline Size_T operator()( cref<Code_T> v )  const
            {
                using namespace std;
                
                constexpr Size_T n = code_length * sizeof(CodeInt) / sizeof(Size_T);
                
                cptr<Size_T> w = reinterpret_cast< const Size_T *>(&v[0]);
                
                Size_T seed = 0;
                
                for( Size_T i = 0; i < n; ++i )
                {
                    Tools::hash_combine(seed,w[i]);
                }
                
                return seed;
            }
        };
        
        struct CodeLess
        {
            inline bool operator()( cref<Code_T> v, cref<Code_T> w )  const
            {
                using namespace std;
                
                constexpr Size_T n = code_length * sizeof(CodeInt) / sizeof(Size_T);
                
                cptr<Size_T> v_ = reinterpret_cast< const Size_T *>(&v[0]);
                cptr<Size_T> w_ = reinterpret_cast< const Size_T *>(&w[0]);
                
                for( Size_T i = n; i --> Size_T(0); )
                {
                    if( v_[i] < w_[i] )
                    {
                        return true;
                    }
                    else if ( v_[i] > w_[i]  )
                    {
                        return false;
                    }
                }
                
                return false;
            }
        };
        
//        using CodeSet_T = std::set<Code_T,CodeLess>;
//        using CodeSet_T = boost::container::flat_set<Code_T,CodeLess>;
//        using CodeSet_T = boost::container::set<Code_T,CodeLess>;
        
//        using CodeSet_T = std::unordered_set<Code_T,CodeHash>;
//        using CodeSet_T = boost::unordered_set<Code_T,CodeHash>;
        using CodeSet_T = boost::unordered_flat_set<Code_T,CodeHash>;
        
        
        // TODO: Find faster alternative.
    
    private:
        
        Int crossing_count = 0;
        
        CodeSet_T global_minimal_codes;
        CodeSet_T global_other_codes;
        
        Size_T thread_count;
        
        std::vector<CodeSet_T> thread_minimal_codes;
        std::vector<CodeSet_T> thread_other_codes;
        
    public:
        
        PlantriSiever()
        :   thread_count { Size_T(1) }
        {}
        
        PlantriSiever( const Size_T thread_count_ )
        :   thread_count { thread_count_   }
        {}
        
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        template<typename ExtInt>
        bool Reset( ExtInt crossing_count_ )
        {
            static_assert(IntQ<ExtInt>);
         
            if( std::cmp_greater(crossing_count_,max_crossing_count) )
            {
                eprint(MethodName("Reset")+": Only works for 64 crossings or less. Aborting.");
                
                return false;
            }
            
            crossing_count = int_cast<Int>(crossing_count_);
            
            global_minimal_codes = CodeSet_T();
            global_other_codes   = CodeSet_T();
            
            return true;
        }
        
        Size_T ThreadCount() const
        {
            return thread_count;
        }
        
        template<typename ExtInt>
        void SetThreadCount( const ExtInt val )
        {
            static_assert(IntQ<ExtInt>,"");
            thread_count = Max(Size_T(1),ToSize_T(val));
            
            thread_minimal_codes = std::vector<CodeSet_T>(thread_count);
            thread_other_codes   = std::vector<CodeSet_T>(thread_count);
        }
        
        
    private:
        
        template<bool signed_pd_codeQ, typename ExtInt>
        PD_T FromPDCode( cptr<ExtInt> input, Size_T i = Size_T(0) )
        {
            static_assert(IntQ<ExtInt>);
            if constexpr ( signed_pd_codeQ )
            {
                const Int code_size = Size_T(5) * crossing_count;
                return PD_T::FromSignedPDCode(
                    &input[code_size * i],
                    crossing_count, Int(0), true, false
                );
            }
            else
            {
                const Int code_size = Size_T(4) * crossing_count;
                return PD_T::FromUnsignedPDCode(
                    &input[code_size * i],
                    crossing_count, Int(0), true, false
                );
            }
        }
        
    public:
        
        template<
            bool signed_pd_codeQ,
            typename ExtInt,  typename ExtInt2,
            typename ExtInt3, typename ExtInt4, typename ExtInt5
        >
        void LoadPDCodes(
            mref<Reapr_T> reapr,
            ExtInt        rattle_iter_,
            cptr<ExtInt2> input,
            ExtInt3       input_count_,
            ExtInt4       crossing_count_,
            ExtInt5       thread_count_
        )
        {
            TOOLS_PTIMER(timer,MethodName("LoadPDCodes"));
            
            static_assert(IntQ<ExtInt>);
            static_assert(IntQ<ExtInt2>);
            static_assert(IntQ<ExtInt3>);
            static_assert(IntQ<ExtInt4>);
            static_assert(IntQ<ExtInt5>);
            
            if( !Reset(crossing_count_) ) { return; }
            
            SetThreadCount(thread_count_);
            
            const Size_T input_count = ToSize_T(input_count_);
            const Int rattle_iter = int_cast<Int>(rattle_iter_);
            
            const JobPointers job_ptr ( ToSize_T(input_count), thread_count );
            
//            tic("Main loop");
            ParallelDo(
                [&reapr, &job_ptr, rattle_iter, input, this]
                ( Size_T thread )
                {
                    TimeInterval thread_timer;
                    thread_timer.Tic();
                    
                    Reapr_T local_reapr = reapr;
                    local_reapr.Reseed();
                    
                    const Size_T job_begin = job_ptr[thread    ];
                    const Size_T job_end   = job_ptr[thread + 1];
                    
                    const UInt64 i_max = (UInt64(1) << crossing_count );
                    
                    PD_T::CrossingStateContainer_T C_state (crossing_count);
                    
                    CodeSet_T local_minimal_codes;
                    CodeSet_T local_other_codes;
                    
                    for( Size_T job = job_begin; job < job_end; ++job )
                    {
                        PD_T pd_0 = this->template FromPDCode<signed_pd_codeQ>(input,job);
                        
                        for( UInt64 i = 0; i < i_max; ++i )
                        {
                            for( Int j = 0; j < crossing_count; ++j )
                            {
                                C_state [j] = get_bit(i,j)
                                            ? CrossingState_T::RightHanded
                                            : CrossingState_T::LeftHanded;
                            }

                            PD_T pd (
                                pd_0.Crossings().data(),
                                C_state.data(),
                                pd_0.Arcs().data(),
                                pd_0.ArcStates().data(),
                                crossing_count,
                                Int(0), false
                            );
                            
                            auto pd_list = local_reapr.Rattle(pd,rattle_iter);
                            
                            if(
                                (pd_list.size() == Size_T(1))
                                &&
                                (pd_list[0].CrossingCount() == crossing_count)
                            )
                            {
                                bool proven_minimalQ = pd_list[0].ProvenMinimalQ();
                                
                                for( bool mirrorQ : {false,true} )
                                {
                                    for( bool reverseQ : {false,true} )
                                    {
                                        // We collect the chirality transforms of the _original_ diagram, not the simplified one.
                                        
                                        PD_T pd_1 = pd.CachelessCopy();
                                        pd_1.ChiralityTransform(mirrorQ,reverseQ);
                                        
                                        if( proven_minimalQ )
                                        {
                                            local_minimal_codes.insert(Code(pd_1));
                                        }
                                        else
                                        {
                                            local_other_codes.insert(Code(pd_1));
                                        }
                                    }
                                }
                            }
                        }
                        
                    } // for( Size_T job = job_begin; job < job_end; ++job )
                    
                    thread_minimal_codes[thread] = std::move(local_minimal_codes);
                    thread_other_codes[thread]   = std::move(local_other_codes);
                    
                    thread_timer.Toc();
                    logprint(MethodName("LoadPDCode") + ": thread " + ToString(thread) + " time = " + ToStringFPGeneral(thread_timer.Duration()) + "." );
                },
                thread_count
            );
//            toc("Main loop");
            
//            tic("Merge global_minimal_codes");
            // Take the union of all code sets of of proven minimal codes.
            global_minimal_codes = thread_minimal_codes[0];
            for( Size_T thread = 1; thread < thread_count; ++thread )
            {
                global_minimal_codes.merge(thread_minimal_codes[thread]);
//                for( auto & code : thread_minimal_codes[thread] )
//                {
//                    global_minimal_codes.insert(code);
//                }
            }
            thread_minimal_codes = std::vector<CodeSet_T>();
//            toc("Merge global_minimal_codes");
            
//            tic("Merge global_other_codes");
            // Some threads might not have proven minimality of some code, but other might. Thus, we check for each code whether it is proven minimal here.
            for( Size_T thread = 0; thread < thread_count; ++thread )
            {
                for( auto & code : thread_other_codes[thread] )
                {
                    if( global_minimal_codes.count(code) )
                    {
                        // Ignore the code.
                    }
                    else
                    {
                        global_other_codes.insert(code);
                    }
                }
            }
            thread_other_codes = std::vector<CodeSet_T>();
//            toc("Merge global_other_codes");
        }


        Size_T InputCodeSize() const
        {
            return Size_T(4) * Size_T(crossing_count);
        }
        Size_T OutputCodeSize() const
        {
            return Size_T(crossing_count);
        }
        
        
        static Code_T Code( cref<PD_T> pd )
        {
            Code_T code = { Scalar::Max<CodeInt> };
//            Code_T code ( code_length, Scalar::Max<CodeInt> );
            
            pd.template WriteMacLeodCode<CodeInt>( &code[0] );
            
            return code;
        }
        
        
        Size_T ProvenMinimalCodeCount() const
        {
            return global_minimal_codes.size();
        }

        template<typename ExtInt>
        void WriteProvenMinimalCodes( mptr<ExtInt> output ) const
        {
            static_assert(IntQ<ExtInt>,"");
            
            const Size_T output_code_size = OutputCodeSize();
            
            Size_T i = 0;
            
            for( const auto & code : global_minimal_codes )
            {
                
                copy_buffer(
                    &code[0],
                    &output[output_code_size * i],
                    output_code_size
                );
                ++i;
            }
        }
        
        Tensor2<CodeInt,Size_T> ProvenMinimalCodes() const
        {
            TOOLS_PTIMER(timer,MethodName("ProvenMinimalCodes"));
            
            Tensor2<CodeInt,Size_T> output (
                global_minimal_codes.size(), OutputCodeSize()
            );
            
            WriteProvenMinimalCodes(output.data());
            
            return output;
        }
        
        
        
        Size_T OtherCodeCount() const
        {
            return global_other_codes.size();
        }

        template<typename ExtInt>
        void WriteOtherCodes( mptr<ExtInt> output ) const
        {
            static_assert(IntQ<ExtInt>,"");
            
            const Size_T output_code_size = OutputCodeSize();
            
            Size_T i = 0;
            
            for( const auto & code : global_other_codes )
            {
                copy_buffer<code_length>(
                    &code[0],
                    &output[output_code_size * i],
                    output_code_size
                );
                ++i;
            }
        }

        Tensor2<CodeInt,Size_T> OtherCodes() const
        {
            TOOLS_PTIMER(timer,MethodName("OtherCodes"));
            
            Tensor2<CodeInt,Size_T> output (
                global_other_codes.size(), OutputCodeSize()
            );
            
            WriteOtherCodes(output.data());
            
            return output;
        }
        
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("PlantriSiever")
            + "<" + TypeName<Real>
            + "," + TypeName<Int>
            + "," + TypeName<CodeInt>
            + ">";
        }
        
    }; // class PlantriSiever
    
} // namespace Knoodle
