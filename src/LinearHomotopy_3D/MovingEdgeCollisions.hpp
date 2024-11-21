public:

    Real dot( cref<Vector3_T> u, cref<Vector3_T> v )
    {
        return Dot(u,v);
    }

    Vector3_T cross( cref<Vector3_T> u, cref<Vector3_T> v )
    {
        return Cross_Kahan(u,v);
    }

    Real det( cref<Vector3_T> u, cref<Vector3_T> v, cref<Vector3_T> w )
    {
        return Dot(Cross_Kahan(u,v),w);
    }


    void MovingEdgeCollisions( const Int i, const Int j )
    {
        if( L.EdgesAreNeighborsQ(i,j) )
        {
            if constexpr ( i_0 >= 0 && j_0 >= 0 )
            {
                if( i == i_0 && j == j_0 )
                {
                    print("L.EdgesAreNeighborsQ(i,j)");
                }
            }
            
            return;
        }

        Vector3_T t_list;
        
        Tiny::Matrix<2,3,Real,Int> E_0_i ( E_0.data(i) );
        Tiny::Matrix<2,3,Real,Int> E_1_i ( E_1.data(i) );
        Tiny::Matrix<2,3,Real,Int> E_0_j ( E_0.data(j) );
        Tiny::Matrix<2,3,Real,Int> E_1_j ( E_1.data(j) );

        const Vector3_T u_0 {
            E_0_i[1][0] - E_0_i[0][0],
            E_0_i[1][1] - E_0_i[0][1],
            E_0_i[1][2] - E_0_i[0][2]
        };
        
        const Vector3_T u_1 {
            E_1_i[1][0] - E_1_i[0][0] - u_0[0],
            E_1_i[1][1] - E_1_i[0][1] - u_0[1],
            E_1_i[1][2] - E_1_i[0][2] - u_0[2]
        };

        const Vector3_T v_0 {
            E_0_j[1][0] - E_0_j[0][0],
            E_0_j[1][1] - E_0_j[0][1],
            E_0_j[1][2] - E_0_j[0][2]
        };
        const Vector3_T v_1 {
            E_1_j[1][0] - E_1_j[0][0] - v_0[0],
            E_1_j[1][1] - E_1_j[0][1] - v_0[1],
            E_1_j[1][2] - E_1_j[0][2] - v_0[2]
        };

        const Vector3_T w_0 {
            E_0_j[0][0] - E_0_i[0][0],
            E_0_j[0][1] - E_0_i[0][1],
            E_0_j[0][2] - E_0_i[0][2]
        };
        
        const Vector3_T w_1 {
            E_1_j[0][0] - E_1_i[0][0] - w_0[0],
            E_1_j[0][1] - E_1_i[0][1] - w_0[1],
            E_1_j[0][2] - E_1_i[0][2] - w_0[2]
        };
        
        if constexpr ( i_0 >= 0 && j_0 >= 0 )
        {
            if( i == i_0 && j == j_0 )
            {
                dump(u_0);
                dump(u_1);
                
                dump(v_0);
                dump(v_1);
                
                dump(w_0);
                dump(w_1);
            }
        }
        
        // Necessary condition for intersection: The following vectors must be coplanar:
        // u(t) = u_0 * ( 1 - t ) + t u_1 // vector along first edge at time t
        // v(t) = v_0 * ( 1 - t ) + t v_1 // vector along second edge at time t
        // w(t) = w_0 * ( 1 - t ) + t w_1 // vector between the first point of both edges at time t
        //
        // That is, we have to watch out for a root of
        //
        // <u(t) x v(t), w(t)>
        //
        // This is a polynomial of order 3.
        //
        // Let's compute its coefficients.
        
        Vector3_T uv [2][2] = {
            { cross(u_0,v_0), cross(u_0,v_1) },
            { cross(u_1,v_0), cross(u_1,v_1) }
        };
        
        Tiny::Vector<4,Real,Int> coeff = {
            dot(uv[0][0],w_0),
            dot(uv[0][0],w_1) + dot(uv[0][1],w_0) + dot(uv[1][0],w_0),
            dot(uv[0][1],w_1) + dot(uv[1][0],w_1) + dot(uv[1][1],w_0),
            dot(uv[1][1],w_1)
        };
        
    //            auto f = [&coeff]( const Real t )
    //            {
    //                return ((coeff[3] * t + coeff[2]) * t + coeff[1]) * t + coeff[0];
    //            };
        
        
//        if( 
//           (coeff[3] >= 0 && coeff[2] >= 0 && coeff[1] >= 0 && coeff[0] >= 0)
//           ||
//           (coeff[3] <= 0 && coeff[2] <= 0 && coeff[1] <= 0 && coeff[0] <= 0)
//        )
//        {
//            return;
//        }
        
        // TODO: Is this the bottleneck? Can we do something about it?
        
        const int count = RealCubicSolve_UnitInterval_RegulaFalsi(
            coeff[3], coeff[2], coeff[1], coeff[0], t_list.data(), 0.0000000000001, 32
        );
        
//        const int count = RealCubicSolve_UnitInterval_Cardano(
//            coeff[3], coeff[2], coeff[1], coeff[0], t_list.data()
//        );
        
        if constexpr ( i_0 >= 0 && j_0 >= 0 )
        {
            if( i == i_0 && j == j_0 )
            {
                dump(coeff);
                dump(t_list);
            }
        }
        
        // We have up to three candidates in t_list
        for( Int k = 0; k < count; ++k )
        {
            const Real t = t_list[k];
            
            const Real t_abs = T_0 + t * DeltaT;
            
            // We discard the first collision if it happens earlier than this.
            // The idea here is that at a splice we will have an immediate collision by construction.
            // But we want to ignore this!

            
            if( t_abs <= first_collision_tolerance )
            {
                if constexpr ( i_0 >= 0 && j_0 >= 0 )
                {
                    if( i == i_0 && j == j_0 )
                    {
                        print("first_collision_tolerance");
                        
                        dump(t);
                        
                        dump(t_abs);
                    }
                }
                
                continue;
            }
            
            // We want to check whether there are z[0] in [0,1) and z[1] in [0,1) such that
            //     x_t[0] + u_t * z[0]     == y_t[0] + v_t * z[1]
            //
            // <=> u_t * z[0] - v_t * z[1] == y_t[0] - x_t[0]
            //
            // <=> u_t * z[0] - v_t * z[1] == w_t
            //
            // <=> <u_t, u_t> * z[0] - <u_t, v_t> * z[1] == <u_t,w_t>
            //     and
            //     <v_t, u_t> * z[0] - <v_t, v_t> * z[1] == <v_t,w_t>
            
            // We check this in the plane spanned by u_t and v_t.

            
            // first edge's vector at time t
            const Vector3_T u_t {
                u_0[0] + t * u_1[0],
                u_0[1] + t * u_1[1],
                u_0[2] + t * u_1[2],
            };
            
            // second edge's vector at time t
            const Vector3_T v_t {
                v_0[0] + t * v_1[0],
                v_0[1] + t * v_1[1],
                v_0[2] + t * v_1[2],
            };
            
            // second edge's vector at time t
            const Vector3_T w_t {
                w_0[0] + t * w_1[0],
                w_0[1] + t * w_1[1],
                w_0[2] + t * w_1[2],
            };
            
            if constexpr ( i_0 >= 0 && j_0 >= 0 )
            {
                if( i == i_0 && j == j_0 )
                {
                    dump(u_t);
                    dump(v_t);
                    dump(w_t);
                }
            }
            
// If tip and tail of one edge are entirely in the front of back of the other edge or vice versa, then we can exit here. This should deal with the case that both edges stem from a single edge by splitting it. In this case u_t and v_t will be (almost) linearly dependent and the remaining code here might work awfully.
//
// Cases to sieve out
//
// Second edge is entirely in front of first edge:
// Dot( y_t_0 - x_t_1, u_t ) >= 0 && Dot( y_t_1 - x_t_1, u_t ) >= 0
// Second edge is entirely in back of first edge:
// Dot( y_t_0 - x_t_0, u_t ) <= 0 && Dot( y_t_1 - x_t_0, u_t ) <= 0
//
// First edge is entirely in front of second edge:
// Dot( x_t_0 - y_t_1, v_t ) >= 0 && Dot( x_t_1 - y_t_1, v_t ) >= 0
// First edge is entirely in back of second edge:
// Dot( x_t_0 - y_t_0, v_t ) <= 0 && Dot( x_t_1 - y_t_0, v_t ) <= 0
//
//
// Substituting x_t_1 = x_t_0 + u_t and y_t_1 = y_t_0 + v_t:
//
// Second edge is entirely in front of first edge:
// Dot( y_t_0 - x_t_0 - u_t, u_t ) >= 0 && Dot( y_t_0 + v_t - x_t_0 - u_t, u_t ) >= 0
// Second edge is entirely in back of first edge:
// Dot( y_t_0 - x_t_0      , u_t ) <= 0 && Dot( y_t_0 + v_t - x_t_0      , u_t ) <= 0
//
// First edge is entirely in front of second edge:
// Dot( x_t_0 - y_t_0 - v_t, v_t ) >= 0 && Dot( x_t_0 + u_t - y_t_0 - v_t, v_t ) >= 0
// First edge is entirely in back of second edge:
// Dot( x_t_0 - y_t_0      , v_t ) <= 0 && Dot( x_t_0 + u_t - y_t_0      , v_t ) <= 0
//
//
// Substituting y_t_0 - x_t_0  = w_t:
//
// Second edge is entirely in front of first edge:
// Dot(  w_t - u_t, u_t ) >= 0 && Dot(  w_t + v_t - u_t, u_t ) >= 0
// Second edge is entirely in back of first edge:
// Dot(  w_t      , u_t ) <= 0 && Dot(  w_t + v_t      , u_t ) <= 0
//
// First edge is entirely in front of second edge:
// Dot( -w_t - v_t, v_t ) >= 0 && Dot(  w_t + u_t - v_t, v_t ) >= 0
// First edge is entirely in back of second edge:
// Dot( -w_t      , v_t ) <= 0 && Dot( -w_t + u_t      , v_t ) <= 0
//
//
// Substituting dots;
//
// Second edge is entirely in front of first edge:
// (wu - uu >= 0) && (wu + vu - uu) >= 0
// First edge is entirely in back of second edge:
// (wu <= 0) && (wu + vu <= 0)
//
// First edge is entirely in front of second edge:
// (-wv - vv >= 0) && (wv + uv - vv >= 0)
// First edge is entirely in back of second edge:
// (-wv <= 0) && (-wv + uv <= 0)

            const Real uu = dot( u_t, u_t );
            const Real uv = dot( u_t, v_t );
            const Real uw = dot( u_t, w_t );

            const Real vv = dot( v_t, v_t );
            const Real vw = dot( v_t, w_t );

            if( (uw - uu >= 0) && (uw + uv - uu) >= 0 )
            {
//                    print("Edge " + ToString(j) + " is entirely in front of edge " + ToString(i) + ".");
                continue;
            }
            else if ( (uw <= 0) && (uw + uv <= 0) )
            {
//                    print("Edge " + ToString(j) + " is entirely in back of edge " + ToString(i) + ".");
                continue;
            }
            else if ( (- vw - vv >= 0) && (vw + uv - vv >=0) )
            {
//                    print("Edge " + ToString(i) + " is entirely in front of edge " + ToString(j) + ".");
                continue;
            }
            else if ( (-vw <= 0) && (-vw + uv <= 0) )
            {
//                    print("Edge " + ToString(i) + " is entirely in back of edge " + ToString(j) + ".");
                continue;
            }
            {
//                    print("Edge { " + ToString(i) + ", " + ToString(j) + " } passed front/back test.");
            }
            
            // TODO: A good idea might be to check whether the planar 4-gon {0,0,0}, w_t, w_t + v_t, -u_t is convex: https://physics.stackexchange.com/a/373609/230503
            
            // Compute Gram matrix of the two edge vectors
            
//            const Tiny::Matrix<2,2,Real,Int> A { { uu, -uv }, { uv, -vv } };
//            
//            const Tiny::Vector<2,Real,Int> b { uw, vw };
            
            // z = A^{-1} b via Cramer's rule.
            
//            const Real det = uv * uv - uu * vv;
            
            const Real determinant = Det2D_Kahan( uv, uu, vv, uv );
            
            if( Abs(determinant) == 0 )
            {
                wprint(ClassName()+"::MovingEdgeCollisions: Gram matrix of edge vectors of edge pair { " + ToString(i) + ", " + ToString(j) + " } is singular");
            }
            
            const Real det_inv = Inv( determinant );
            
//            const Vector2_T z {
//                det_inv * ( uv * vw - vv * uw ),
//                det_inv * ( uu * vw - uv * uw )
//            };
            
            const Vector2_T z {
                det_inv * Det2D_Kahan( uv, uw, vv, vw),
                det_inv * Det2D_Kahan( uu, uv, uw, vw)
            };
            
            
            
            // Finally, an intersection only occurs if 0 <= z[0] <= 1 and 0 <= z[1] <= 1.
            
            if constexpr ( i_0 >= 0 && j_0 >= 0 )
            {
                if( i == i_0 && j == j_0 )
                {
                    const Tiny::Matrix<2,2,Real,Int> A { { uu, -uv }, { uv, -vv } };
                    
                    const Tiny::Vector<2,Real,Int> b { uw, vw };
                    
                    print("MovingEdgeCollisions");
                    dump(t);
                    dump(i);
                    dump(j);
                    dump(A);
                    dump(b);
                    dump(z);
                }
            }
            
            // TODO: If z[0] or z[1] is too close to 0 or 1, we should rather do some joint check together with the according neighbor edges...
            
            if( (zero <= z[0] ) && (z[0] < one) && (zero <= z[1]) && (z[1] < one)  )
            {
                // x_t = (one - t) * x_0 + t * x_1;
                
//                 inter = x_t + z[0] * u_t;
                
                cptr<Real> x_0 = &E_0_i[0][0];
//                cptr<Real> x_1 = &E_1_i[0][0];
                
                // delta_x = x_1 - x_0
                const Vector3_T delta_x {
                    E_1_i[0][0] - x_0[0],
                    E_1_i[0][1] - x_0[1],
                    E_1_i[0][2] - x_0[2]
                };
                
                const Vector3_T inter {
                    x_0[0] + t * delta_x[0] + z[0] * u_t[0],
                    x_0[1] + t * delta_x[1] + z[0] * u_t[1],
                    x_0[2] + t * delta_x[2] + z[0] * u_t[2],
                };
                
                // The sign of the collision is the sign of the determinant
                // of the edge vectors u_t, v_t and the velocity vector velo
                // of the collision point inter.
                
                const Vector3_T velo {
                    delta_x[0] + z[0] * u_1[0],
                    delta_x[1] + z[0] * u_1[1],
                    delta_x[2] + z[0] * u_1[2],
                };
                
                
//                cptr<Real> x_0 = &E_0_i[0][0];
//                cptr<Real> x_1 = &E_1_i[0][0];
//                
//                const Vector3_T inter {
//                    (one - t) * x_0[0] + t * x_1[0] + z[0] * u_t[0],
//                    (one - t) * x_0[1] + t * x_1[1] + z[0] * u_t[1],
//                    (one - t) * x_0[2] + t * x_1[2] + z[0] * u_t[2],
//                };
//                
//                // The sign of the collision is the sign of the determinant
//                // of the edge vectors u_t, v_t and the velocity vector velo
//                // of the collision point inter.
//                
//                const Vector3_T velo {
//                    x_1[0] - x_0[0] + z[0] * u_1[0],
//                    x_1[1] - x_0[1] + z[0] * u_1[1],
//                    x_1[2] - x_0[2] + z[0] * u_1[2],
//                };

                Int sign = - Sign( det( u_t, v_t, velo ) );
                
                if( sign == Int(0) )
                {
                    eprint( ClassName()+"::MovingEdgeCollisions: Collision between edges " + ToString(i) + " and " + ToString(j) + " is degenerate." );
                }
                
                collisions.emplace_back( t, std::move(inter), std::move(z), i, j, sign );
                
                if constexpr ( i_0 >= 0 && j_0 >= 0 )
                {
                    if( i == i_0 && j == j_0 )
                    {
                        print("Collision found");
                        dump(t);
                        dump(sign);
                        dump(collisions.size());
                    }
                }
            }
            
        } // for( Int k = 0; k < count; ++k )
        
    } // MovingEdgeCollisions


//    /// TODO: Add further checks:
//    /// - Denote the current link configuration by L.
//    /// - Suppose that i and j lie in the same link component of L.
//    /// - Denote with L_0 the smaller (by edge count) component generated by splicing the link.
//    /// - Denote the rest by L_c.
//    /// - Let S be any convenient surface spanned into L_0, e.g.
//    ///     - computed from Poisson kernel on meshed disk
//    ///     - computed from heat kernel on meshed disk
//    ///     - a level set of a harmonic function
//    ///         --> ask Keenan about the fast rendering of a harmonic level set spanned into a curve!
//    ///
//    /// - Check whether L_0 is "unlinked" from L_c, e.g., checking intersections of L_c with S.
//    ///   If "unlinked", then L = L_0 # L_c. So it might be a good idea to STOP the simulation here, make
//    ///   a #-node (flag!) in the expression tree and continue with L_0 and L_c in separate simulations.
//    ///
//    /// - If "unlinked" and if S is embedded, then we have a connected sum with a circle,
//    ///   thus we can also just ignore this collision and continue the flow as is.
//    ///     PRO:    This is easier and does not discard already computed collisions.
//    ///     CONTRA: It might lead to further nonessential intersections.
//    ///
//    /// - Even if L_0 is not "unlinked" but S is embedded but, then there may be a shortcut:
//    ///   Typically, we will have only few crossings of L_c with S.
//    ///   At least for just one crossing we should be able to immediately say something.
//    ///
//    /// - If L_0 has less than 6 edges, then it is certainly an unknot (stick number!). So if "unlinked",
//    ///   then we better just ignore the collision; these up to six edges won't cause much trouble later.
//    ///
//    /// - If L_0 has fewer then, say 8, vertices, then a "good" spanning surface would probably
//    ///   be the triangulation obtained by connecting each edge with the barycenter of the vertices.
//    ///     --> check few triangles for intersection with the rest.
//
//                        // Maybe a tiny loop nozzles off.
//                        // If we find a small spanning disk that does not intersect with the remainder of the link, then we can just ignore this crossing.
////                        essentialQ = essentialQ && CheckTinyDisk( i, j, c_i, inter );



    Tensor2<Int,Int> ExportCollisionPairs() const
    {
        const Int n = static_cast<Int>(collisions.size());

        Tensor2<Int,Int> pairs ( n, static_cast<Int>(2) );

        for( Int k = 0; k < n; ++k )
        {
            pairs(k,0) = collisions[k].i;
            pairs(k,1) = collisions[k].j;
        }
        return pairs;
        
    } // ExportCollisionPairs
