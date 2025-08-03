#pragma once

#include "Reapr.hpp"

namespace Knoodle
{
    template<typename Real_, typename Int_>
    class PlantriSiever final
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
        using Real      = Real_;
        using Int       = Int_;
        
        using PD_T      = PlanarDiagram<Int>;
        using Reapr_T   = Reapr<Real,Int>;
        
        using CodeInt   = Reapr_T::CodeInt;
        using Code_T    = Reapr_T::Code_T;
        using CodeSet_T = Reapr_T::CodeSet_T;
        
    private:
        
        Int crossing_count = 0;
        
        CodeSet_T global_minimal_codes;
        CodeSet_T global_other_codes;
        
        Size_T thread_count;
        std::vector<CodeSet_T> thread_code_set;
        
//        Reapr_T reapr;
        
    public:
        
        PlantriSiever()
        :   thread_count    { Size_T(1) }
        ,   thread_code_set { Size_T(1) }
        {}
        
        PlantriSiever( const Size_T thread_count_ )
        :   thread_count    { thread_count_   }
        ,   thread_code_set { thread_count    }
        {}
        
        template<typename ExtInt>
        bool Reset( ExtInt crossing_count_ )
        {
            static_assert(IntQ<ExtInt>);
         
            if( int_cast<Int>(crossing_count_) > Int(64) )
            {
                eprint(MethodName("Reset")+": Only works for 64 crossings or less. Aborting.");
                
                return false;
            }
            
            crossing_count = int_cast<Int>(crossing_count_);
            
            global_minimal_codes = CodeSet_T();
            global_other_codes   = CodeSet_T();
            
            
            return true;
        }
        
        
//        template<typename ExtInt, typename ExtInt2>
//        void LoadSignedPDCodes(
//             cptr<ExtInt> input, Size_T input_count, ExtInt2 crossing_count_
//        )
//        {
//            this->template LoadPDCodes<true>(input,input_count,crossing_count_);
//        }
//        
//        template<typename ExtInt, typename ExtInt2>
//        void LoadUnsignedPDCodes(
//             cptr<ExtInt> input, Size_T input_count, ExtInt2 crossing_count_
//        )
//        {
//            this->template LoadPDCodes<false>(input,input_count,crossing_count_);
//        }
        
        template<
            bool signed_pd_codeQ,
            typename ExtInt, typename ExtInt2, typename ExtInt3
        >
        void LoadPDCodes(
             mref<Reapr_T> reapr, ExtInt2 rattle_iter,
             cptr<ExtInt> input, Size_T input_count, ExtInt3 crossing_count_
        )
        {
            static_assert(IntQ<ExtInt>);
            static_assert(IntQ<ExtInt2>);
            static_assert(IntQ<ExtInt3>);
            
            const Int iter = int_cast<Int>(rattle_iter);
            
            if( !Reset(crossing_count_) )
            {
                return;
            }
            
            
            TOOLS_DUMP(rattle_iter);
            TOOLS_DUMP(input_count);
            TOOLS_DUMP(crossing_count);
            
            for( Size_T i = 0; i < input_count; ++i )
            {
                PD_T pd;
                
                pd = this->template FromPDCode<signed_pd_codeQ>(input,i);
                
                Process(pd,reapr,iter);
            }
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
        
        
        void Process( cref<PD_T> pd_0, mref<Reapr_T> reapr, const Int rattle_iter )
        {
            const Int n = pd_0.CrossingCount();
            
            if( n > Int(64) )
            {
                eprint(MethodName("Process")+": Only works for 64 crossings or less. Aborting");
                return;
            }
            
            const UInt64 i_max = (UInt64(1) << n );
            
            Tensor1<CrossingState,Int> C_state ( n );
                    
            for( UInt64 i = 0; i < i_max; ++i )
            {
                for( Int j = 0; j < n; ++j )
                {
                    C_state [j] = get_bit(i,j)
                                ? CrossingState::RightHanded
                                : CrossingState::LeftHanded;
                }

                PD_T pd (
                    pd_0.Crossings().data(),
                    C_state.data(),
                    pd_0.Arcs().data(),
                    pd_0.ArcStates().data(),
                    n,
                    Int(0),
                    pd_0.ProvenMinimalQ()
                );
                
                auto pd_list = reapr.Rattle(pd,rattle_iter);

                if(
                    (pd_list.size() == Size_T(1))
                    &&
                    (pd_list[0].CrossingCount() == n)
                )
                {
                    bool proven_minimalQ = pd_list[0].ProvenMinimalQ();
                    
                    for( bool mirrorQ : {false,true} )
                    {
                        for( bool reverseQ : {false,true} )
                        {
                            // We collect the chirality transforms of the _original_ diagram, not the simplified one.
                            
                            PD_T pd_1 = pd.ChiralityTransform(mirrorQ,reverseQ);
                            
                            if( proven_minimalQ )
                            {
                                global_minimal_codes.insert(
                                    pd_1.template MacLeodCode<CodeInt>()
                                );
                            }
                            else
                            {
                                global_other_codes.insert(
                                    pd_1.template MacLeodCode<CodeInt>()
                                );
                            }
                        }
                    }
                }
            }
        }
        
    public:

        
        Size_T OutputCodeSize() const
        {
            return Size_T(2) * Size_T(crossing_count);
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
                code.Write( &output[output_code_size * i] );
                ++i;
            }
        }
        
        Tensor2<CodeInt,Size_T> ProvenMinimalCodes() const
        {
            Tensor2<CodeInt,Size_T> output (
                global_minimal_codes.size(), OutputCodeSize()
            );
            
            WriteProvenMinimalCodes(output.data());
            
            return output;
        }
        
        template<typename ExtInt>
        void WriteOtherCodes( mptr<ExtInt> output ) const
        {
            static_assert(IntQ<ExtInt>,"");
            
            const Size_T output_code_size = OutputCodeSize();
            
            Size_T i = 0;
            
            for( const auto & code : global_other_codes )
            {
                code.Write( &output[output_code_size * i] );
                ++i;
            }
        }
        
        
        Size_T OtherCodeCount() const
        {
            return global_other_codes.size();
        }
        
        Tensor2<CodeInt,Size_T> OtherCodes() const
        {
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
            return std::string("PlantriSiever<") + TypeName<Real> + "," + TypeName<Int> + ">";
        }
        
    }; // class PlantriSiever
    
} // namespace Knoodle
