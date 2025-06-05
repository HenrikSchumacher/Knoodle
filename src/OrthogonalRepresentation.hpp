#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: NextLeftEdge;
    // TODO: check for EdgeTurns;
    
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    
    template<typename Int_>
    class OrthogonalRepresentation
    {
    public:
        static_assert(IntQ<Int_>,"");
        
        using Int       = Int_;
        using SInt      = Int8;
        using COIN_Real = double;
        using COIN_Int  = int;
        using COIN_LInt = CoinBigIndex;
        using UInt      = UInt32;
        using Dir_T     = SInt;

        using DiGraph_T           = MultiDiGraph<Int,Int>;
        using PlanarDiagram_T     = PlanarDiagram<Int>;
        using VertexContainer_T   = Tiny::VectorList_AoS<4, Int,Int>;
        using EdgeContainer_T     = DiGraph_T::EdgeContainer_T;
        using EdgeTurnContainer_T = Tiny::VectorList_AoS<2,SInt,Int>;
        using DirectedArcNode     = PlanarDiagram_T::DirectedArcNode;

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
        
        // TODO: swap
        // TODO: copy assignment
        // TODO: move constructor
        // TODO: move assignment
        

        UInt getArcOrientation( bool io, bool lr )
        {
            return (io ? ( UInt(2) + lr ): !lr) % UInt(4);
        }
        
        OrthogonalRepresentation( mref<PlanarDiagram_T> pd, const Int exterior_face )
        {
            LoadPlanarDiagram(pd,exterior_face);
            ComputeConstraintGraphs();
        }
                                 
        ~OrthogonalRepresentation() = default;
        
    private:
        
        Int crossing_count = 0;
        Int arc_count      = 0;
        Int face_count     = 0;
        Int bend_count     = 0;
        Int vertex_count   = 0;
        Int edge_count     = 0;
        
        Int exterior_face  = 0;

        Tensor1<Int,Int>    A_bends;
        VertexContainer_T   V_dE; // Entries are _outgoing_ directed edge indices. Do /2 to get actual arc index.
        EdgeContainer_T     A_C; // The orginal arcs. Not sure whether we are really going to need them...
        EdgeContainer_T     E_V;
        EdgeContainer_T     E_left_dE; // Entries are _directed_ edge indices. Do /2 to get actual arc indes.
        
        // This ought to map directed edges to the turn at the _tail_.
        EdgeTurnContainer_T E_turn;
        // TODO: Computing both (A_V_ptr,A_V_idx) and (A_E_ptr,A_E_idx) seems redundant.
        Tensor1<SInt,Int>   E_dir; // Cardinal direction of _undirected_ edges.
        
        Tensor1<Int,Int>    A_V_ptr;
        Tensor1<Int,Int>    A_V_idx;
        // Arc a has vertices A_V_idx[A_V_ptr[a]], A_V_idx[A_V_ptr[0]+1],A_V_idx[A_V_ptr[0]+2],...,A_V_idx[A_V_ptr[a+1]])
        
        Tensor1<Int,Int>    E_A; // Undirected edge indices to undirected arc indices.
        Tensor1<Int,Int>    A_E_ptr;
        Tensor1<Int,Int>    A_E_idx;
        // Arc a has undirected edges A_E_idx[A_E_ptr[a]], A_E_idx[A_E_ptr[0]+1],A_E_idx[A_E_ptr[0]+2],...,A_E_idx[A_E_ptr[a+1]])
        
        EdgeContainer_T     E_F;
        Tensor1<Int,Int>    F_dE_ptr;
        Tensor1<Int,Int>    F_dE_idx;
        
        DiGraph_T           D_h; // constraint graph of horizontal segments
        DiGraph_T           D_v; // constraint graph of vertical segments
        
        // The range [ S_h_idx[S_h_ptr[s]],...,S_h_idx[S_h_ptr[s+1]] ) are the edges that belong to s-th horizontal segment, ordered from West to East.
        Tensor1<Int,Int>    S_h_ptr;
        Tensor1<Int,Int>    S_h_idx;
        
        // The range [ S_v_idx[S_v_ptr[s]],...,S_v_idx[S_v_ptr[s+1]] ) are the edges that belong to s-th vertical segment, ordered from West to East.
        Tensor1<Int,Int>    S_v_ptr;
        Tensor1<Int,Int>    S_v_idx;
        
        // Maps each undirected edge to its horizontal segment (-1 if vertical)
        Tensor1<Int,Int>    E_S_h;
        // Maps each undirected edge to its vertical segment (-1 if horizontal)
        Tensor1<Int,Int>    E_S_v;
        
        // Maps each vertex to its horizontal segment (-1 if vertical)
        Tensor1<Int,Int>    V_S_h;
        // Maps each vertex to its vertical segment (-1 if horizontal)
        Tensor1<Int,Int>    V_S_v;
        
        mutable Tensor1<Int,Int> V_scratch;
        mutable Tensor1<Int,Int> E_scratch;
        
        bool proven_turn_regularQ = false;
        
        PlanarDiagram_T * pd_ptr;
        
    private:

#include "OrthogonalRepresentation/BendsSystemLP.hpp"
#include "OrthogonalRepresentation/BendsByLP.hpp"
#include "OrthogonalRepresentation/LoadPlanarDiagram.hpp"
#include "OrthogonalRepresentation/ConstraintGraphs.hpp"
        
    public:
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        cref<VertexContainer_T> VertexDiEdges() const
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
        
        SInt EdgeTurn( const Int de ) const
        {
            return E_turn.data()[de];
        }

        SInt EdgeTurn( const Int e, const bool d )  const
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
        Int ToDiEdge( const Int e, const bool d ) const
        {
            return Int(2) * e + d;
        }
        
        template<bool d>
        Int ToDiEdge( const Int e ) const
        {
            return Int(2) * e + d;
        }
        
        // You should always do a check that de >= 0 before calling this!
        std::pair<Int,bool> FromDiEdge( const Int de ) const
        {
            return std::pair( de / Int(2), de % Int(2) );
        }
        
        cref<Tensor1<SInt,Int>> EdgeDirections() const
        {
            return E_dir;
        }
        
        
        cref<Tensor1<Int,Int>> Bends() const
        {
            return A_bends;
        }
        
        
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
        
        cref<Tensor1<Int,Int>> VerticesHorizontalSegments() const
        {
            return V_S_h;
        }
        
        cref<Tensor1<Int,Int>> EdgesHorizontalSegments() const
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
        
        cref<Tensor1<Int,Int>> VerticesVerticalSegments() const
        {
            return V_S_v;
        }
        
        cref<Tensor1<Int,Int>> EdgesVerticalSegments() const
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
        
        
        cref<Tensor1<Int,Int>> FaceDiEdgePointers() const
        {
            return F_dE_ptr;
        }
        
        cref<Tensor1<Int,Int>> FaceDiEdgeIndices() const
        {
            return F_dE_idx;
        }

        cref<EdgeContainer_T> EdgeFaces() const
        {
            return E_F;
        }
        
//        OrthogonalRepresentation RegularizeTurnsByKittyCorners()
//        {
//            // DONE: Compute the faces.
//            // TODO: Check faces for turn-regularity/detect kitty-corners.
//            // TODO: Resolve kitty-corners.
//            //      TODO: Do that horizontally/vertically in alternating fashion.
//            //      TODO: Do that randomly.
//            // TODO: Update and forward the map from edges to original arcs.
//        }
  
//        Not that important:
//        OrthogonalRepresentation RegularizeTurnsByReactangles()
//        {
//            // TODO: Check faces for turn-regularity/detect kitty-corners.
//            // TODO: Resolve by using rectangles.
//            // TODO: Update and forward the map from original crossings to vertices.
//            // TODO: Update and forward the map from edges to original arcs.
//        }

    public:
        
        static std::string DirectionString( const SInt dir )
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
        
        template<bool bound_checkQ = true,bool verboseQ = true>
        bool CheckEdgeDirection( Int e )
        {
            if constexpr( bound_checkQ )
            {
                if( (e < Int(0)) || (e >= EdgeCount()) )
                {
                    eprint(ClassName() + "::CheckEdgeDirection: Index " + ToString(e) + " is out of bounds.");
                    
                    return false;
                }
            }
            
            const Int tail = E_V(e,Tail);
            const Int head = E_V(e,Head);
            
            const SInt dir       = E_dir[e];
            const SInt tail_port = E_dir[e];
            const SInt head_port = static_cast<SInt>( (static_cast<UInt>(dir) + UInt(2) ) % UInt(4) );
            
            const bool tail_okayQ = (V_dE(tail,tail_port) == Int(2) * e + Head);
            const bool head_okayQ = (V_dE(head,head_port) == Int(2) * e + Tail);
            
            if constexpr( verboseQ )
            {
                if( !tail_okayQ )
                {
                    eprint(ClassName() + "::CheckEdgeDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its tail " + ToString(tail) + ".");
                }
                if( !head_okayQ )
                {
                    eprint(ClassName() + "::CheckEdgeDirection: edge " + ToString(e) + " points " + DirectionString(dir) + ", but it is not docked to the " + DirectionString(tail_port) + "ern port of its head " + ToString(head) + ".");
                }
            }
            return tail_okayQ && head_okayQ;
        }
        
        template<bool verboseQ = true>
        bool CheckEdgeDirections()
        {
            for( Int e = 0; e < EdgeCount(); ++e )
            {
                if( !this->template CheckEdgeDirection<false,verboseQ>(e) )
                {
                    return false;
                }
            }
            return true;
        }

        
        Tensor1<Int,Int> CreateFaceFromDiEdge( const Int de_0 ) const
        {
            TOOLS_PTIMER(timer,ClassName() + "::CreateFaceFromDiEdge");
            
            if( de_0 < Int(0) )
            {
                return Tensor1<Int,Int>();
            }
            
            Int ptr = 0;
            Int de = de_0;
            
            do
            {
                E_scratch[ptr++] = de;
                de = DiEdgeLeftDiEdge(de);
            }
            while( (de != de_0) && ptr < edge_count );
            
            if( (de != de_0) && (ptr >= edge_count) )
            {
                eprint(ClassName() + "::CreatedFaceFromDiEdge: Aborted because two many elements were collected. The data structure is probably corrupted.");
            }
            
            return Tensor1<Int,Int>( E_scratch.data(), ptr );
        }
        
        std::pair<Int,Int> FindKittyCorner( const Int de_0 ) const
        {
            TOOLS_PTIMER(timer,ClassName() + "::FindKittyCorner");
            
            if( de_0 < Int(0) )
            {
                return std::pair(Int(-1),Int(-1));
            }

            mptr<Int> rotation = V_scratch.data();
            
            Int ptr = 0;
            rotation[0] = Int(0);
            
            {
                Int de = de_0;
                do
                {
                    E_scratch[ptr++] = de;
                    rotation[ptr] = rotation[ptr-1] + E_turn.data()[de];
                    de = DiEdgeLeftDiEdge(de);
                }
                while( (de != de_0) && ptr < edge_count );
            
                if( (de != de_0) && (ptr >= edge_count) )
                {
                    eprint(ClassName() + "::TurnRegularFaceQ: Aborted because two many elements were collected. The data structure is probably corrupted.");
                    
                    return std::pair(Int(-1),Int(-1));
                }
            }
            
            valprint("rotation", ArrayToString(rotation,{ptr}));
            
            for( Int i = 0; i < ptr; ++i )
            {
                const Int de = E_scratch[i];
                
                if( E_turn.data()[de] == SInt(-1) )
                {
                    for( Int j = i; j < ptr; ++j )
                    {
                        const Int df = E_scratch[j];
                        
                        if( rotation[i] - rotation[j] == Int(2) )
                        {
                            if( E_turn.data()[df] == SInt(-1) )
                            {
                                const Int v = E_V.data()[de];
                                const Int w = E_V.data()[df];
                                TOOLS_DUMP(de/2);
                                TOOLS_DUMP(v);
                                TOOLS_DUMP(df/2);
                                TOOLS_DUMP(w);
                                return std::pair(v,w);
                            }
                        }
                    }
                }
            }
            
            return std::pair(Int(-1),Int(-1));
        }
        
        
    public:
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation
    
} // namespace Knoodle



