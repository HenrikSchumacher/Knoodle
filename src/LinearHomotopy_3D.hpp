#pragma once

namespace Knoodle
{
    /*!@brief **EXPERIMENTAL** This loads two sets of vertex coordinates for a `Link_3D` and provides means to check whether the linear homotopy between the two arising link embeddings is an isotopy. The main routine is `RequireCollisions`.
     *
     * CAUTION: This uses computations in double precision and root finding of a polynomials of order 3. This may have severe accuracy issues, e.g., when the distance between the line segments of the same edge at time `T_0` and `T_1` are large compared to the lengths of these line segments. For example, I experimenced this when checking large updates of a gradient flow of a very finely sampled polygon (`edge_count` much greater than 10000). Therefore, this class is tagged **EXPERIMENTAL**.
     */
    
    template<FloatQ Real_, IntQ Int_>
    class alignas( ObjectAlignment ) LinearHomotopy_3D final
    {
        
    public:
                
        using Real = Real_;
        using Int  = Int_;
        
//        static constexpr Int i_0 = 26 - 1;
//        static constexpr Int j_0 = 56 - 1;
        
        // For debugging only.
        static constexpr Int i_0 = - 1;
        static constexpr Int j_0 = - 1;
        
        using UInt         = ToUnsigned<Int>;
        
        using Link_T       = Link_3D<Real,Int>;
        using Tree_T       = typename Link_T::Tree_T;
        
        using BContainer_T = typename Tree_T::BContainer_T;
        using EContainer_T = typename Link_T::EContainer_T;
        
        using Vector2_T    = typename Tiny::Vector<2,Real,Int>;
        using Vector3_T    = typename Tiny::Vector<3,Real,Int>;
        
        // We discard the first collision if it happens earlier than this.
        // The idea here is that at a splice we will have an immediate collision by construction.
        // But we want to ignore this!
        static constexpr Real first_collision_tolerance = 8 * Scalar::eps<Real>;
        
        static_assert(std::in_range<Int>(4 * 128 + 1),"");
        
        // Maximal depth of block cluster tree.
        static constexpr Int max_depth = 128;
        
        static constexpr Real zero    = 0;
        static constexpr Real one     = 1;
        static constexpr Real two     = 2;
        static constexpr Real three   = 3;
        static constexpr Real eps     = 128 * Scalar::eps<Real>;
        static constexpr Real infty   = Scalar::Max<Real>;
        
        static constexpr Int max_disk_pts = 8;
        
//        using Collision_T  = Collision<Real,Int>;
        
        struct Collision_T
        {
            Real time;
            Vector3_T point;
            Vector2_T z;
            Int  i;
            Int  j;
            Int  flag;
            
            Collision_T(
                cref<Real> time_,
                cref<Vector3_T> point_,
                cref<Vector2_T> z_,
                cref<Int> i_,
                cref<Int> j_,
                cref<Int> flag_
            )
            :   time        ( time_  )
            ,   point       ( point_ )
            ,   z           ( z_     )
            ,   i           ( i_     )
            ,   j           ( j_     )
            ,   flag        ( flag_  )
            {}
        };
        
    protected:
        
        cref<Link_T> L;
        
        cref<Tree_T> T;
        
        const Real T_0;
        const Real T_1;
        const Real DeltaT;
        
//        cref<VContainer_T> V_0;
//        cref<VContainer_T> V_1;
        
        // Only needed when initialized from vertex coordinates.
        EContainer_T E_0_buffer;
        EContainer_T E_1_buffer;
        BContainer_T B_0_buffer;
        BContainer_T B_1_buffer;
        
        cref<EContainer_T> E_0;
        cref<EContainer_T> E_1;
        
        cref<BContainer_T> B_0;
        cref<BContainer_T> B_1;
        
        // Format for boxes
        // Real B_0 [3][2][edge_count];
        // First  index: coordinate axis (3D)
        // Second index: min/max (min = 0, max = 1)
        // Third  index: tree node index
        
        
        std::vector<Collision_T> collisions;
        
        Size_T test_counter;
        Size_T first_collision = 0;
        Real time = Scalar::Infty<Real>;
        bool collisions_computedQ = false;
        
//###################################################################################
//####         Constructors                                                      ####
//###################################################################################
        
    public:
        
        /*!@brief Initialization from precomputed edge coordinates and bounding boxes. This is useful when handling piecewise-linear homotopies, because this data can be reused.
         */
        LinearHomotopy_3D(
            mref<Link_T> L_,
            const Real T_0_, cref<EContainer_T> E_0_, cref<BContainer_T> B_0_,
            const Real T_1_, cref<EContainer_T> E_1_, cref<BContainer_T> B_1_
        )
        :   L       { L_        }
        ,   T       { L.Tree()  }
        ,   T_0     { T_0_      }
        ,   T_1     { T_1_      }
        ,   DeltaT  { T_1 - T_0 }
        ,   E_0     { E_0_      }
        ,   E_1     { E_1_      }
        ,   B_0     { B_0_      }
        ,   B_1     { B_1_      }
        {}
        
        /*!@brief Initialization from times and vertex positions.
         *
         * Denote the piecewise-linear interpolations of `P_0` and `P_1` by \f$f_0 \colon M \to R^3\f$ and \f$f_1 \colon M \to R^3\f$, where \f$M\f$ is some closed one-dimensional manifold. Then the homotopy presented by this objects is the following:
         * \f\[
         *      H \colon [T_0,T_1] \times M \to R^3,
         *      \quad
         *      H(t,x) = \frac{t - T_0}{T_1 - T_0} \, f_0(x) + \frac{T_1 - t}{T_1 - T_0} \, f_1(x).
         *   \f\]
         */
        LinearHomotopy_3D(
            mref<Link_T> L_,
            const Real T_0_, cptr<Real> P_0,
            const Real T_1_, cptr<Real> P_1
        )
        :   L           { L_                   }
        ,   T           { L.Tree()             }
        ,   T_0         { T_0_                 }
        ,   T_1         { T_1_                 }
        ,   DeltaT      { T_1 - T_0            }
        ,   E_0_buffer  { L.EdgeCount()        }
        ,   E_1_buffer  { L.EdgeCount()        }
        ,   B_0_buffer  { L.Tree().NodeCount() }
        ,   B_1_buffer  { L.Tree().NodeCount() }
        ,   E_0         { E_0_buffer           }
        ,   E_1         { E_1_buffer           }
        ,   B_0         { B_0_buffer           }
        ,   B_1         { B_1_buffer           }
        {
            L.template ReadVertexCoordinates<false>( P_0, E_0_buffer );
            L.template ReadVertexCoordinates<false>( P_1, E_1_buffer );

            
            L.Tree().template ComputeBoundingBoxes<2,3>( E_0.data(), B_0_buffer.data() );
            L.Tree().template ComputeBoundingBoxes<2,3>( E_1.data(), B_1_buffer.data() );
        }
        
        // Default constructor
        LinearHomotopy_3D() = default;
        // Destructor
        ~LinearHomotopy_3D() = default;
        // Copy constructor
        LinearHomotopy_3D( const LinearHomotopy_3D & other ) = default;
        // Copy assignment operator
        LinearHomotopy_3D & operator=( const LinearHomotopy_3D & other ) = default;
        // Move constructor
        LinearHomotopy_3D( LinearHomotopy_3D && other ) = default;
        // Move assignment operator
        LinearHomotopy_3D & operator=( LinearHomotopy_3D && other ) = default;

//###################################################################################
//####         Interface                                                         ####
//###################################################################################
        
    public:
            
        cref<Link_T> Link() const
        {
            return L;
        }
        
        cref<Tree_T> Tree() const
        {
            return T;
        }
        
        /*!@brief Return the number of edges in the piecewise linear curve(s).*/
        Int EdgeCount() const
        {
            return L.EdgeCount();
        }
        
        /*!@brief Return a list with all collision xtimes in the interval `[T_0,T_1]`.*/
        Tensor1<Real,Int> ExportCollisionTimes()
        {
            RequireCollisions();
            
            const Int n = int_cast<Int>(collisions.size());
            
            Tensor1<Real,Int> times ( n );

            for( Int k = 0; k < n; ++k )
            {
                times[k] = collisions[k].time;
            }
            
            return times;
        }
        
        
        /*!@brief Return the initial time `T_0` of the homotopy.*/
        Real InitialTime() const { return T_0; }
        
        /*!@brief Return the final time `T_1` of the homotopy.*/
        Real FinalTime() const { return T_1; }

        /*!@brief Return the internal list of collisions.*/
        std::vector<Collision_T> Collisions()
        {
            RequireCollisions();

            return collisions;
        }

        /*!@brief Return the earliest collision time in the interval `[T_0,T_1]`.*/
        Real EarliestCollisionTime()
        {
            RequireCollisions();

            return time;
        }
        
        [[deprecated("Superseded by EarliestCollisionIndex because in makes the intention clearer.")]]
        /*!@brief Return the earliest collision time in the interval `[T_0,T_1]`.*/
        Real CollisionTime()
        {
            RequireCollisions();

            return time;
        }
        
        /*!@brief Return the position of the position with the earliest collision time in the list `Collisions()`.*/
        Int EarliestCollisionIndex()
        {
            RequireCollisions();

            return first_collision;
        }
        
        
        /*!@brief Return the a list containing for each collision parameters `{x,y}` of collision _relative to the corresponding edges.
         *
         * I.e., if `x = 0`, `y = 0.5`, then the point of collision is the left end point of the first edge and the midpoint of the second edge.
         * */
        Tensor2<Real,Int> ExportCollisionParameters()
        {
            RequireCollisions();
            
            const Int n = int_cast<Int>(collisions.size());
            
            Tensor2<Real,Int> times ( n, 2 );

            for( Int k = 0; k < n; ++k )
            {
                times[k][0] = collisions[k].z[0];
                times[k][1] = collisions[k].z[1];
            }
            
            return times;
        }
        
        /*!@brief Return the a list containing for each collision the collision point in 3-space.*/
        Tensor2<Real,Int> ExportCollisionPoints()
        {
            RequireCollisions();
            
            const Int n = int_cast<Int>(collisions.size());
            
            Tensor2<Real,Int> points ( n, Int(3) );
            
            for( Int k = 0; k < n; ++k )
            {
                copy_buffer<3>( &collisions[k].point[0], points.data(k));
            }
            return points;
        }
        
        /*!@brief Return the a list containing for each collision the pairs corresponding edge indices  `{i,j}`.*/
        Tensor2<Int,Int> ExportCollisionEdgePairs()
        {
            RequireCollisions();
            
            const Int n = int_cast<Int>(collisions.size());
            
            Tensor2<Int,Int> edge_pairs ( n, Int(2) );
            
            for( Int k = 0; k < n; ++k )
            {
                edge_pairs(k,0) = collisions[k].i;
                edge_pairs(k,1) = collisions[k].j;
            }
            return edge_pairs;
        }
        
        /*!@brief Return the a list containing for each collision the flag raised by the collision finder.*/
        Tensor1<Int,Int> ExportCollisionFlags()
        {
            RequireCollisions();
            
            const Int n = int_cast<Int>(collisions.size());
            
            Tensor1<Int,Int> times ( n );

            for( Int k = 0; k < n; ++k )
            {
                times[k] = collisions[k].flag;
            }
            
            return times;
        }
        
//###################################################################################
//####         Access                                                           ####
//###################################################################################
        
        /*!@brief Return the data of the line segments at time `T_0`.*/
        cref<EContainer_T> EdgeData0() const
        {
            return E_0;
        }
        
        /*!@brief Return the data of the line segments at time `T_1`.*/
        cref<EContainer_T> EdgeData1() const
        {
            return E_1;
        }
        
        /*!@brief Return bounding boxes at time `T_0`.*/
        cref<BContainer_T> BoundingBoxes0() const
        {
            return B_0;
        }
        
        /*!@brief Return bounding boxes at time `T_1`.*/
        cref<BContainer_T> BoundingBoxes1() const
        {
            return B_1;
        }

        
//###################################################################################
//####         Collision                                                         ####
//###################################################################################
        
    public:
        
        /*!@brief Erase data of the collisions computed so that a new call to `RequireCollisions` will recompute them.*/
        void ClearCollisionData()
        {
            collisions.clear();
            test_counter    = 0;
            first_collision = 0;
            time = T_1;
            collisions_computedQ = false;
        }
        
        /*!@brief Return the number of collisions in the time interval `[T_0,T_1]`.*/
        Size_T CollisionCount()
        {
            RequireCollisions();
            
            return collisions.size();
        }
        
        Size_T CollisionTestCount()
        {
            RequireCollisions();
            
            return int_cast<Int>(test_counter);
        }
        
        cref<Collision_T> GetCollision( const Int k )
        {
            RequireCollisions();
            
            return collisions[k];
        }
        
        
        
        void WriteCollisionTriples( mptr<Real> triples )
        {
            RequireCollisions();
            
            // Suppose the homotopy is parameterized by X : [T_0,T_1] x [0,1] -> R^3.
            
            // This function exports all triples (t,x,y) such that X(t,x) = X(t,y).
            
            const Real Delta_x = Frac<Real>(1,EdgeCount());
            
            const Size_T k_count = CollisionCount();
             
            for( Size_T k = 0; k < k_count; ++k )
            {
                cref<Collision_T> C = collisions[k];
                
                triples[3 * k + 0] = C.time;
                triples[3 * k + 1] = Delta_x * ( C.i + C.z[0] );
                triples[3 * k + 2] = Delta_x * ( C.j + C.z[1] );
            }
        }
        
#include "LinearHomotopy_3D/FindCollisions.hpp"
        
#include "LinearHomotopy_3D/MovingBoxesCollidingQ.hpp"
        
#include "LinearHomotopy_3D/MovingEdgeCollisions.hpp"
//#include "LinearHomotopy_3D/MovingEdgeCollisions_2.hpp"
        
    public:

        Int EdgeIndexDistance( const Int i, const Int j, const Int c ) const
        {
            const Int d = std::abs(i-j);
            
            const Int component_size = L.ComponentEnd(c) - L.ComponentBegin(c);
            
            return Min( d, component_size + Int(1) - d);
        }
        
        
    private:
        
/// TODO: Check triangles.
/// Loop over all edges between i and j.
///     - Compute the average of the vertices and inter and store it in center.
///     - Compute the AABB.
///     - Maybe use trapezoidal subdivision?
///
/// Finding collisions of this AABB with all AABBs of P_t would be too expensive as we would
/// have to compute the whole polygon and that would be O(n).
/// Instead, we can use the linear interpolations of B_0 and B_1!
/// Just use MovingBoxesCollidingQ for that!
///
/// For each interpolated leaf box hit (should be O(1) many), 
/// compute the _actual_ line segment at time t.
/// Then compute the intersection of the triangles { { center, v[k], v[k+1] } and check whether
/// they are within the arc between i and j.
        

//###################################################################################
//####         Debugging Tools                                                   ####
//###################################################################################
        
        bool NodesContainEdgesQ( const Int node_0, const Int node_1 ) const
        {
            if constexpr ( i_0 >= Int(0) && j_0 >= Int(0) )
            {
                return T.NodesContainEdgesQ( node_0, node_1, i_0, j_0 );
            }
            else
            {
                return false;
            }

            return false;
        }
        
    public:
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("LinearHomotopy_3D")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }

    }; // LinearHomotopy_3D
    
} // namespace Knoodle
