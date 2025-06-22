#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: check EdgeTurns;
    // TODO: What to do with multiple diagram components?
    
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    
    template<typename Int_>
    class OrthogonalRepresentation2 : CachedObject
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
            bool use_dual_simplexQ = false
        )
        {
            LoadPlanarDiagram( pd, exterior_face_, use_dual_simplexQ );
            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
            
//            ComputeFaces();
            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
//            TOOLS_DUMP(BoolString(CheckFaceTurns()));
//
//            auto V_dE_backup = V_dE;
//            auto E_V_backup = E_V;
//            auto E_dir_backup = E_dir;
//            auto E_turn_backup = E_turn;
            
//            bool regularizeQ = true;
//            bool regularizeQ = false;
            
//            print("TurnRegularize...");
//            TurnRegularize( regularizeQ );
//            print("TurnRegularize done.");
            
//            TOOLS_DUMP(V_dE == V_dE_backup);
//            TOOLS_DUMP(E_V == E_V_backup);
//            TOOLS_DUMP(E_dir == E_dir_backup);
//            TOOLS_DUMP(E_turn == E_turn_backup);
//
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
//            TOOLS_DUMP(BoolString(CheckFaceTurns()));
//            TOOLS_DUMP(BoolString(CheckTreDirections()));
            
//            ComputeConstraintGraphs();
//            
//            TOOLS_DUMP(BoolString(CheckEdgeDirections()));
//            TOOLS_DUMP(BoolString(CheckFaceTurns()));
//            TOOLS_DUMP(BoolString(CheckTreDirections()));
//            
//            if( regularizeQ )
//            {
//                TOOLS_DUMP(BoolString(CheckTurnRegularity()));
//            }
//            
////            ComputeVertexCoordinates_ByTopologicalNumbering();
//            ComputeVertexCoordinates_ByTopologicalTightening();
//            
//            ComputeArcLines();
//            
//            ComputeArcSplines();
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
        
        
//        // Copy constructor
//        OrthogonalRepresentation2( const OrthogonalRepresentation2 & other ) = default;
//        
////        MultiGraph( const MultiGraph & other )
////        :   Base_T( static_cast<const Base_T &>(other) )
////        {}
//        
//        friend void swap( OrthogonalRepresentation2 & A, OrthogonalRepresentation2 & B ) noexcept
//        {
//            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
//            using std::swap;
//            
//            swap( static_cast<Base_T &>(A), static_cast<Base_T &>(B) );
//            
//            swap( A.crossing_count      , B.crossing_count      );
//            swap( A.arc_count           , B.arc_count           );
//            swap( A.face_count          , B.face_count          );
//            swap( A.max_crossing_count  , B.max_crossing_count  );
//            swap( A.max_arc_count       , B.max_arc_count       );
//            swap( A.bend_count          , B.bend_count          );
//            swap( A.vertex_count        , B.vertex_count        );
//            swap( A.edge_count          , B.edge_count          );
//            swap( A.virtual_edge_count  , B.virtual_edge_count  );
//            
////            swap( A.exterior_face       , B.exterior_face       );
////            swap( A.maximum_face        , B.maximum_face        );
//            swap( A.max_face_size       , B.max_face_size       );
//            
//            swap( A.A_overQ             , B.A_overQ             );
//            swap( A.A_bends             , B.A_bends             );
//            
//            swap( A.A_V                 , B.A_V                 );
//            swap( A.A_E                 , B.A_E                 );
//            
//            swap( A.V_dE                , B.V_dE                );
//            swap( A.V_flag              , B.V_flag              );
//            swap( A.V_scratch           , B.V_scratch           );
//            
//            swap( A.E_A                 , B.E_A                 );
//            swap( A.E_V                 , B.E_V                 );
//            swap( A.E_left_dE           , B.E_left_dE           );
//            swap( A.E_turn              , B.E_turn              );
//            swap( A.E_dir               , B.E_dir               );
//            swap( A.E_flag              , B.E_flag              );
//            swap( A.E_scratch           , B.E_scratch           );
//            
//            swap( A.proven_turn_regularQ, B.proven_turn_regularQ);
//            
//            swap( A.x_grid_size         , B.x_grid_size         );
//            swap( A.y_grid_size         , B.y_grid_size         );
//            swap( A.x_gap_size          , B.x_gap_size          );
//            swap( A.y_gap_size          , B.y_gap_size          );
//        }
//        
//        
//        // Copy assignment operator
//        OrthogonalRepresentation2 & operator=( OrthogonalRepresentation2 other )
//        {   //                                     ^
//            //                                     |
//            // Use the copy constructor     -------+
//            swap( *this, other );
//            return *this;
//        }
//        
//        // Move constructor
//        OrthogonalRepresentation2( OrthogonalRepresentation2 && other ) noexcept
//        :   OrthogonalRepresentation2()
//        {
//            swap(*this, other);
//        }


        
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
        
        bool proven_turn_regularQ = false;
        
        // Plotting
        
//        Int width       = 0;
//        Int height      = 0;
//        
        Int x_grid_size = 20;
        Int y_grid_size = 20;
        Int x_gap_size  = 4;
        Int y_gap_size  = 4;
        
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
        
        void RequireMaxEdgeCount( const Int max_edge_count )
        {
            if( max_edge_count > E_V.Dimension(0) )
            {
        //        OrthogonalRepresentation2 H = *this;
                
                // We do not change E_A. This way we still know how many nonvirtual edge we had in the beginning.
                
                EdgeContainer_T     new_E_V         ( max_edge_count );
                EdgeContainer_T     new_E_left_dE   ( max_edge_count );
                EdgeTurnContainer_T new_E_turn      ( max_edge_count );
                Tensor1<Dir_T,Int>  new_E_dir       ( max_edge_count );
                EdgeFlagContainer_T new_E_flag      ( max_edge_count );
                Tensor1<Int,Int>    new_E_scratch   ( Int(2) * max_edge_count );
                
                E_V.Write       ( new_E_V.data()       );
                E_left_dE.Write ( new_E_left_dE.data() );
                E_turn.Write    ( new_E_turn.data()    );
                E_dir.Write     ( new_E_dir.data()     );
                E_flag.Write    ( new_E_flag.data()    );
                
                for( Int e = E_V.Dimension(0); e < max_edge_count; ++e )
                {
                    new_E_V(e,Tail)         = Uninitialized;
                    new_E_V(e,Head)         = Uninitialized;
                    new_E_left_dE(e,Head)   = Uninitialized;
                    new_E_left_dE(e,Head)   = Uninitialized;
                    new_E_turn(e,Tail)      = Turn_T(0);
                    new_E_turn(e,Head)      = Turn_T(0);
                    new_E_dir[e]            = NoDir;
                    new_E_flag(e,Tail)      = EdgeFlag_T(0);
                    new_E_flag(e,Head)      = EdgeFlag_T(0);
                }
                
                swap( E_V       ,new_E_V        );
                swap( E_left_dE ,new_E_left_dE  );
                swap( E_turn    ,new_E_turn     );
                swap( E_dir     ,new_E_dir      );
                swap( E_flag    ,new_E_flag     );
                swap( E_scratch ,new_E_scratch  );
                
            }
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
//#include "OrthogonalRepresentation2/SaturateFaces.hpp"
//#include "OrthogonalRepresentation2/ConstraintGraphs.hpp"
//#include "OrthogonalRepresentation2/LengthsLP.hpp"
//#include "OrthogonalRepresentation2/Coordinates.hpp"
//#include "OrthogonalRepresentation2/FindIntersections.hpp"


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
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation2<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation2
    
} // namespace Knoodle



