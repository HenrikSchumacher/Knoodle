#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: check EdgeTurns;
    // TODO: What to do with multiple diagram components?
    
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    

    
    template<typename Int_>
    class OrthogonalRepresentation2 final : CachedObject
    {
    private:
        
        using Base_T = CachedObject;
        
    public:
        static_assert(SignedIntQ<Int_>,"");
        
        using Int        = Int_;
        using COIN_Real  = double;
        using COIN_Int   = int;
        using COIN_LInt  = CoinBigIndex;
        using UInt       = UInt32;
        using Dir_T      = UInt8;
        using EdgeFlag_T = UInt8;
        using Turn_T     = std::make_signed_t<Int>;
        using Cost_T     = std::make_signed_t<Int>;
        
        
        enum class VertexFlag_T : Int8
        {
            Inactive    =  0,
            RightHanded =  1,
            LeftHanded  = -1,
            Corner      =  2
        };
        
        struct Settings_T
        {
            bool pm_matrixQ           = true;
            bool redistribute_bendsQ      = false;
            bool use_dual_simplexQ        = false;
            bool turn_regularizeQ         = true;
            bool saturate_facesQ          = true;
            bool saturate_exterior_faceQ  = false;
            int  compaction_method        = 0;
            
            
            Int  x_grid_size              = 20;
            Int  y_grid_size              = 20;
            
            Int  x_gap_size               =  4;
            Int  y_gap_size               =  4;
        };
        
        using DiGraph_T             = MultiDiGraph<Int,Int>;
        using HeadTail_T            = DiGraph_T::HeadTail_T;
        using VertexContainer_T     = Tiny::VectorList_AoS<4,Int,Int>;
        using EdgeContainer_T       = DiGraph_T::EdgeContainer_T;
        using EdgeTurnContainer_T   = Tiny::VectorList_AoS<2,Turn_T,Int>;
        using CoordsContainer_T     = Tiny::VectorList_AoS<2,Int,Int>;
        using VertexFlagContainer_T = Tensor1<VertexFlag_T,Int>;
        using EdgeFlagContainer_T   = Tiny::VectorList_AoS<2,EdgeFlag_T,Int>;
        using Vector_T              = Tiny::Vector<2,Int,Int>;
        using PlanarDiagram_T       = PlanarDiagram<Int>;
        
        using ArcSplineContainer_T  = RaggedList<std::array<Int,2>,Int>;
        
        using COIN_Matrix_T = Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt>;
        using COIN_Agg_T = TripleAggregator<COIN_Int,COIN_Int,COIN_Real,COIN_LInt>;

        
//        enum class EdgeState : UInt8
//        {
//            Inactive    = 0,
//            Active      = 1,
//            Virtual     = 2
//        };
        
        static constexpr Int Uninitialized = PlanarDiagram_T::Uninitialized;
        static constexpr Int MaxValidIndex = PlanarDiagram_T::MaxValidIndex;
        
        static constexpr bool ValidIndexQ( const Int i )
        {
            return PlanarDiagram_T::ValidIndexQ(i);
        }
        
    private:
        
        static constexpr Int ToDarc( const Int a, const bool d )
        {
            return PlanarDiagram_T::ToDarc(a,d);
        }
        
        template<bool d>
        static constexpr Int ToDarc( const Int a )
        {
            return PlanarDiagram_T::ToDarc<d>(a);
        }
        
        static constexpr std::pair<Int,bool> FromDarc( const Int da )
        {
            return PlanarDiagram_T::FromDarc(da);
        }
        
        
#include "OrthogonalRepresentation2/Constants.hpp"

    public:
        
        // TODO: Add this when the class is finished:
        // TODO: swap
        // TODO: copy assignment
        // TODO: move constructor
        // TODO: move assignment

        template<typename ExtInt>
        OrthogonalRepresentation2(
            mref<PlanarDiagram<ExtInt>> pd,
            const ExtInt exterior_face_ = ExtInt(-1),
            Settings_T settings_ = Settings_T()
        )
        :   settings( settings_ )
        {
            if( pd.CrossingCount() <= ExtInt(0) )
            {
                return;
            }
            
            LoadPlanarDiagram( pd, exterior_face_, settings.use_dual_simplexQ );
            
            if( settings.turn_regularizeQ )
            {
                TurnRegularize();
            }
            
            ComputeConstraintGraphs();

            switch( settings.compaction_method)
            {
                case 0:
                {
                    ComputeVertexCoordinates_ByLengths_Variant1();
                    break;
                }
                case 1:
                {
                    ComputeVertexCoordinates_ByTopologicalTightening();
                    break;
                }
                case 2:
                {
                    ComputeVertexCoordinates_ByTopologicalNumbering();
                    break;
                }
                case 3:
                {
                    ComputeVertexCoordinates_ByTopologicalOrdering();
                    break;
                }
                case 4:
                {
                    ComputeVertexCoordinates_ByLengths_Variant2();
                    break;
                }
                default:
                {
                    wprint(ClassName() + "(): Unknown compaction method. Using default (0).");
                    ComputeVertexCoordinates_ByLengths_Variant1();
                    break;
                }
            }

//            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
//            TOOLS_DUMP(BoolString(CheckFaceTurns()));
//            TOOLS_DUMP(BoolString(CheckTreDirections()));
//            
//            if( regularizeQ )
//            {
//                TOOLS_DUMP(BoolString(CheckTurnRegularity()));
//            }
        }
                                 
        // Default constructor
        OrthogonalRepresentation2() = default;
        // Destructor (virtual because of inheritance)
        virtual ~OrthogonalRepresentation2() override = default;
        // Copy constructor
        OrthogonalRepresentation2( const OrthogonalRepresentation2 & other ) = default;
        // Copy assignment operator
        OrthogonalRepresentation2 & operator=( const OrthogonalRepresentation2 & other ) = default;
        // Move constructor
        OrthogonalRepresentation2( OrthogonalRepresentation2 && other ) = default;
        // Move assignment operator
        OrthogonalRepresentation2 & operator=( OrthogonalRepresentation2 && other ) = default;
        
    private:
        
        Int crossing_count      = 0; // number of active crossings
        Int arc_count           = 0; // number of active arcs
        Int face_count          = 0; // number of number of faces
        
        Int max_crossing_count  = 0; // number of active + inactive crossings
        Int max_arc_count       = 0; // number of active + inactive arcs
        
        Int bend_count          = 0;
        Int vertex_count        = 0;
        Int edge_count          = 0;
        Int virtual_edge_count  = 0;
        
//        Int exterior_face       = 0;
//        Int maximum_face        = 0;
        Int max_face_size       = 0;
        
        Tiny::VectorList_AoS<2,bool,Int> A_overQ;
        
        Tensor1<Int,Int>    A_bends;
        
        RaggedList<Int,Int> A_V;
        RaggedList<Int,Int> A_E;
        
        // Entries are _outgoing_ directed edge indices. Do /2 to get actual arc index.
        VertexContainer_T   V_dE;
        mutable VertexFlagContainer_T V_flag;
        // General purpose buffers. May be used in all routines as temporary space.
        mutable Tensor1<Int,Int> V_scratch;


        // Undirected edge indices to undirected arc indices.
        Tensor1<Int,Int>    E_A;
        EdgeContainer_T     E_V;
        // Entries are _directed_ edge indices. Do /2 to get actual arc indes.
        EdgeContainer_T     E_left_dE;
        EdgeTurnContainer_T E_turn;
        Tensor1<Dir_T,Int>  E_dir; // Cardinal direction of _undirected_ edges.
    
        mutable Tensor1<Int,Int>     E_scratch;
        mutable EdgeFlagContainer_T  E_flag;
        
        Settings_T settings;
        
        mutable bool proven_turn_regularQ = false;
        
        // Plotting
        
//        Int width       = 0;
//        Int height      = 0;
//        
//        Int x_grid_size = 20;
//        Int y_grid_size = 20;
//        Int x_gap_size  = 4;
//        Int y_gap_size  = 4;
        
//        CoordsContainer_T V_coords;
//        CoordsContainer_T A_line_coords;
//        
//        Tensor1<Int,Int>  A_spline_ptr;
//        CoordsContainer_T A_spline_coords;
//        
//        std::vector<std::array<Int,2>> Dh_edge_agg;
//        std::vector<std::array<Int,2>> Dv_edge_agg;
        
    public:
        
        /*!@brief Make room for more virtual edges.
         */
        
        void Resize( const Int max_edge_count_ )
        {
            const Int max_edge_count = Max(Int(0), max_edge_count_ );
            
            // We do not change E_A. This way we still know how many nonvirtual edges we had in the beginning.
            
            const Int old_max_edge_count = E_V.Dim(0);
            
            if( max_edge_count == old_max_edge_count) { return; };
            
            // Might or might not be necessary.
            this->ClearAllCache();
            
//            print("before");
//            TOOLS_DUMP(E_V.Dim(0));
//            TOOLS_DUMP(E_flag.Dim(0));
//            TOOLS_DUMP(E_flag);
            
            E_A.      template Resize<true >( max_edge_count );
            E_V.      template Resize<true >( max_edge_count );
            E_left_dE.template Resize<true >( max_edge_count );
            E_turn.   template Resize<true >( max_edge_count );
            E_dir.    template Resize<true >( max_edge_count );
            E_flag.   template Resize<true >( max_edge_count );
            E_scratch.template Resize<false>( max_edge_count * Int(2) );

            if( max_edge_count > old_max_edge_count )
            {
                const Int p = old_max_edge_count;
                const Int d = max_edge_count - old_max_edge_count;
                
                fill_buffer( E_A.data(p)        , Uninitialized, d );
                fill_buffer( E_V.data(p)        , Uninitialized, d * Int(2) );
                fill_buffer( E_left_dE.data(p)  , Uninitialized, d * Int(2) );
                fill_buffer( E_turn.data(p)     , Turn_T(0)    , d * Int(2) );
                fill_buffer( E_dir.data(p)      , NoDir        , d );
                fill_buffer( E_flag.data(p)     , EdgeFlag_T(0), d * Int(2) );
            }
            
            
//            for( Int e = old_max_edge_count; e < max_edge_count; ++e )
//            {
//                E_A(e)              = Uninitialized;
//                E_V(e,Tail)         = Uninitialized;
//                E_V(e,Head)         = Uninitialized;
//                E_left_dE(e,Head)   = Uninitialized;
//                E_left_dE(e,Head)   = Uninitialized;
//                E_turn(e,Tail)      = Turn_T(0);
//                E_turn(e,Head)      = Turn_T(0);
//                E_dir[e]            = NoDir;
//                E_flag(e,Tail)      = EdgeFlag_T(0);
//                E_flag(e,Head)      = EdgeFlag_T(0);
//            }
            
//            print("after");
//            TOOLS_DUMP(E_V.Dim(0));
//            TOOLS_DUMP(E_flag.Dim(0));
//            TOOLS_DUMP(E_flag);
            
//            E_left_dE.template Resize<true>( max_edge_count );
//            E_turn.   template Resize<true>( max_edge_count );
//            E_dir.    template Resize<true>( max_edge_count );
//            E_flag.   template Resize<true>( max_edge_count );
//            E_scratch.template Resize<true>( Int(2) * max_edge_count );
            
//            EdgeContainer_T     new_E_V         ( max_edge_count );
//            EdgeContainer_T     new_E_left_dE   ( max_edge_count );
//            EdgeTurnContainer_T new_E_turn      ( max_edge_count );
//            Tensor1<Dir_T,Int>  new_E_dir       ( max_edge_count );
//            EdgeFlagContainer_T new_E_flag      ( max_edge_count );
//            Tensor1<Int,Int>    new_E_scratch   ( Int(2) * max_edge_count );
//
//            
//            if( max_edge_count >= old_max_edge_count )
//            {
//                E_V.Write       ( new_E_V.data()       );
//                E_left_dE.Write ( new_E_left_dE.data() );
//                E_turn.Write    ( new_E_turn.data()    );
//                E_dir.Write     ( new_E_dir.data()     );
//                E_flag.Write    ( new_E_flag.data()    );
//                
//                for( Int e = E_V.Dim(0); e < max_edge_count; ++e )
//                {
//                    new_E_V(e,Tail)         = Uninitialized;
//                    new_E_V(e,Head)         = Uninitialized;
//                    new_E_left_dE(e,Head)   = Uninitialized;
//                    new_E_left_dE(e,Head)   = Uninitialized;
//                    new_E_turn(e,Tail)      = Turn_T(0);
//                    new_E_turn(e,Head)      = Turn_T(0);
//                    new_E_dir[e]            = NoDir;
//                    new_E_flag(e,Tail)      = EdgeFlag_T(0);
//                    new_E_flag(e,Head)      = EdgeFlag_T(0);
//                }
//            }
//            else
//            {
//                new_E_V.Read(E_V.data());
//                new_E_left_dE.Read(E_left_dE.data());
//                new_E_turn.Read(E_turn.data());
//                new_E_dir.Read(E_dir.data());
//                new_E_flag.Read(E_flag.data());
//            }
//
//            swap( E_V       ,new_E_V        );
//            swap( E_left_dE ,new_E_left_dE  );
//            swap( E_turn    ,new_E_turn     );
//            swap( E_dir     ,new_E_dir      );
//            swap( E_flag    ,new_E_flag     );
//            swap( E_scratch ,new_E_scratch  );
        }
        
    private:

#include "OrthogonalRepresentation2/BendsLP.hpp"
#include "OrthogonalRepresentation2/LoadPlanarDiagram.hpp"
#include "OrthogonalRepresentation2/Edges.hpp"
#include "OrthogonalRepresentation2/Faces.hpp"
#include "OrthogonalRepresentation2/TurnRegularize.hpp"
        
#include "OrthogonalRepresentation2/Plotting.hpp"
        
//#include "OrthogonalRepresentation2/Tredges.hpp"
//#include "OrthogonalRepresentation2/Trfaces.hpp"
#include "OrthogonalRepresentation2/SaturateFaces.hpp"
#include "OrthogonalRepresentation2/ConstraintGraphs.hpp"
#include "OrthogonalRepresentation2/LengthsLP_Variant1.hpp"
#include "OrthogonalRepresentation2/LengthsLP_Variant2.hpp"
#include "OrthogonalRepresentation2/Coordinates.hpp"
#include "OrthogonalRepresentation2/FindIntersections.hpp"


//###########################################################
//##        Accessor Routines
//###########################################################
        
    public:
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        Int ArcCount() const
        {
            return arc_count;
        }
        
        bool VertexActiveQ( const Int v ) const
        {
            return V_flag[v] != VertexFlag_T::Inactive;
        }
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        cref<VertexFlagContainer_T> VertexFlags() const
        {
            return V_flag;
        }
        
        
        cref<VertexContainer_T> VertexDedges() const
        {
            return V_dE;
        }
        
        
        Int EdgeCount() const
        {
            return edge_count;
        }
        
        cref<EdgeContainer_T> Edges() const
        {
            return E_V;
        }
        
        cref<EdgeFlagContainer_T> EdgeFlags() const
        {
            return E_flag;
        }
                
        Int VirtualEdgeCount() const
        {
            return virtual_edge_count;
        }
//        
//        EdgeContainer_T VirtualEdges() const
//        {
//            const Int virtual_edge_count = VirtualEdgeCount();
//            
//            EdgeContainer_T virtual_edges ( virtual_edge_count );
//            
//            copy_buffer(
//                TRE_V.data( edge_count ),
//                virtual_edges.data(),
//                Int(2) * virtual_edge_count
//            );
//            
//            return virtual_edges;
//        }
        
//        Int ExteriorFace() const
//        {
//            return exterior_face;
//        }
        
//        cref<VertexContainer_T> TrvDtre() const
//        {
//            return V_dTRE;
//        }
//        
//        cref<EdgeContainer_T> TreTrv() const
//        {
//            return TRE_V;
//        }
        
        
        cref<Tensor1<Int,Int>> EdgeArcs() const
        {
            return E_A;
        }

        cref<RaggedList<Int,Int>> ArcVertices() const
        {
            return A_V;
        }
        
        cref<RaggedList<Int,Int>> ArcEdges() const
        {
            return A_E;
        }
        
        cref<EdgeTurnContainer_T> EdgeTurns() const
        {
            return E_turn;
        }
        
        Turn_T EdgeTurn( const Int de ) const
        {
            return E_turn.data()[de];
        }

        Turn_T EdgeTurn( const Int e, const bool d )  const
        {
            return E_turn(e,d);
        }

        
        cref<EdgeContainer_T> EdgeLeftDedges() const
        {
            return E_left_dE;
        }
        
        Int DedgeLeftDedge( const Int de ) const
        {
            return E_left_dE.data()[de];
        }
        
        std::pair<Int,bool> EdgeLeftEdge( const Int e, const bool d ) const
        {
            return std::pair(e,d);
        }
        
        Int EdgeLeftDedge( const Int e, const bool d )  const
        {
            return E_left_dE(e,d);
        }
        
        
        cref<Tensor1<Dir_T,Int>> EdgeDirections() const
        {
            return E_dir;
        }
        
        cref<Tensor1<Int,Int>> Bends() const
        {
            return A_bends;
        }
  
        bool PlusMinusMatrixQ() const
        {
            return settings.pm_matrixQ;
        }

        void SetPlusMinusMatrixQ( const bool val )
        {
            settings.pm_matrixQ = val;
        }
        
        
//        // Horizontal and vertical constaint graph
//
//        cref<DiGraph_T> DhConstraintGraph() const
//        {
//            return Dh;
//        }
//        
//        cref<Tensor1<Int,Int>> DhVertexEdgePointers() const
//        {
//            return DhV_E_ptr;
//        }
//        
//        cref<Tensor1<Int,Int>> DhVertexEdgeIndices() const
//        {
//            return DhV_E_idx;
//        }
//        
//        cref<Tensor1<Int,Int>> VertexDhVertex() const
//        {
//            return V_DhV;
//        }
//        
//        cref<Tensor1<Int,Int>> EdgeDhVertex() const
//        {
//            return E_DhV;
//        }
//        
//        
//        cref<DiGraph_T> DvConstraintGraph() const
//        {
//            return Dv;
//        }
//        
//        cref<Tensor1<Int,Int>> DvVertexEdgePointers() const
//        {
//            return DvV_E_ptr;
//        }
//        
//        cref<Tensor1<Int,Int>> DvVertexEdgeIndices() const
//        {
//            return DvV_E_idx;
//        }
//        
//        cref<Tensor1<Int,Int>> VertexDvVertex() const
//        {
//            return V_DvV;
//        }
//        
//        cref<Tensor1<Int,Int>> EdgeDvVertices() const
//        {
//            return E_DvV;
//        }
        
        
//        cref<Tensor1<Int,Int>> DhTopologicalNumbering() const
//        {
//            return Dh.TopologicalNumbering();
//        }
//
//        cref<Tensor1<Int,Int>> DhTopologicalOrdering() const
//        {
//            return Dh.TopologicalOrdering();
//        }
//
//        cref<Tensor1<Int,Int>> DvTopologicalNumbering() const
//        {
//            return Dv.TopologicalNumbering();
//        }
//
//        cref<Tensor1<Int,Int>> DvTopologicalOrdering() const
//        {
//            return Dv.TopologicalOrdering();
//        }
        
        
//        cref<CoordsContainer_T> VertexCoordinates() const
//        {
//            return V_coords;
//        }
//        
//        cref<CoordsContainer_T> ArcLineCoordinates() const
//        {
//            return A_line_coords;
//        }
//
//        cref<Tensor1<Int,Int>> ArcSplinePointers() const
//        {
//            return A_spline_ptr;
//        }
//        
//        cref<CoordsContainer_T> ArcSplineCoordinates() const
//        {
//            return A_spline_coords;
//        }


    public:
        
        static std::string DirectionString( const Dir_T dir )
        {
            switch ( dir )
            {
                case East:  return "east";
                case North: return "north";
                case West:  return "west";
                case South: return "south";
                default:    return "invalid";
            }
        }
        
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation2<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation2
    
} // namespace Knoodle



