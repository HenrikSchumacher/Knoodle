#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: check EdgeTurns;
    // TODO: What to do with multiple diagram components?
    
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    

    
    template<typename Int_>
    class OrthogonalRepresentation final : CachedObject
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
            bool pm_matrixQ               = true;
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
            
            void PrintInfo() const
            {
                TOOLS_DDUMP(pm_matrixQ);
                TOOLS_DDUMP(redistribute_bendsQ);
                TOOLS_DDUMP(use_dual_simplexQ);
                TOOLS_DDUMP(turn_regularizeQ);
                TOOLS_DDUMP(saturate_facesQ);
                TOOLS_DDUMP(saturate_exterior_faceQ);
                TOOLS_DDUMP(compaction_method);
                
                TOOLS_DDUMP(x_grid_size);
                TOOLS_DDUMP(y_grid_size);
                
                TOOLS_DDUMP(x_gap_size);
                TOOLS_DDUMP(y_gap_size);
            }
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
        
        static constexpr Int Uninitialized = PlanarDiagram_T::Uninitialized;
        static constexpr Int MaxValidIndex = PlanarDiagram_T::MaxValidIndex;
        
        static constexpr bool ValidIndexQ( const Int i )
        {
            return PlanarDiagram_T::ValidIndexQ(i);
        }
        
        
        
#include "OrthogonalRepresentation/Constants.hpp"

        
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

    public:
        
        // TODO: Add this when the class is finished:
        // TODO: swap
        // TODO: copy assignment
        // TODO: move constructor
        // TODO: move assignment

        template<typename ExtInt>
        OrthogonalRepresentation(
            mref<PlanarDiagram<ExtInt>> pd,
            const ExtInt exterior_region_ = ExtInt(-1),
            Settings_T settings_ = Settings_T()
        )
        :   settings( settings_ )
        {
            if( !pd.ValidQ() ) { return; }
            
            LoadPlanarDiagram( pd, exterior_region_, settings.use_dual_simplexQ );
            
            if( settings.turn_regularizeQ )
            {
                TurnRegularize();
            }
            
//            if( !CheckEdgeDirections() )
//            {
//                eprint(ClassName()+"(): CheckEdgeDirections failed after calling TurnRegularize.");
//            }
//            if( !CheckAllFaceTurns() )
//            {
//                eprint(ClassName()+"(): CheckAllFaceTurns failed after calling TurnRegularize.");
//            }
            
            ComputeConstraintGraphs();

            switch( settings.compaction_method)
            {
                case 0:
                {
                    ComputeVertexCoordinates_ByLengths_Variant2();
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
                default:
                {
                    wprint(ClassName() + "(): Unknown compaction method. Using default (0).");
                    ComputeVertexCoordinates_ByLengths_Variant2();
                    break;
                }
            }
        }
                                 
        // Default constructor
        OrthogonalRepresentation() = default;
        // Destructor (virtual because of inheritance)
        virtual ~OrthogonalRepresentation() override = default;
        // Copy constructor
        OrthogonalRepresentation( const OrthogonalRepresentation & other ) = default;
        // Copy assignment operator
        OrthogonalRepresentation & operator=( const OrthogonalRepresentation & other ) = default;
        // Move constructor
        OrthogonalRepresentation( OrthogonalRepresentation && other ) = default;
        // Move assignment operator
        OrthogonalRepresentation & operator=( OrthogonalRepresentation && other ) = default;
        
    private:
        
        Int crossing_count      = 0; // number of active crossings
        Int arc_count           = 0; // number of active arcs
        
        Int max_crossing_count  = 0; // number of active + inactive crossings
        Int max_arc_count       = 0; // number of active + inactive arcs
        
        Int bend_count          = 0;
        Int vertex_count        = 0;
        Int edge_count          = 0;
        Int virtual_edge_count  = 0;
        Int face_count          = 0; // number of number of faces
        
        Int exterior_region     = 0;
//        Int maximum_face        = 0;
        Int max_face_size       = 0;
        
        Tiny::VectorList_AoS<2,bool,Int> A_overQ;
        
        Tensor1<Int,Int>    A_bends;

        RaggedList<Int,Int> R_dA;
        
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
        }
        
    private:

#include "OrthogonalRepresentation/BendsLP.hpp"
#include "OrthogonalRepresentation/LoadPlanarDiagram.hpp"
#include "OrthogonalRepresentation/Edges.hpp"
#include "OrthogonalRepresentation/Faces.hpp"
#include "OrthogonalRepresentation/TurnRegularize.hpp"
        
#include "OrthogonalRepresentation/Plotting.hpp"
        
#include "OrthogonalRepresentation/SaturateFaces.hpp"
#include "OrthogonalRepresentation/ConstraintGraphs.hpp"
#include "OrthogonalRepresentation/LengthsLP_Variant2.hpp"
#include "OrthogonalRepresentation/Coordinates.hpp"
#include "OrthogonalRepresentation/FindIntersections.hpp"


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
        
        Int ExteriorRegion() const
        {
            return exterior_region;
        }
        
        Int RegionCount() const
        {
            return R_dA.SublistCount();
        }
        
        cref<RaggedList<Int,Int>> RegionDarcs() const
        {
            return R_dA;
        }
        
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
        
        void PrintSettings()
        {
            settings.PrintInfo();
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation
    
} // namespace Knoodle



