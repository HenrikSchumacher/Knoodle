#pragma once

namespace KnotTools
{
    
    template<typename Real_, typename Int_>
    class alignas( ObjectAlignment ) LinearHomotopy_3D
    {
        
    public:
                
        using Real = Real_;
        using Int  = Int_;
        using SInt = char;
        
//        static constexpr Int i_0 = 26 - 1;
//        static constexpr Int j_0 = 56 - 1;
        
        static constexpr Int i_0 = - 1;
        static constexpr Int j_0 = - 1;
        
        using UInt         = Scalar::Unsigned<Int>;
        
        using Link_T       = Link_3D<Real,Int>;
        using Tree_T       = typename Link_T::Tree_T;
        
        using BContainer_T = typename Tree_T::BContainer_T;
        using EContainer_T = typename Link_T::EContainer_T;
        
        using Collision_T  = Collision<Real,Int>;
        
        
        using Vector2_T    = typename Tiny::Vector<2,Real,Int>;
        using Vector3_T    = typename Tiny::Vector<3,Real,Int>;
        
        // We discard the first collision if it happens earlier than this.
        // The idea here is that at a splice we will have an immediate collision by construction.
        // But we want to ignore this!
        static constexpr Real first_collision_tolerance = 8 * Scalar::eps<Real>;
        
        // Maximal depth of block cluster tree.
        static constexpr Int max_depth = 128;
        
        static constexpr Real zero    = 0;
        static constexpr Real one     = 1;
        static constexpr Real two     = 2;
        static constexpr Real three   = 3;
        static constexpr Real eps     = 128 * Scalar::eps<Real>;
        static constexpr Real infty   = Scalar::Max<Real>;
        
        static constexpr Int max_disk_pts = 8;
        
    protected:
        
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
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

/////////////////////////////////////////////////////////////////////////////////////
//////         Constructors                                                       ///
/////////////////////////////////////////////////////////////////////////////////////

    public:
        
        LinearHomotopy_3D() = default;
        
        // Initialization from precomputed edge coodinates and bounding boxes.
        // This is usedful when handling piecewise-linear homotopies, because this data can be reused.
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
        
        // Initialization by times and vertex positions.
        LinearHomotopy_3D(
            mref<Link_T> L_,
            const Real T_0_, cptr<Real> P_0,
            const Real T_1_, cptr<Real> P_1
        )
        :   L           { L_                            }
        ,   T           { L.Tree()                      }
        ,   T_0         { T_0_                          }
        ,   T_1         { T_1_                          }
        ,   DeltaT      { T_1 - T_0                     }
        ,   E_0_buffer  { L.EdgeCount(), 2, 3           }
        ,   E_1_buffer  { L.EdgeCount(), 2, 3           }
        ,   B_0_buffer  { L.Tree().NodeCount(), 3, 2    }
        ,   B_1_buffer  { L.Tree().NodeCount(), 3, 2    }
        ,   E_0         { E_0_buffer                    }
        ,   E_1         { E_1_buffer                    }
        ,   B_0         { B_0_buffer                    }
        ,   B_1         { B_1_buffer                    }
        {
            L.template ReadVertexCoordinates<false>( P_0, E_0_buffer );
            L.template ReadVertexCoordinates<false>( P_1, E_1_buffer );

            L.Tree().ComputeBoundingBoxes( E_0, B_0_buffer );
            L.Tree().ComputeBoundingBoxes( E_1, B_1_buffer );
        }
        
        ~LinearHomotopy_3D() = default;
        
/////////////////////////////////////////////////////////////////////////////////////
//////         Interface                                                          ///
/////////////////////////////////////////////////////////////////////////////////////

    public:
            
        cref<Link_T> Link() const
        {
            return L;
        }
        
        cref<Tree_T> Tree() const
        {
            return T;
        }
        
        Int EdgeCount() const
        {
            return L.EdgeCount();
        }
        
        
        Tensor1<Real,Int> ExportCollisionTimes() const
        {
            const Int n = static_cast<Int>(collisions.size());
            
            Tensor1<Real,Int> times ( n );

            for( Int k = 0; k < n; ++k )
            {
                times[k] = collisions[k].time;
            }
            
            return times;
        }
        
        Tensor2<Real,Int> ExportCollisionParameters() const
        {
            const Int n = static_cast<Int>(collisions.size());
            
            Tensor2<Real,Int> times ( n, 2 );

            for( Int k = 0; k < n; ++k )
            {
                times[k][0] = collisions[k].z[0];
                times[k][1] = collisions[k].z[1];
            }
            
            return times;
        }
        
        Tensor2<Real,Int> ExportCollisionPoints() const
        {
            const Int n = static_cast<Int>(collisions.size());
            
            Tensor2<Real,Int> points ( n, static_cast<Int>(3) );
            
            for( Int k = 0; k < n; ++k )
            {
                copy_buffer<3>( &collisions[k].point[0], points.data(k));
            }
            return points;
        }
        
        Tensor1<Int,Int> ExportCollisionFlags() const
        {
            const Int n = static_cast<Int>(collisions.size());
            
            Tensor1<Int,Int> times ( n );

            for( Int k = 0; k < n; ++k )
            {
                times[k] = collisions[k].flag;
            }
            
            return times;
        }
        
        

        
/////////////////////////////////////////////////////////////////////////////////////
//////         Collision                                                          ///
/////////////////////////////////////////////////////////////////////////////////////
        
    public:
        
        void ClearCollisionData()
        {
            collisions.clear();
            
            test_counter = 0;
        }
        
        Int CollisionCount() const
        {
            return static_cast<Int>(collisions.size());
        }
        
        Int CollisionTestCount() const
        {
            return static_cast<Int>(test_counter);
        }
        
        cref<Collision_T> Collision( const Int k ) const
        {
            return collisions[k];
        }
        
        void WriteCollisionTriples( mptr<Real> triples ) const
        {
            // Suppose the homotopy is parameterized by X : [T_0,T_1] x [0,1] -> R^3.
            
            // This function exports all triples (t,x,y) such that X(t,x) = X(t,y).
            
            const Real Delta_x = Frac<Real>(1,EdgeCount());
            
            
            for( Int k = 0; k < CollisionCount(); ++k )
            {
                cref<Collision_T> C = collisions[k];
                
                triples[3 * k + 0] = T_0 + DeltaT * C.time;
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
            
            return Min( d, component_size + static_cast<Int>(1) - d);
        }
        
        
        Real CollisionTime() const
        {
            Real T = Scalar::Infty<Real>;
            
            for( Int k = 0; k < CollisionCount(); ++k )
            {
                cref<Collision_T> C = collisions[k];
                
                T = Min( T, C.time );
            }
            
            return T;
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
        
/////////////////////////////////////////////////////////////////////////////////////
//////         Debugging Tools                                                     ///
/////////////////////////////////////////////////////////////////////////////////////
        
        bool NodesContainEdgesQ( const Int node_0, const Int node_1 ) const
        {
            if constexpr ( i_0 >= 0 && j_0 >= 0 )
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
        
        static std::string ClassName()
        {
            return std::string("LinearHomotopy_3D")+"<"+TypeName<Real>+","+TypeName<Int>+">";
        }

    }; // LinearHomotopy_3D
    
} // namespace KnotTools
