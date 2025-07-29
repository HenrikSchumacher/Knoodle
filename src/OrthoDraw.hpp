#pragma once

// TODO: Make this optional.
#include "../submodules/Tensors/Clp.hpp"

// TODO: We have to put the following directories onto the search path:
// TODO:    "../submodules/Min-Cost-Flow-Class/OPTUtils/",
// TODO:    "../submodules/Min-Cost-Flow-Class/MCFClass/",
// TODO:    "../submodules/Min-Cost-Flow-Class/MCFSimplex/

namespace MCF
{
    #include "MCFSimplex.C"
}

namespace Knoodle
{
    // TODO: What to do with multiple diagram components?
    // TODO: Get/setters for all settings.
    
    template<typename Int_>
    class OrthoDraw final : CachedObject
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
        using Turn_T     = ToSigned<Int>;
        using Cost_T     = ToSigned<Int>;
        
        
        enum class VertexFlag_T : Int8
        {
            Inactive    =  0,
            RightHanded =  1,
            LeftHanded  = -1,
            Corner      =  2
        };
        
        struct Settings_T
        {
            int  bend_min_method          = 0;
            bool network_matrixQ          = true;
            bool redistribute_bendsQ      = true;
            bool use_dual_simplexQ        = false;
            bool turn_regularizeQ         = true;
            bool soften_virtual_edgesQ    = false;
            bool randomizeQ               = false;
            bool saturate_facesQ          = true;
            bool saturate_exterior_faceQ  = true;
            bool filter_saturating_edgesQ = true;
            bool parallelizeQ             = true;
            int  compaction_method        = 0;
            
            Int  x_grid_size              = 20;
            Int  y_grid_size              = 20;
            
            Int  x_gap_size               =  4;
            Int  y_gap_size               =  4;
            
            Int  x_rounding_radius        =  4;
            Int  y_rounding_radius        =  4;
            
            void PrintInfo() const
            {
                TOOLS_DDUMP(bend_min_method);
                TOOLS_DDUMP(network_matrixQ);
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
        using CrossingContainer_T   = PlanarDiagram_T::CrossingContainer_T;
        using ArcContainer_T        = PlanarDiagram_T::ArcContainer_T;
        
        using ArcSplineContainer_T  = RaggedList<std::array<Int,2>,Int>;
        
        using COIN_Matrix_T = Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt>;
        using COIN_Agg_T = TripleAggregator<COIN_Int,COIN_Int,COIN_Real,COIN_LInt>;
        
        static constexpr Int Uninitialized = PlanarDiagram_T::Uninitialized;
        static constexpr Int MaxValidIndex = PlanarDiagram_T::MaxValidIndex;
        

        
        static constexpr bool ValidIndexQ( const Int i )
        {
            return PlanarDiagram_T::ValidIndexQ(i);
        }
        
        // TODO: Maybe replace by pcg32?
        // TODO: However, it is used rather seldomly. We prefer our graph drawer to be deterministic.
        
        using PRNG_T = std::mt19937;
        
#include "OrthoDraw/Constants.hpp"

        
    private:
        
        static constexpr Int ToDarc( const Int a, const bool d )
        {
            return PlanarDiagram_T::ToDarc(a,d);
        }
        
        template<bool d>
        static constexpr Int ToDarc( const Int a )
        {
            return PlanarDiagram_T::template ToDarc<d>(a);
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
        OrthoDraw(
            mref<PlanarDiagram<ExtInt>> pd,
            const ExtInt exterior_region_ = ExtInt(-1),
            Settings_T settings_ = Settings_T()
        )
        :   settings( settings_ )
        {
            if( !pd.ValidQ() ) { return; }
            
            LoadPlanarDiagram( pd, exterior_region_ );
            
            if( settings.turn_regularizeQ )
            {
                TurnRegularize();
            }
            
//            this->template CheckEdgeDirections<true>();
            
            ComputeConstraintGraphs();
            
            switch( settings.compaction_method)
            {
                case 0:
                {
                    ComputeVertexCoordinates_ByLengths_Variant3(false);
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
                case 20:
                {
                    ComputeVertexCoordinates_ByLengths_Variant2(false);
                    break;
                }
                case 21:
                {
                    ComputeVertexCoordinates_ByLengths_Variant2(true);
                    break;
                }
                case 30:
                {
                    ComputeVertexCoordinates_ByLengths_Variant3(false);
                    break;
                }
                case 31:
                {
                    ComputeVertexCoordinates_ByLengths_Variant3(true);
                    break;
                }
                case 40:
                {
                    ComputeVertexCoordinates_ByLengths_Variant4(false);
                    break;
                }
                case 41:
                {
                    ComputeVertexCoordinates_ByLengths_Variant4(true);
                    break;
                }
                default:
                {
                    wprint(ClassName() + "(): Unknown compaction method. Using default (0).");
                    ComputeVertexCoordinates_ByLengths_Variant3(false);
                    break;
                }
            }
        }
                                 
        // Default constructor
        OrthoDraw() = default;
        // Destructor (virtual because of inheritance)
        virtual ~OrthoDraw() override = default;
        // Copy constructor
        OrthoDraw( const OrthoDraw & other ) = default;
        // Copy assignment operator
        OrthoDraw & operator=( const OrthoDraw & other ) = default;
        // Move constructor
        OrthoDraw( OrthoDraw && other ) = default;
        // Move assignment operator
        OrthoDraw & operator=( OrthoDraw && other ) = default;
        
    private:
        
        Int crossing_count      = 0; // number of active crossings
        Int arc_count           = 0; // number of active arcs
        
        Int bend_count          = 0;
        Int vertex_count        = 0;
        Int edge_count          = 0;
        Int virtual_edge_count  = 0;
        
        Int exterior_region     = 0;
        Int max_face_size       = 0;
        
        // Indices to the last active vertex and edge.
        Int V_end               = 0;
        Int E_end               = 0;
        
        CrossingContainer_T C_A;
        ArcContainer_T A_C;
        
        Tiny::VectorList_AoS<2,bool,Int> A_overQ;
        
        Tensor1<Turn_T,Int> A_bends;

        RaggedList<Int,Int> R_dA;
        
        RaggedList<Int,Int> A_V;
        RaggedList<Int,Int> A_E;
        
        // Entries are _outgoing_ directed edge indices. Use FromDedge to get actual arc index and direction.
        VertexContainer_T   V_dE;
        mutable VertexFlagContainer_T V_flag;
        // General purpose buffers. May be used in all routines as temporary space.
        mutable Tensor1<Int,Int> V_scratch;


        // Undirected edge indices to undirected arc indices.
        Tensor1<Int,Int>    E_A;
        EdgeContainer_T     E_V;
        // Entries are _directed_ edge indices. Use FromDedge to get actual arc index and direction.
        EdgeContainer_T     E_left_dE;
        EdgeTurnContainer_T E_turn;
        Tensor1<Dir_T,Int>  E_dir; // Cardinal direction of _undirected_ edges.

        
        mutable Tensor1<Int,Int>     E_scratch;
        mutable EdgeFlagContainer_T  E_flag;
        
        Settings_T settings;
        
        mutable bool proven_turn_regularQ = false;
        
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

#include "OrthoDraw/BendsLP.hpp"
#include "OrthoDraw/BendsLP_Clp.hpp"
#include "OrthoDraw/BendsLP_MCFClass.hpp"
#include "OrthoDraw/LoadPlanarDiagram.hpp"
#include "OrthoDraw/Vertices.hpp"
#include "OrthoDraw/Edges.hpp"
#include "OrthoDraw/Faces.hpp"
#include "OrthoDraw/TurnRegularize.hpp"
        
#include "OrthoDraw/SaturateFaces.hpp"
#include "OrthoDraw/ConstraintGraphs.hpp"
#include "OrthoDraw/LengthsLP_Variant2.hpp"
#include "OrthoDraw/LengthsLP_Variant3.hpp"
#include "OrthoDraw/LengthsLP_Variant4.hpp"

#include "OrthoDraw/PostProcessing.hpp"

#include "OrthoDraw/Coordinates.hpp"
#include "OrthoDraw/Plotting.hpp"
#include "OrthoDraw/FindIntersections.hpp"


//###########################################################
//##        Accessor Routines
//###########################################################
        
    public:
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
        
        Int MaxCrossingCount() const
        {
            return C_A.Dim(0);
        }
        
        cref<CrossingContainer_T> Crossings() const
        {
            return C_A;
        }
        
        Int ArcCount() const
        {
            return arc_count;
        }
        
        Int MaxArcCount() const
        {
            return A_C.Dim(0);
        }
        
        cref<ArcContainer_T> Arcs() const
        {
            return A_C;
        }
        
        Int VertexCount() const
        {
            return vertex_count;
        }
        
        Int MaxVertexCount() const
        {
            return V_dE.Dim(0);
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
        
        Int MaxEdgeCount() const
        {
            return E_V.Dim(0);
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
        
        EdgeContainer_T VirtualEdges() const
        {
            EdgeContainer_T virtual_edges ( virtual_edge_count );
            
            copy_buffer(
                E_V.data( E_V.Dim(0) - virtual_edge_count ),
                virtual_edges.data(),
                Int(2) * virtual_edge_count
            );
            
            return virtual_edges;
        }
        
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
        
        Int BendCount() const
        {
            return bend_count;
        }
        
        cref<Tensor1<Turn_T,Int>> Bends() const
        {
            return A_bends;
        }
  
        bool NetworkMatrixQ() const
        {
            return settings.network_matrixQ;
        }

        void SetNetworkMatrixQ( const bool val )
        {
            settings.network_matrixQ = val;
        }
        
        
        cref<Tensor1<Int,Int>> ArcNextArc() const
        {
            std::string tag ("ArcNextArc");
            TOOLS_PTIMER(timer,MethodName(tag));
            if( !this->InCacheQ(tag) )
            {
                const Int n = C_A.Dim(0);
                const Int m = A_C.Dim(0);
                
                Tensor1<Int,Int> A_next_A ( m, Uninitialized );
                
                for( Int c = 0; c < n; ++c )
                {
                    if( VertexActiveQ(c) )
                    {
                        A_next_A(C_A(c,In,Left )) = C_A(c,Out,Right);
                        A_next_A(C_A(c,In,Right)) = C_A(c,Out,Left );
                    }
                }
                this->SetCache(tag,std::move(A_next_A));
            }
            return this->GetCache<Tensor1<Int,Int>>(tag);
        }


    public:
        
        static std::string DirectionString( const Dir_T dir )
        {
            switch ( dir )
            {
                case East:      return "east";
                case North:     return "north";
                case West:      return "west";
                case South:     return "south";
                    
                case NorthEast: return "north-east";
                case NorthWest: return "north-west";
                case SouthWest: return "south-west";
                case SouthEast: return "south-east";
                    
                default:        return "invalid";
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
            return std::string("OrthoDraw<") + TypeName<Int> + ">";
        }
        
    }; // class OrthoDraw
    
} // namespace Knoodle



