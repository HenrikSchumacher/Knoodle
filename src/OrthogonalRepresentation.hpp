#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: check EdgeTurns;
    // TODO: What to do with multiple diagram components?
    
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    
    template<typename Int_>
    class OrthogonalRepresentation
    {
    public:
        static_assert(SignedIntQ<Int_>,"");
        
        using Int       = Int_;
        using COIN_Real = double;
        using COIN_Int  = int;
        using COIN_LInt = CoinBigIndex;
        using UInt      = UInt32;
        using Dir_T     = UInt8;
        using Turn_T    = std::make_signed<Int>::type;
        
        using DiGraph_T           = MultiDiGraph<Int,Int>;
        using HeadTail_T          = DiGraph_T::HeadTail_T;
        using PlanarDiagram_T     = PlanarDiagram<Int>;
        using VertexContainer_T   = Tiny::VectorList_AoS<4, Int,Int>;
        using EdgeContainer_T     = DiGraph_T::EdgeContainer_T;
        using EdgeTurnContainer_T = Tiny::VectorList_AoS<2,Turn_T,Int>;
        using DirectedArcNode     = PlanarDiagram_T::DirectedArcNode;
        using CoordsContainer_T   = Tiny::VectorList_AoS<2,Int,Int>;
        using FlagContainer_T     = Tiny::VectorList_AoS<2,UInt8,Int>;
        
        enum class VertexState : Int8
        {
            Inactive    =  0,
            RightHanded =  1,
            LeftHanded  = -1,
            Corner      =  2
        };
        
        enum class EdgeState : UInt8
        {
            Inactive    = 0,
            Active      = 1,
            Virtual     = 2
        };
        
        static constexpr Int Uninitialized = PlanarDiagram_T::Uninitialized;
        static constexpr Int MaxValidIndex = PlanarDiagram_T::MaxValidIndex;
        
        static constexpr bool ValidIndexQ( const Int i )
        {
            return PlanarDiagram_T::ValidIndexQ(i);
        }
        
    private:
        
        static constexpr Int ToDiArc( const Int a, const bool d )
        {
            return PlanarDiagram_T::ToDiArc(a,d);
        }
        
        template<bool d>
        static constexpr Int ToDiArc( const Int a )
        {
            return PlanarDiagram_T::ToDiArc<d>(a);
        }
        
        static constexpr std::pair<Int,bool> FromDiArc( const Int da )
        {
            return PlanarDiagram_T::FromDiArc(da);
        }
        
        
#include "OrthogonalRepresentation/Constants.hpp"

    public:
        
        OrthogonalRepresentation() = default;
        
        // Copy constructor
        OrthogonalRepresentation( const OrthogonalRepresentation & other ) = default;
        
        // TODO: Add this when the class is finished:
        // TODO: swap
        // TODO: copy assignment
        // TODO: move constructor
        // TODO: move assignment

        OrthogonalRepresentation(
            mref<PlanarDiagram_T> pd, const Int exterior_face
        )
        {
            SubdividePlanarDiagram(pd,exterior_face);
            
            // We can compute this only after E_V and V_dE have been computed completed.
            ComputeEdgeLeftDiEdge();
            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
            
            ComputeFaces(pd);
            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
//            TOOLS_DUMP(BoolString(CheckFaceTurns()));
//            
//            auto V_dE_backup = V_dE;
//            auto E_V_backup = E_V;
//            auto E_dir_backup = E_dir;
//            auto E_turn_backup = E_turn;
            
            TurnRegularize();
            
//            TOOLS_DUMP(V_dE == V_dE_backup);
//            TOOLS_DUMP(E_V == E_V_backup);
//            TOOLS_DUMP(E_dir == E_dir_backup);
//            TOOLS_DUMP(E_turn == E_turn_backup);
            
            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
            TOOLS_DUMP(BoolString(CheckFaceTurns()));
            TOOLS_DUMP(BoolString(Check_TRE_Directions()));
            
            ComputeConstraintGraphs();
            
            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
            TOOLS_DUMP(BoolString(CheckFaceTurns()));
            TOOLS_DUMP(BoolString(Check_TRE_Directions()));
            TOOLS_DUMP(BoolString(CheckTurnRegularity()));
            
            {
                auto x = VerticalSegmentTopologicalNumbering();
                auto y = HorizontalSegmentTopologicalNumbering();
                
//                TOOLS_DUMP(x);
//                TOOLS_DUMP(y);
//                TOOLS_DUMP(V_S_v);
//                TOOLS_DUMP(V_S_h);
                
                ComputeVertexCoordinates(x,y);
            }
            
            ComputeArcLines();
            
            ComputeArcSplines();
        }
                                 
        ~OrthogonalRepresentation() = default;
        
    private:
        
        Int crossing_count      = 0; // number of active crossings
        Int arc_count           = 0; // number of active arcs
        Int face_count          = 0; // number of number of faces
        
        Int max_crossing_count  = 0; // number of active + inactive crossings
        Int max_arc_count       = 0; // number of active + inactive arcs
        
        Int bend_count          = 0;
        Int vertex_count        = 0;
        Int edge_count          = 0;
        
        Int exterior_face       = 0;

        Tensor1<VertexState,Int> V_state;
        Tensor1<EdgeState,Int>   E_state;
        
        Tiny::VectorList_AoS<2,bool,Int>  A_overQ;
        
        Tensor1<Int,Int>    A_bends;
        VertexContainer_T   V_dE; // Entries are _outgoing_ directed edge indices. Do /2 to get actual arc index.
        EdgeContainer_T     A_C; // The orginal arcs. Not sure whether we are really going to need them...
        EdgeContainer_T     E_V;
        EdgeContainer_T     E_left_dE; // Entries are _directed_ edge indices. Do /2 to get actual arc indes.
        
        EdgeTurnContainer_T E_turn;
        Tensor1<Dir_T,Int>  E_dir; // Cardinal direction of _undirected_ edges.
        
//        // TODO: Computing both (A_V_ptr,A_V_idx) and (A_E_ptr,A_E_idx) seems redundant.
        Tensor1<Int,Int>    A_V_ptr;
        Tensor1<Int,Int>    A_V_idx;
        
        Tensor1<Int,Int>    E_A; // Undirected edge indices to undirected arc indices.
        Tensor1<Int,Int>    A_E_ptr;
        Tensor1<Int,Int>    A_E_idx;
        // Arc a has undirected edges A_E_idx[A_E_ptr[a]], A_E_idx[A_E_ptr[0]+1],A_E_idx[A_E_ptr[0]+2],...,A_E_idx[A_E_ptr[a+1]])
        
        // Faces
        
        EdgeContainer_T     E_F;
        Tensor1<Int,Int>    F_dE_ptr;
        Tensor1<Int,Int>    F_dE_idx;
        
        
        // Turn regularization (TRE = turn regularized edges
        
        Int tre_count  = 0;
        
        VertexContainer_T   V_dTRE;
        EdgeContainer_T     TRE_V;
        EdgeContainer_T     TRE_left_dTRE;
        EdgeTurnContainer_T TRE_turn;
        Tensor1<Dir_T,Int>  TRE_dir;
        
        // Compaction
        
        DiGraph_T           D_h; // constraint graph of horizontal segments
        DiGraph_T           D_v; // constraint graph of vertical segments
        
        // The range [ S_h_idx[S_h_ptr[s]],...,S_h_idx[S_h_ptr[s+1]] ) are the edges that belong to s-th horizontal segment, ordered from West to East.
        Tensor1<Int,Int>    S_h_ptr;
        Tensor1<Int,Int>    S_h_idx;
        
        // The range [ S_v_idx[S_v_ptr[s]],...,S_v_idx[S_v_ptr[s+1]] ) are the edges that belong to s-th vertical segment, ordered from West to East.
        Tensor1<Int,Int>    S_v_ptr;
        Tensor1<Int,Int>    S_v_idx;
        
        // Maps each undirected edge to its horizontal segment (`Uninitialized` if vertical)
        Tensor1<Int,Int>    E_S_h;
        // Maps each undirected edge to its vertical segment (`Uninitialized` if horizontal)
        Tensor1<Int,Int>    E_S_v;
        
        // Maps each vertex to its horizontal segment (`Uninitialized` if vertical)
        Tensor1<Int,Int>    V_S_h;
        // Maps each vertex to its vertical segment (`Uninitialized` if horizontal)
        Tensor1<Int,Int>    V_S_v;
        
        // General purpose buffers. May be used in all routines as temporary space.
        mutable Tensor1<Int,Int> V_scratch;
        mutable Tensor1<Int,Int> E_scratch;
        mutable FlagContainer_T  TRE_flag;
        
        bool proven_turn_regularQ = false;
        
        // TODO: Delete this when the class is finished.
        PlanarDiagram_T * pd_ptr = nullptr;
        
        
        // Plotting
        
        Int width       = 0;
        Int height      = 0;
        
        Int x_grid_size = 20;
        Int y_grid_size = 20;
        Int x_gap_size  = 4;
        Int y_gap_size  = 4;
        
        CoordsContainer_T V_coords;
        CoordsContainer_T A_line_coords;
        
        Tensor1<Int,Int>  A_spline_ptr;
        CoordsContainer_T A_spline_coords;
        
    private:

#include "OrthogonalRepresentation/BendsSystemLP.hpp"
#include "OrthogonalRepresentation/BendsByLP.hpp"
#include "OrthogonalRepresentation/SubdividePlanarDiagram.hpp"
#include "OrthogonalRepresentation/Edges.hpp"
#include "OrthogonalRepresentation/Faces.hpp"
#include "OrthogonalRepresentation/TurnRegularize.hpp"
#include "OrthogonalRepresentation/SaturateFaces.hpp"
#include "OrthogonalRepresentation/ConstraintGraphs.hpp"
#include "OrthogonalRepresentation/Coordinates.hpp"
        
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
            return V_state[v] != VertexState::Inactive;
        }
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        cref<Tensor1<VertexState,Int>> VertexStates() const
        {
            return V_state;
        }
        
        
        cref<VertexContainer_T> VertexDiEdges() const
        {
            return V_dE;
        }
        
        bool EdgeActiveQ( const Int e ) const
        {
            return E_state[e] != EdgeState::Inactive;
        }
        
        Int EdgeCount() const
        {
            return edge_count;
        }
        
        cref<EdgeContainer_T> Edges() const
        {
            return E_V;
        }
        
        Int VirtualEdgeCount() const
        {
            return tre_count - edge_count;
        }
        
        EdgeContainer_T VirtualEdges() const
        {
            const Int virtual_edge_count = VirtualEdgeCount();
            
            EdgeContainer_T virtual_edges ( virtual_edge_count );
            
            copy_buffer(
                TRE_V.data( edge_count ),
                virtual_edges.data(),
                Int(2) * virtual_edge_count
            );
            
            return virtual_edges;
        }
        
        Int ExteriorFace() const
        {
            return exterior_face;
        }
        
        cref<VertexContainer_T> TurnRegularizedVertexDiEdges() const
        {
            return V_dTRE;
        }
        
        cref<EdgeContainer_T> TurnRegularizedEdges() const
        {
            return TRE_V;
        }
        
        
        cref<Tensor1<Int,Int>> EdgeArcs() const
        {
            return E_A;
        }
        
        cref<Tensor1<Int,Int>> ArcVertexPointers() const
        {
            return A_V_ptr;
        }
        
        cref<Tensor1<Int,Int>> ArcVertexIndices() const
        {
            return A_V_idx;
        }
        
        cref<Tensor1<Int,Int>> ArcEdgePointers() const
        {
            return A_E_ptr;
        }
        
        cref<Tensor1<Int,Int>> ArcEdgeIndices() const
        {
            return A_E_idx;
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

        
        cref<EdgeContainer_T> EdgeLeftDiEdges() const
        {
            return E_left_dE;
        }
        
        Int DiEdgeLeftDiEdge( const Int de ) const
        {
            return E_left_dE.data()[de];
        }
        
        std::pair<Int,bool> EdgeLeftEdge( const Int e, const bool d ) const
        {
            return std::pair(e,d);
        }
        
        Int EdgeLeftDiEdge( const Int e, const bool d )  const
        {
            return E_left_dE(e,d);
        }
        
        
        // You should always do a check that e >= 0 before calling this!
        Int ToDiEdge( const Int e, const HeadTail_T d ) const
        {
            return Int(2) * e + d;
        }
        
        template<bool d>
        Int ToDiEdge( const Int e ) const
        {
            return Int(2) * e + d;
        }
        
        // You should always do a check that de >= 0 before calling this!
        std::pair<Int,HeadTail_T> FromDiEdge( const Int de ) const
        {
            return std::pair( de / Int(2), de % Int(2) );
        }
        
        static constexpr Int FlipDiEdge( const Int de )
        {
            return de ^ Int(1);
        }
        
        
        cref<Tensor1<Dir_T,Int>> EdgeDirections() const
        {
            return E_dir;
        }
        
        
        cref<Tensor1<Int,Int>> Bends() const
        {
            return A_bends;
        }
        
        // Horizontal and vertical constaint grapj
        
        cref<DiGraph_T> HorizontalConstraintGraph() const
        {
            return D_h;
        }
        
        cref<Tensor1<Int,Int>> HorizontalSegmentPointers() const
        {
            return S_h_ptr;
        }
        
        cref<Tensor1<Int,Int>> HorizontalEdgeIndices() const
        {
            return S_h_idx;
        }
        
        cref<Tensor1<Int,Int>> VertexHorizontalSegments() const
        {
            return V_S_h;
        }
        
        cref<Tensor1<Int,Int>> EdgeHorizontalSegments() const
        {
            return E_S_h;
        }
        
        
        cref<DiGraph_T> VerticalConstraintGraph() const
        {
            return D_v;
        }
        
        cref<Tensor1<Int,Int>> VerticalSegmentPointers() const
        {
            return S_v_ptr;
        }
        
        cref<Tensor1<Int,Int>> VerticalEdgeIndices() const
        {
            return S_v_idx;
        }
        
        cref<Tensor1<Int,Int>> VertexVerticalSegments() const
        {
            return V_S_v;
        }
        
        cref<Tensor1<Int,Int>> EdgeVerticalSegments() const
        {
            return E_S_v;
        }
        
        
        Tensor1<Int,Int> HorizontalSegmentTopologicalNumbering() const
        {
            return D_h.TopologicalNumbering();
        }
        
        Tensor1<Int,Int> HorizontalSegmentTopologicalOrdering() const
        {
            return D_h.TopologicalOrdering();
        }
        
        Tensor1<Int,Int> VerticalSegmentTopologicalNumbering() const
        {
            return D_v.TopologicalNumbering();
        }

        Tensor1<Int,Int> VerticalSegmentTopologicalOrdering() const
        {
            return D_v.TopologicalOrdering();
        }
        
        
        cref<Tensor1<Int,Int>> FaceDirectedEdgePointers() const
        {
            return F_dE_ptr;
        }
        
        cref<Tensor1<Int,Int>> FaceDirectedEdgeIndices() const
        {
            return F_dE_idx;
        }

        cref<EdgeContainer_T> EdgeFaces() const
        {
            return E_F;
        }
        
        
        cref<CoordsContainer_T> VertexCoordinates() const
        {
            return V_coords;
        }
        
        cref<CoordsContainer_T> ArcLineCoordinates() const
        {
            return A_line_coords;
        }

        cref<Tensor1<Int,Int>> ArcSplinePointers() const
        {
            return A_spline_ptr;
        }
        
        cref<CoordsContainer_T> ArcSplineCoordinates() const
        {
            return A_spline_coords;
        }


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

        // Drawing
    public:
        
        Int HorizontalGridSize() const
        {
            return x_grid_size;
        }
        
        void SetHorizontalGridSize( const Int val )
        {
            x_grid_size = val;
        }
        
        Int VerticalGridSize() const
        {
            return y_grid_size;
        }
        
        void SetVerticalGridSize( const Int val )
        {
            y_grid_size = val;
        }
        
        
        
        Int HorizontalGapSize() const
        {
            return x_gap_size;
        }
        
        void SetHorizontalGapSize( const Int val )
        {
            x_gap_size = val;
        }
        
        Int VerticalGapSize() const
        {
            return y_gap_size;
        }
        
        void SetVerticalGapSize( const Int val )
        {
            y_gap_size = val;
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation
    
} // namespace Knoodle



