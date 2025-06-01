#pragma once

#include "../submodules/Tensors/Clp.hpp"

namespace Knoodle
{
    // TODO: Child class TurnRegularOrthogonalRepresentation?
    // Or just use a flag for that?
    
    template<typename Int_>
    class OrthogonalRepresentation
    {
    public:
        
        using Int       = Int_;
        using SInt      = Int8;
        using COIN_Real = double;
        using COIN_Int  = int;
        using COIN_LInt = CoinBigIndex;
        
        using UInt      = UInt32;
        
//        using Graph_T           = MultiGraph<Int,Int>;
        
        using PlanarDiagram_T     = PlanarDiagram<Int>;
        using VertexContainer_T   = Tiny::VectorList_AoS<4, Int,Int>;
        using EdgeContainer_T     = Tiny::VectorList_AoS<2, Int,Int>;
        using EdgeTurnContainer_T = Tiny::VectorList_AoS<2,SInt,Int>;
        using DirectedArcNode     = PlanarDiagram_T::DirectedArcNode;
        
        using Dir_T = SInt;
        
        static constexpr Dir_T East  =  0;
        static constexpr Dir_T North =  1;
        static constexpr Dir_T West  =  2;
        static constexpr Dir_T South =  3;
        
        static constexpr Dir_T NoDir = -1;
        
        static constexpr bool Left  = PlanarDiagram_T::Left;
        static constexpr bool Right = PlanarDiagram_T::Right;
        
        static constexpr bool Out   = PlanarDiagram_T::Out;
        static constexpr bool In    = PlanarDiagram_T::In;
        
        static constexpr bool Tail  = PlanarDiagram_T::Tail;
        static constexpr bool Head  = PlanarDiagram_T::Head;
        
        Dir_T TurnLeft( Dir_T dir )
        {
            switch( dir )
            {
                case East:  return North;
                case North: return West;
                case West:  return South;
                case South: return East;
                case NoDir: return NoDir;
            }
        }
        
        Dir_T TurnRight( Dir_T dir )
        {
            switch( dir )
            {
                case East:  return South;
                case North: return East;
                case West:  return North;
                case South: return West;
                case NoDir: return NoDir;
            }
        }
        
        Dir_T TurnAround( Dir_T dir )
        {
            switch( dir )
            {
                case East:  return West;
                case North: return South;
                case West:  return East;
                case South: return North;
                case NoDir: return NoDir;
            }
        }
        
        OrthogonalRepresentation() = default;
        
        // Copy constructor
        OrthogonalRepresentation( const OrthogonalRepresentation & other ) = default;
        
        

        UInt getArcOrientation( bool io, bool lr )
        {
            UInt d =  (io ? ( UInt(2) + lr ): !lr);
//            d -= rot;
            d %= 4;
            
            return d;
        }
        
        OrthogonalRepresentation(
            mref<PlanarDiagram_T> pd, const Int exterior_face
        )
        {
            A_bends = BendsByLP(pd,exterior_face);
            
            TOOLS_LOGDUMP(A_bends);

            const Int crossing_count = pd.CrossingCount();
            const Int arc_count      = pd.ArcCount();
            
            Int bend_count = 0;
            
            A_V_ptr = Tensor1<Int,Int>( arc_count + Int(1) );
            A_V_ptr[0] = 0;
            
            for( Int a = 0; a < arc_count; ++a )
            {
                const Int b = Abs(A_bends[a]);
                bend_count += b;
                A_V_ptr[a+1] = A_V_ptr[a] + b + Int(2);
            }
            
            const Int vertex_count = crossing_count + bend_count;
            const Int edge_count   = arc_count      + bend_count;
            
            TOOLS_LOGDUMP(crossing_count);
            TOOLS_LOGDUMP(arc_count);
            TOOLS_LOGDUMP(bend_count);
            TOOLS_LOGDUMP(vertex_count);
            TOOLS_LOGDUMP(edge_count);
            
            // TODO: Fill this correctly!
            V_E = VertexContainer_T( vertex_count );
            V_E.Fill(Int(-1)); 
            E_V = EdgeContainer_T  ( edge_count   );
            E_V.Fill(Int(-1));
            
            // E_turn[2*a+Head] = turn between a and NextLeftArc(a,Head);
            // E_turn[2*a+Tail] = turn between a and NextLeftArc(a,Tail);
            E_turn  = EdgeTurnContainer_T( edge_count );
            // TODO: Remove filling.
            Tensor1<SInt,Int> C_dir ( crossing_count, SInt(-1) );

            E_dir   = Tensor1<SInt,Int>( edge_count );
            E_A     = Tensor1< Int,Int>( edge_count );
            // TODO: Remove filling.
            A_V_idx = Tensor1< Int,Int>( A_V_ptr.Last(), Int(-1) );
            
            TOOLS_DUMP(A_V_idx);
            
            const auto & C_A = pd.Crossings();
            const auto & A_C = pd.Arcs();
            
            // Vertices, 0,..., pd.CrossingCount()-1 correspond to crossings 0,..., pd.CrossingCount()-1.
            // We tranverse the planar diagram and insert additional vertices on the arcs for each of the bends as we go.
            Int V_counter = crossing_count;
            Int E_counter = arc_count;

            auto fun = [&,this]( cref<DirectedArcNode> da )
            {
                if( da.a < Int(0) ) { return; }

                const Int  a = (da.a >> 1);
//                const bool d = (da.a | Int(1));
                
                logprint("=====================");
                logprint("fun");
                TOOLS_LOGDUMP(a);
                
                const Int  pos    = A_V_ptr[a];
                const Int  b      = A_bends[a];
                const Int  abs_b  = Abs(b);
                const SInt sign_b = Sign<SInt>(b);     // bend per turn.

                const Int  c_0    = A_C(a,Tail);
                const Int  c_1    = A_C(a,Head);

                TOOLS_LOGDUMP(pos);
                TOOLS_LOGDUMP(b);
                TOOLS_LOGDUMP(sign_b);
                TOOLS_LOGDUMP(c_0);
                TOOLS_LOGDUMP(c_1);
                
                TOOLS_LOGDUMP(C_dir[c_0]);
                TOOLS_LOGDUMP(C_dir[c_1]);


//               |                arc a                  |
//            c_0|        v_1       v_2       v_3        |c_1
//               X-------->+-------->+-------->+-------->X
//               |    a        e_1       e_2       e_3   |
//               |                                       |

                
                A_V_idx[pos] = c_0;

                Int e = a;
                Int v = c_0;
                
                TOOLS_LOGDUMP(e);
                TOOLS_LOGDUMP(v);

                const bool side_0 = (C_A(c_0,Out,Right) == a);
//                const bool side_1 = (C_A(c_1,In ,Right) == a);
                UInt e_dir = getArcOrientation(Out,side_0) + C_dir[c_0];
                e_dir %= 4;
                TOOLS_LOGDUMP(e_dir);
                
                E_A[e] = a;
                E_V(e,Tail) = c_0;
                
                E_dir[e] = static_cast<SInt>(e_dir);
                
                TOOLS_LOGDUMP(C_dir[c_0]);
                
                Int slot_0 = (e_dir - C_dir[c_0]) % 4;
                TOOLS_LOGDUMP(slot_0);
                V_E(c_0,slot_0) = e;
                
                logvalprint("V_E("+ToString(c_0)+","+ToString(slot_0)+")",V_E(c_0,slot_0));
                
//               |
//            c_0|   e     ?
//               X-------->+--
//               |(*)
//               | +----------------------+
//                                        v
                E_turn(e,Tail) = SInt(1);
                
                // We use f for storing the "previous edge".
                // We use e for storing the "current edge".
                
                Int f = e;
                
                logprint("Inserting additional corners and edges.");
                // Inserting additional corners and edges
                for( Int k = 0; k < abs_b; ++k )
                {
                    v = V_counter++;
                    e = E_counter++;
                    
                    TOOLS_LOGDUMP(e);
                    TOOLS_LOGDUMP(v);

//                                 + ??
//                                 ^
//                                 |
//                                 | e
//                    ???     f    |
//                       +-------->+ v

                    V_E(v,(e_dir + UInt(2) - C_dir[v]) % 4) = f;
                    
                    e_dir   += sign_b;
                    e_dir   %= 4;
                    E_dir[e] = static_cast<SInt>(e_dir);
                    TOOLS_LOGDUMP(e_dir);
                    
                    V_E(v,(e_dir - C_dir[v]) % 4) = e;
                    
                    E_A[e] = a;
                    A_V_idx[pos + 1 + k] = v;
                    
                    E_V(f,Head) = v; E_turn(f,Head) =  sign_b;
                    E_V(e,Tail) = v; E_turn(e,Tail) = -sign_b;
                    
                    
                    
                    f = e;
                }
                logprint("Insertion done.");
                
                A_V_idx[pos + abs_b + 1] = c_1;
                
//                               |
//                     v      (*)|c_1
//                     +-------->X
//                          e    |
//                               |
                
                E_V(e,Head) = c_1; E_turn(e,Head) = SInt(1);
                
                TOOLS_LOGDUMP(e_dir);
                TOOLS_LOGDUMP(C_dir[c_1]);
                
                Int slot_1 = (e_dir + UInt(2) - C_dir[c_1]) % 4;
                
                TOOLS_LOGDUMP(slot_1);
                
                V_E(c_1,slot_1) = e;
                logvalprint("V_E(c_1,slot_1)",V_E(c_1,slot_1));
                
                logvalprint("V_E.data("+ToString(c_0)+")",ArrayToString(V_E.data(c_0),{4}));
                logvalprint("V_E.data("+ToString(c_1)+")",ArrayToString(V_E.data(c_1),{4}));
                
                logvalprint("E_V.data("+ToString(e)+")",ArrayToString(E_V.data(e),{2}));
                
                logprint("=====================");
            };
            
            auto fun_discover = [&fun,&C_dir,&C_A,this]( cref<DirectedArcNode> da )
            {
//                logprint("=====================");
//                logprint("=====================");
//                logprint("fun_discover");
//
//                
//                TOOLS_LOGDUMP(da.tail);
//                TOOLS_LOGDUMP(da.a);
//                TOOLS_LOGDUMP(da.head);
                
                if( da.a < Int(0) )
                {
//                    logprint("case A");
                    C_dir[da.head] = SInt(0);
                    
//                    TOOLS_LOGDUMP(C_dir[da.head]);
                }
                else
                {
//                    logprint("case B");
                    Int  a  = da.a / 2;
                    bool d  = da.a % 2;

                    const Int c_0 = da.tail;
                    const Int c_1 = da.head;
                    
//                    TOOLS_LOGDUMP(a);
//                    TOOLS_LOGDUMP(d);
//                    TOOLS_LOGDUMP(c_0);
//                    TOOLS_LOGDUMP(c_1);
                    
                    const bool io_0 = !d;
                    const bool lr_0 = (C_A(c_0,io_0,Right) == a);
                    
//                    TOOLS_LOGDUMP(io_0);
//                    TOOLS_LOGDUMP(lr_0);
//                    TOOLS_LOGDUMP(C_A(c_0,io_0,lr_0));
                    
                    // Direction where a would leave the standard-oriented port.
                    UInt a_dir = getArcOrientation(io_0,lr_0);
//                    TOOLS_LOGDUMP(a_dir);
                    // Take orientation of c_0 into account.
                    a_dir += C_dir[c_0];
                    
//                    TOOLS_LOGDUMP(A_bends[a]);
                    // Take bends into account.
                    a_dir += (d ? A_bends[a] : -A_bends[a]);
                    // Arc enters through opposite direction.
                    a_dir += 2;
                    
                    // a_dir % 4 is the port to dock to.
                    const bool io_1 = d;
                    const bool lr_1 = (C_A(c_1,io_1,Right) == a);
                    
//                    TOOLS_LOGDUMP(io_1);
//                    TOOLS_LOGDUMP(lr_1);
//                    TOOLS_LOGDUMP(C_A(c_1,io_1,lr_1));
                    // Now we have to rotate c_1 by rot so that C_A(c_1,io_1,lr_1) equals a_dir:
                    // getArcOrientation(io_1,lr_1) + C_dir[c_1] == a_dir mod 4
                    
                    C_dir[c_1] = (a_dir - getArcOrientation(io_1,lr_1)) % 4;
                    
//                    logprint("updated dir");
//                    TOOLS_LOGDUMP(C_dir[c_0]);
//                    TOOLS_LOGDUMP(C_dir[c_1]);
                }
                fun(da);
            };
            
            logprint("begin DepthFirstSearch");
            pd.DepthFirstSearch(
                fun_discover,                           // discover
                fun,                                    // rediscover
                PlanarDiagram_T::TrivialArcFunction,    // previsit
                PlanarDiagram_T::TrivialArcFunction     // postvisit
            );
            logprint("end DepthFirstSearch");
            
            TOOLS_DUMP(C_dir);
            TOOLS_DUMP(E_dir);
        }
                                 
        ~OrthogonalRepresentation() = default;
        
    private:
        
        Tensor1<Int,Int>    A_bends;
        
        VertexContainer_T   V_E;
        EdgeContainer_T     E_V;
        
        // This ought to map directed edges to directed edges.
        Tensor1<Int,Int>    E_next_left_E;
        
        // This ought to map directed edges to the turn at the _tail_.
        EdgeTurnContainer_T E_turn;
        
        Tensor1<SInt,Int>   E_dir;
        Tensor1<Int,Int>    C_V;
        // M
        Tensor1<Int,Int>    E_A;
    
        Tensor1<Int,Int>    A_V_ptr;
        Tensor1<Int,Int>    A_V_idx;
        
        bool proven_turn_regularQ = false;
        
    private:
        
#include "OrthogonalRepresentation/BendsSystemLP.hpp"
#include "OrthogonalRepresentation/BendsByLP.hpp"
        
    public:
        
        Int VertexCount() const
        {
            return V_E.Dimension(0);
        }
        
        cref<VertexContainer_T> Vertices() const
        {
            return V_E;
        }
        
        Int EdgeCount() const
        {
            return E_V.Dimension(0);
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
        
        cref<EdgeTurnContainer_T> DirectedEdgeTurns() const
        {
            return E_turn;
        }
        
        cref<Tensor1<SInt,Int>> EdgeDirections() const
        {
            return E_dir;
        }
        
        
        cref<Tensor1<Int,Int>> Bends() const
        {
            return A_bends;
        }
        
        
//        OrthogonalRepresentation RegularizeTurnsByKittyCorners()
//        {
//            // TODO: Compute the faces.
//            // TODO: Check faces for turn-regularity/detect kitty-corners.
//            // TODO: Resolve kitty-corners.
//            //      TODO: Do that horizontally/vertically in alternating fashion.
//            //      TODO: Do that randomly.
//            // TODO: Update and forward the map from original crossings to vertices.
//            // TODO: Update and forward the map from edges to original arcs.
//        }
  
//        Not that important:
//        OrthogonalRepresentation RegularizeTurnsByReactangles()
//        {
//            // TODO: Compute the faces.
//            // TODO: Check faces for turn-regularity/detect kitty-corners.
//            // TODO: Resolve by using rectangles.
//            // TODO: Update and forward the map from original crossings to vertices.
//            // TODO: Update and forward the map from edges to original arcs.
//        }

        
    public:
        
        static std::string ClassName()
        {
            return std::string("OrthogonalRepresentation<") + TypeName<Int> + ">";
        }
        
    }; // class OrthogonalRepresentation
    
} // namespace Knoodle
