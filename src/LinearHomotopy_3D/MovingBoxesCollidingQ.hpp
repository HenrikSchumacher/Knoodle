public:

    bool MovingBoxesCollidingQ(
        cptr<Real> P_0, cptr<Real> P_1, cptr<Real> Q_0, cptr<Real> Q_1
    )
    {
        // Let P_0, P_1 be the bounding boxes of node i at time t=0 and t=1, repectively.
        // Let Q_0, Q_1 be the bounding boxes of node j at time t=0 and t=1, repectively.
        //
        // This function computes the interval [t_first,t_last] of those t in [0,1] such that the AABBs defined by
        //     P(t) = (1-t) * p_0 + t * p_1
        // and
        //     Q(t) = (1-t) * q_0 + t * q_1
        // intersect.
        // If this time interval is nonempty, then we return true.
        //
        // Otherwise, if no intersection occurs at all, then we return false.
        //
        // Warning to my future self: We do _not_ compute all the intersections in (-infty,infty)
        // because we have no guarantee that the interval endpoints won't flip for t < 0 or t > 1.
        // So in general, the world polygon of an interval need not be convex.
        // Hence, the set of intersection times need not be convex in the general case.
        // We don't want to bother with this here.
        
    //
    //            if( i == j )
    //            {
    //                return true;
    //            }
        
        
        Real t_0 = zero;
        Real t_1 = one;
        
        
        for( Int k = 0; k < 3; ++k )
        {
            // Get the intervals of the four AABBs in the k-th coordinate direction.
            
    //                const Matrix2_T P { {B_0(i,k,0), B_0(i,k,1)}, {B_1(i,k,0), B_1(i,k,1)} };
    //
    //                const Matrix2_T Q { {B_0(j,k,0), B_0(j,k,1)}, {B_1(j,k,0), B_1(j,k,1)} };
            const Real P [2][2] { {P_0[2*k], P_0[2*k+1]}, {P_1[2*k], P_1[2*k+1]} };
            
            const Real Q [2][2] { {Q_0[2*k], Q_0[2*k+1]}, {Q_1[2*k], Q_1[2*k+1]} };

            // Consider the moving intervals
            //
            // P(t) = [ P[0][0] + (P[1][0] - P[0][0]) * t, P[0][1] + (P[1][1] - P[0][1]) * t ]
            // Q(t) = [ Q[0][0] + (Q[1][0] - Q[0][0]) * t, Q[0][1] + (Q[1][1] - Q[0][1]) * t ]
            //
            // If at time t in (0,1) we change from intersecting to nonintersecting or vice versa,
            // then one of the following conditions must be met:
            //
            // The upper boundary of P(t) equals the lower boundary of Q(t)
            // or
            // The upper boundary of Q(t) equals the lower boundary of P(t)
            //
            // <=>
            //
            // P[0][0] + (P[1][0] - P[0][0]) * t == Q[0][1] + (Q[1][1] - Q[0][1]) * t
            // or
            // Q[0][0] + (Q[1][0] - Q[0][0]) * t == P[0][1] + (P[1][1] - P[0][1]) * t
            //
            // <=>
            //
            // ((P[1][0] - P[0][0]) - (Q[1][1] - Q[0][1])) * t == Q[0][1] - P[0][0]
            // or
            // ((Q[1][0] - Q[0][0]) - (P[1][1] - P[0][1])) * t == P[0][1] - Q[0][0]

            bool P_L_Q_0 = P[0][1] < Q[0][0];
            bool P_R_Q_0 = P[0][0] > Q[0][1];
            
            bool P_L_Q_1 = P[1][1] < Q[1][0];
            bool P_R_Q_1 = P[1][0] > Q[1][1];
            
            if( ( P_L_Q_0 && P_L_Q_1 ) || ( P_R_Q_0 && P_R_Q_1 ) )
            {
                return false;
            }
            
            bool initially_intersecting = ! ( P_L_Q_0 || P_R_Q_0 );
            bool finally_intersecting   = ! ( P_L_Q_1 || P_R_Q_1 );
            
            if( initially_intersecting && finally_intersecting )
            {
                continue;
            }
            
            const Real m_0 = ((P[1][0] - P[0][0]) - (Q[1][1] - Q[0][1])) ;
            const Real m_1 = ((Q[1][0] - Q[0][0]) - (P[1][1] - P[0][1]));

            
            const auto [A,B] = MinMax(
                ( m_0 != zero ) ? (Q[0][1] - P[0][0]) / m_0 : infty,
                ( m_1 != zero ) ? (P[0][1] - Q[0][0]) / m_1 : infty
            );

            if( zero <= A && A <= one )
            {
                if( zero <= B && B <= one )
                {
                    if( initially_intersecting )
                    {
                        t_1 = Min(t_1,B);
                    }
                    else if( finally_intersecting )
                    {
                        t_0 = Max(t_0,A);
                    }
                    else
                    {
                        t_0 = Max(t_0,A);
                        t_1 = Min(t_1,B);
                    }
                }
                else
                {
                    if( initially_intersecting )
                    {
                        // [0,A]
                        t_1 = Min(t_1,A);
                    }
                    else if( finally_intersecting )
                    {
                        // [A,1]
                        t_0 = Max(t_0,A);
                    }
                    else
                    {
                        // [A]
                        t_0 = Max(t_0,A);
                        t_1 = Min(t_1,A);
                    }
                }
            }
            else
            {
                if( zero <= B && B <= one )
                {
                    if( initially_intersecting )
                    {
                        // [0,B]
                        t_1 = Min(t_1,B);
                    }
                    else if( finally_intersecting )
                    {
                        // [B,1]
                        t_0 = Max(t_0,B);
                    }
                    else
                    {
                        // [B]
                        t_0 = Max(t_0,B);
                        t_1 = Min(t_1,B);
                    }
                }
                else
                {
                    if( initially_intersecting )
                    {
                        // [0]
                        t_1 = Min(t_1,zero);
                    }
                    else if ( finally_intersecting )
                    {
                        // [1]
                        t_0 = Max(t_0,one);
                    }
                    else
                    {
                        // Should never be reached.
                        return false;
                    }
                }
            }
        } // for( Int k = 0; k < 3; ++k )
        
        return (t_0 <= t_1);
        
    } // MovingBoxesCollidingQ
