#pragma once

namespace Knoodle
{
    
    template<IntQ Int_ = Int64>
    class Tetrahedralization final : public CachedObject<1,0,0,0>
    {
    public:
        
        using Int = Int_;
        
        
        static constexpr int lower_bit_count    = 4;
        static constexpr Int lower_bit_mask     = Int{15};
        
        static constexpr int face_bit_count     = 2;
        static constexpr Int face_bit_mask      = Int{3};
        
        static constexpr int dedge_bit_count     = 4;
        static constexpr Int dedge_bit_mask      = Int{15};
        
        static constexpr int rotation_bit_count = 2;
        static constexpr Int rotation_bit_mask  = Int{3};
        
        static constexpr Int Uninitialized = SignedIntQ<Int> ? Int{-1}: std::numeric_limits<Int>::max();
        static constexpr Int DoNotVisit    = Uninitialized - Int{1};
        static constexpr Int Unvisited     = Uninitialized - Int{2};
        
        // We need to be able to store the lower bits.
        static constexpr Int MaxTetIndex = std::numeric_limits<Int>::max() >> (lower_bit_count + 4);
        
        static_assert(MaxTetIndex > Int{0}, "Integer type is too small.");
        
        /*!@brief Check whether an index is valid.
         */
        static constexpr bool TetIndexValidQ( const Int i )
        {
            if constexpr ( SignedIntQ<Int> )
            {
                return (i >= Int{0}) && (i <= MaxTetIndex);
            }
            else
            {
                return (i <= MaxTetIndex);
            }
        }
        
        using Class_T              = Tetrahedralization;
        
        using TetGlue_T            = Tiny::Vector<4,Int,Int>;
        
        using TetStateContainer_T  = Tensor1<bool,Int>;
        using TetGluingContainer_T = Tiny::VectorList_AoS<4,Int,Int>;
        using TetVertexContainer_T = Tiny::VectorList_AoS<4,Int,Int>;
        using TetDedgeContainer_T  = Tiny::VectorList_AoS<12,Int,Int>;
        
        struct Vertex_T;
        struct Dedge_T;
        struct Dface_T;
        struct Gluing_T;
        
#include "Tetrahedralization/Z3.hpp"
//#include "Tetrahedralization/Zmod3_lut.hpp"
#include "Tetrahedralization/LUT.hpp"
#include "Tetrahedralization/Gluing.hpp"
        
    private:
        
        Int tet_capacity = 0;
        Int tet_count = 0;
        TetStateContainer_T  tet_state;
        TetVertexContainer_T tet_glue;
//        TetVertexContainer_T tet_vert;
        
        mutable Int t_search_ptr = 0;
        
        // No unsafe operations can be performed while this flag is set to `true`.
        bool lockedQ             = true;
        
    public:
        
        Tetrahedralization() = default;
        ~Tetrahedralization() = default;
        
        Tetrahedralization( const Int tet_count_, const Int tet_capacity_ = 0 )
        :   tet_capacity  { Ramp(Max(tet_capacity_,tet_count_)) }
        ,   tet_count     { Ramp(tet_count_)                    }
        ,   tet_state     { tet_capacity                        }
        ,   tet_glue      { tet_capacity                        }
        ,   t_search_ptr  { tet_count                           }
        {
            fill_buffer( tet_state.data(), true, tet_count );
            fill_buffer( tet_glue.data(), Uninitialized, Int{4} * tet_count );
        }
        
    private:
        
        void RequireTetCapacity( const Int new_tet_capacity )
        {
            if( new_tet_capacity > tet_capacity )
            {
                SetTetCapacity( new_tet_capacity );
            }
        }
        
        void SetTetCapacity( const Int new_tet_capacity )
        {
            
            Size_T size = ToSize_T(new_tet_capacity - tet_capacity);
            
            tet_state.template Resize<true>(new_tet_capacity);
            fill_buffer( tet_state.data(tet_capacity), false, size );
            
            tet_glue.template Resize<true>(new_tet_capacity);
            fill_buffer( tet_glue.data(tet_capacity), Uninitialized, Size_T{4} * size );
            
            tet_capacity = new_tet_capacity;
        }
        
        void Expand()
        {
            SetTetCapacity(
                (tet_capacity <= Int{2}) ? Int{4} : int_cast<Int>((Size_T{3} * ToSize_T(tet_capacity))/Size_T{2})
            );
        }
        
        // TODO: Shrink() and Compress()
        
#include "Tetrahedralization/Lock.hpp"
#include "Tetrahedralization/Tets.hpp"
#include "Tetrahedralization/ModifyUnsafe.hpp"
#include "Tetrahedralization/Dfaces.hpp"
#include "Tetrahedralization/Checks.hpp"
#include "Tetrahedralization/Constructors.hpp"
#include "Tetrahedralization/Vertices.hpp"
#include "Tetrahedralization/Dedges.hpp"

    public:
        
        /*!@brief Return a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.*/
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*!@brief Return a string that identifies this class with type information. Mostly used for logging and in error messages.*/
        static constexpr auto ClassName()
        {
            return ct_string("PlanarDiagram")
                + "<" + TypeName<Int>
                + ">";
        }
        
    }; // class Tetrahedralization
    
} // namespace Knoodle
