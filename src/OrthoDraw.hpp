#pragma once

// TODO: Make this optional.
#ifdef KNOODLE_USE_CLP
#include "../submodules/Tensors/Clp.hpp"
#endif


// For this, the user has to put the following directories onto the search path:
//      "../submodules/Min-Cost-Flow-Class/OPTUtils/",
//      "../submodules/Min-Cost-Flow-Class/MCFClass/",
//      "../submodules/Min-Cost-Flow-Class/MCFSimplex/

namespace MCF
{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmisleading-indentation"

    // TODO: MCFSimplex.C really does not like the -ffast-math flag. I get inconsistent results with his flag. I could use TOOLS_MAKE_FP_STRICT() here (which is #pragma float_control(precise, on) under clang). But that just suppressed compiler errors; the inconsistence still remains, even if I always place TOOLS_MAKE_FP_STRICT() before the load.
    
//    TOOLS_MAKE_FP_STRICT()
    
    #include "MCFSimplex.C"
    
#pragma clang diagnostic pop
}

namespace Knoodle
{
    // TODO: What to do with multiple diagram components?
    // TODO: Get/setters for all settings.
    
/*!@brief A class for drawing orthogonal layouts of planar diagrams.
 */
    
    template<class PD_T_>
    class OrthoDraw final : CachedObject<1,0,0,0>
    {
    private:
        
        using Base_T = CachedObject<1,0,0,0>;
        
    public:
        
        using PD_T       = PD_T_;
        using Int        = PD_T::Int;
        
        // TODO: Are signed integers really necessary here?
//        static_assert(SignedIntQ<Int>,"");
        
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

        /*!@brief Enum class for choosing the method for bend optimization in `OrthoDraw`. */
        enum class BendMethod_T : Int8
        {
            Unknown         = -1
            , Bends_MCF     =  0 /*!< Use MCFClas to minimize the number of bends. */
            , Bends_CLP     =  1 /*!< Use CLP to minimize the number of bends. */
        };
        
        /*!@brief Enum class for choosing the compaction method in `OrthoDraw`. */
        enum class CompactionMethod_T : Int8
        {
            Unknown                = -1
            , TopologicalNumbering =  0 /*!< Use a topological numbering (Kahn's algorithm). */
            , TopologicalOrdering  =  1 /*!< Use a topological ordering (Kahn's algorithm). */
            , Length_MCF           =  2 /*!< Use MCFClass to minimize length. */
            , Length_CLP           =  3 /*!< Use CLP to minimize length. */
            , AreaAndLength_CLP    =  4 /*!< Use CLP to minimize area and length. */
        };
        
        /*!@brief Control `struct` for holding settings of `OrthoDraw`. */
        struct Settings_T
        {
            /*! The method to be used for bend minimization. */
            BendMethod_T  bend_method               = BendMethod_T::Bends_MCF;
            /*! Whether to use the dual simplex algorithm. Only relevant when using CLP. */
            bool use_dual_simplexQ                  = false;
            /*! After a minimal distribution of bends is found, randomly turn each crossing this many times. Each time, the crossing is turned by -90, 0, or +90 degrees with probability 1/3. */
            int  randomize_bends                    = 0;
            /*! Try to balance the bends at each crossing. Does not change the number of bends. Only active if `randomize_bends = 0`. */
            bool redistribute_bendsQ                = true;
            /*! Must be set to `true`, otherwise the resulting layout is not guaranteed to be embeddded. */
            bool turn_regularizeQ                   = true;
            /*! Must be set to `false`, otherwise the resulting layout is not guaranteed to be embeddded. */
            bool soften_virtual_edgesQ              = false;
            /*! Flip a coin to determine whether virtual edges (some invisible edges that have to be added to prevent self-intersections) are placed horizontally or vertically. */
            bool randomize_virtual_edgesQ           = false;
            /*! Must be set to `true`, otherwise the resulting layout is not guaranteed to be embeddded. */
            bool saturate_regionsQ                  = true;
            /*! Must be set to `true`, otherwise the resulting layout is not guaranteed to be embeddded. */
            bool saturate_exterior_regionQ          = true;
            /*! Reduce the number of saturing edges by some ad hoc rules for small faces. */
            bool filter_saturating_edgesQ           = true;
            /*! The method to be uses for compaction. */
            CompactionMethod_T compaction_method    = CompactionMethod_T::Length_MCF;
            
            
            /*! The distance between two grid points in horizontal direction. */
            Int  x_grid_size                        = 20;
            /*! The distance between two grid points in vertical direction. */
            Int  y_grid_size                        = 20;
            /*! The size of gaps (when an arcs goes over another arc) in horizontal direction. */
            Int  x_gap_size                         =  4;
            /*! The size of gaps (when an arcs goes over another arc) in vertical direction. */
            Int  y_gap_size                         =  4;
            /*! The rounding "radius in x direction" used for bends. */
            Int  x_rounding_radius                  =  4;
            /*! The rounding "radius in y direction" used for bends. */
            Int  y_rounding_radius                  =  4;
        };
        
        friend std::string ToString( cref<Settings_T> args )
        {
            return std::string("{ ")
                    +   ".bend_method = " + ToString(args.bend_method)
                    + ", .use_dual_simplexQ = " + ToString(args.use_dual_simplexQ)
                    + ", .randomize_bends = " + ToString(args.randomize_bends)
                    + ", .redistribute_bendsQ = " + ToString(args.redistribute_bendsQ)
                    + ", .turn_regularizeQ = " + ToString(args.turn_regularizeQ)
                    + ", .soften_virtual_edgesQ = " + ToString(args.soften_virtual_edgesQ)
                    + ", .randomize_virtual_edgesQ = " + ToString(args.randomize_virtual_edgesQ)
                    + ", .saturate_regionsQ = " + ToString(args.saturate_regionsQ)
                    + ", .saturate_exterior_regionQ = " + ToString(args.saturate_exterior_regionQ)
                    + ", .filter_saturating_edgesQ = " + ToString(args.filter_saturating_edgesQ)
//                    + ", .parallelizeQ = " + ToString(args.parallelizeQ)
                    + ", .compaction_method = " + ToString(args.compaction_method)
                    + ", .x_grid_size = " + ToString(args.x_grid_size)
                    + ", .y_grid_size = " + ToString(args.y_grid_size)
                    + ", .x_gap_size = " + ToString(args.x_gap_size)
                    + ", .y_gap_size = " + ToString(args.y_gap_size)
                    + ", .x_rounding_radius = " + ToString(args.x_rounding_radius)
                    + ", .y_rounding_radius = " + ToString(args.y_rounding_radius)
            + " }";
        }
        
        using DiGraph_T             = MultiDiGraph<Int,Int>;
        using HeadTail_T            = DiGraph_T::HeadTail_T;
        using VertexContainer_T     = Tiny::VectorList_AoS<4,Int,Int>;
        using EdgeContainer_T       = DiGraph_T::EdgeContainer_T;
        using EdgeTurnContainer_T   = Tiny::VectorList_AoS<2,Turn_T,Int>;
        using CoordsContainer_T     = Tiny::VectorList_AoS<2,Int,Int>;
        using VertexFlagContainer_T = Tensor1<VertexFlag_T,Int>;
        using EdgeFlagContainer_T   = Tiny::VectorList_AoS<2,EdgeFlag_T,Int>;
        using Vector_T              = Tiny::Vector<2,Int,Int>;
//        using PD_T                  = PlanarDiagram<Int>;
        using CrossingContainer_T   = PD_T::CrossingContainer_T;
        using ArcContainer_T        = PD_T::ArcContainer_T;
        
        using ArcSplineContainer_T  = RaggedList<std::array<Int,2>,Int>;
        
        
#ifdef KNOODLE_USE_CLP
        using COIN_Real  = double;
        using COIN_Int   = int;
        using COIN_LInt  = CoinBigIndex;
        using COIN_Matrix_T = Sparse::MatrixCSR<COIN_Real,COIN_Int,COIN_LInt,Sequential>;
        using COIN_Agg_T = TripleAggregator<COIN_Int,COIN_Int,COIN_Real,COIN_LInt>;
#endif // KNOODLE_USE_CLP
        
        static constexpr Int Uninitialized = PD_T::Uninitialized;
        static constexpr Int MaxValidIndex = PD_T::MaxValidIndex;
        
        static constexpr bool ValidIndexQ( const Int i )
        {
            return PD_T::ValidIndexQ(i);
        }
        
        using PRNG_T = Knoodle::PRNG_T;
        
#include "OrthoDraw/Constants.hpp"
        
    private:
        
        static constexpr Int ToDarc( const Int a, const bool d )
        {
            return PD_T::ToDarc(a,d);
        }
        
        static constexpr std::pair<Int,bool> FromDarc( const Int da )
        {
            return PD_T::FromDarc(da);
        }

    public:
        
        // TODO: Add this when the class is finished:
        // TODO: swap
        // TODO: copy assignment
        // TODO: move constructor
        // TODO: move assignment

        template<IntQ ExtInt>
        OrthoDraw(
            cref<PD_T> pd,
            const ExtInt exterior_face_ = ExtInt(-1),
            Settings_T settings_ = Settings_T()
        )
        :   settings( settings_ )
        {
            if( !pd.ValidQ() ) { return; }
            
            LoadPlanarDiagram( pd, exterior_face_ );
            
            if( settings.turn_regularizeQ )
            {
                TurnRegularize();
            }
            
            ComputeConstraintGraphs();
            
            switch( settings.compaction_method )
            {
                case CompactionMethod_T::TopologicalNumbering:
                {
                    ComputeVertexCoordinates_TopologicalNumbering();
                    break;
                }
                case CompactionMethod_T::TopologicalOrdering:
                {
                    ComputeVertexCoordinates_TopologicalOrdering();
                    break;
                }
                case CompactionMethod_T::Length_MCF:
                {
                    ComputeVertexCoordinates_Compaction_MCF();
                    break;
                }
#ifdef KNOODLE_USE_CLP
                case CompactionMethod_T::Length_CLP:
                {
                    ComputeVertexCoordinates_Compaction_CLP(false);
                    break;
                }
                case CompactionMethod_T::AreaAndLength_CLP:
                {
                    ComputeVertexCoordinates_Compaction_CLP(true);
                    break;
                }
#endif
                default:
                {
                    wprint(ClassName() + "(): Unknown compaction method " + ToString(settings.compaction_method) + ". Using default (CompactionMethod_T::Length_MCF).");
                    ComputeVertexCoordinates_Compaction_MCF();
                    break;
                }
            }
        }
                                 
        // Default constructor
        OrthoDraw() = default;
        // Destructor (virtual because of inheritance)
        virtual ~OrthoDraw() override = default;
        // Copy constructor
        OrthoDraw( const OrthoDraw & other ) = delete;              // Because of random_engine.
        // Copy assignment operator
        OrthoDraw & operator=( const OrthoDraw & other ) = delete;  // Because of random_engine.
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
        
        Int exterior_face       = 0;
        Int max_face_size       = 0;
        
        // Indices to the last active vertex and edge.
        Int V_end               = 0;
        Int E_end               = 0;
        
        CrossingContainer_T C_A;
        ArcContainer_T A_C;
        
        Tiny::VectorList_AoS<2,bool,Int> A_overQ;
        
        Tensor1<Turn_T,Int> A_bends;

        RaggedList<Int,Int> F_dA;
        
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
        
        PRNG_T random_engine { InitializedRandomEngine<PRNG_T>() };
        
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
                
                fill_buffer( E_A.data(p)        , Uninitialized, d          );
                fill_buffer( E_V.data(p)        , Uninitialized, d * Int(2) );
                fill_buffer( E_left_dE.data(p)  , Uninitialized, d * Int(2) );
                fill_buffer( E_turn.data(p)     , Turn_T(0)    , d * Int(2) );
                fill_buffer( E_dir.data(p)      , NoDir        , d          );
                fill_buffer( E_flag.data(p)     , EdgeFlag_T(0), d * Int(2) );
            }
        }
        
    private:

#include "OrthoDraw/Bends_MCF.hpp"
#include "OrthoDraw/Bends.hpp"
#ifdef KNOODLE_USE_CLP
#include "OrthoDraw/Bends_CLP.hpp"
#endif

#include "OrthoDraw/LoadPlanarDiagram.hpp"
#include "OrthoDraw/Vertices.hpp"
#include "OrthoDraw/Edges.hpp"
#include "OrthoDraw/Regions.hpp"
#include "OrthoDraw/TurnRegularize.hpp"
        
#include "OrthoDraw/SaturateRegions.hpp"
#include "OrthoDraw/ConstraintGraphs.hpp"
#include "OrthoDraw/Compaction_TopologicalOrdering.hpp"
#include "OrthoDraw/Compaction_TopologicalNumbering.hpp"
#include "OrthoDraw/Compaction_MCF.hpp"
#ifdef KNOODLE_USE_CLP
#include "OrthoDraw/Compaction_CLP.hpp"
#endif
        
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
        
        Int ExteriorFace() const
        {
            return exterior_face;
        }
        
        Int FaceCount() const
        {
            return F_dA.SublistCount();
        }
        
        cref<RaggedList<Int,Int>> FaceDarcs() const
        {
            return F_dA;
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
                        Tiny::Matrix<2,2,Int,Int> C ( C_A.data(c) );
                        A_next_A(C[In][Left ]) = C[Out][Right];
                        A_next_A(C[In][Right]) = C[Out][Left ];
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
        
        cref<Settings_T> Settings() const
        {
            return settings;
        }
        
        void PrintSettings() const
        {
            logvalprint(MethodName("Settings()"), ToString(settings));
        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return std::string("OrthoDraw<") + PD_T::ClassName() + ">";
        }
        
    }; // class OrthoDraw
    
} // namespace Knoodle



