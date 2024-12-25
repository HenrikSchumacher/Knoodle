#pragma once

namespace KnotTools
{
    enum class LineSegmentsIntersectionFlag : Int32
    {
        Empty              = 0, // Empty intersection.
        Point_Transversal  = 1, // Exactly one intersec. point X and X != x_1 and X != y_1.
        Point_AtCorner0    = 2, // Exactly one intersec. point X and X == x_0 and X != y_0.
        Point_AtCorner1    = 3, // Exactly one intersec. point X and X != x_0 and X == y_0.
        Point_CornerCorner = 4, // Exactly one intersec. point X == x_0 == y_0.
        Interval           = 5   // The intersection between the two lines consists of more than two points.
    };
    
    bool IntersectingQ( LineSegmentsIntersectionFlag f )
    {
        return ToUnderlying(f) >= 1;
    }
    
    bool DegenerateQ( LineSegmentsIntersectionFlag f )
    {
        return ToUnderlying(f) >= 2;
    }
    
    template<typename Real_,typename Int_,typename SInt_>
    class PlanarLineSegmentIntersector
    {
    public:
        
        using Real      = Real_;
        using Int       = Int_;
        using SInt      = SInt_;
        
        using F_T       = LineSegmentsIntersectionFlag;
        
        using Vector2_T = Tiny::Vector<2,Real,Int>;
        
        
        PlanarLineSegmentIntersector() = default;
        
        ~PlanarLineSegmentIntersector()
        {
            Size_T k;
            
            k = intersection_counts[2] + intersection_counts[3];
            
            if( k > 0 )
            {
                wprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(k) + " edge-corner intersections."
                );
            }
            
            if( intersection_counts[4] > 0 )
            {
                wprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[4]) + " corner-corner intersections."
                );
            }
            
            if( intersection_counts[5] > 0 )
            {
                eprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[5]) + " interval edge-edge intersections."
                );
            }
            
            if( intersection_counts[7] > 0 )
            {
                eprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[7]) + " invalid 3D intersections."
                );
            }
        }
        
        
    protected:
        
//        std::pair<Real,Real> uv_det;
//        std::pair<Real,Real> du_det;
//        std::pair<Real,Real> dv_det;
        
        Vector2_T u;
        Vector2_T v;
        Vector2_T p;
        Vector2_T q;
        Vector2_T d;
        Vector2_T e;
        
        SInt signs [4];
        
        F_T flag;
        
        std::array<Size_T,8> intersection_counts = {};
        
        Vector2_T times_0;
        Vector2_T times_1;
        
        double line_intersection_time = 0;

        
    public:
        
        
        /**
         * @brief Classify whether and how two planar, oriented line segments in the place intersect.
         *
         * @param x_0 Start point of the first line segment.
         *
         * @param x_1 end point of the first line segment.
         *
         * @param y_0 Start point of the second line segment.
         *
         * @param y_1 end point of the second line segment.
         *
         * @return `LineSegmentsIntersectionFlag f`, specified by the following:
         *
         *  `f = LineSegmentsIntersectionFlag::Empty` if and only if the line segments do not intersect at all _or if there is a unique intersection point and if it falls together with `x_1` or `y_1`_.
         *
         *  `f = LineSegmentsIntersectionFlag::Point_Transversal` if and only if  the line segments have exactly one point in common _and if this point does not falls together with `x_1` or `y_1`_.
         *
         * `f = LineSegmentsIntersectionFlag::Point_AtCorner0` if and only if the line segments have only the point `x_0` in common _and if `x_0 != y_0`_.
         *
         * `f = LineSegmentsIntersectionFlag::Point_AtCorner1` if and only if the line segments have only the point `y_0` in common _and if `x_0 != y_0`_.
         *
         * `f = LineSegmentsIntersectionFlag::Point_CornerCorner` if and only if the line segments have only a single pointin common and it is `x_0 = y_0`.
         *
         * `f = LineSegmentsIntersectionFlag::Interval` if the line segments have two or more points in common.
         *
         * The reason for this slightly odd definition is that we want make this work on links that are given by orthogonal link diagrams. There, all crossings look like this:
         *
         *          ^
         *          | d
         *    a     |     b
         *  ------->X------->
         *          ^
         *          | c
         *          |
         *
         * That is, four line segments have a point in common. Here we assume that b is the successor of a and that d is the successor of c. Since intesection detection between direct neighbors in the link should be avoided, only the following pairs can ever be checked (a,c), (b,c), (a,d), (b,d) will be tested. We made it so that the only successfully detected intersections is (b,d); the flag `LineSegmentsIntersectionFlag::Point_CornerCorner` will be issued in this case.
         *
         *
         * Note that the same picture would be obtained if we projected a 3D link to the plane in which b is the successor of c and d is the successor of a.
         *
         * We cannot distinguish these cases here, because this function sees only a single pair of edges. This becomes a bit clearer by the following picture:
         *
         *          ^
         *          | d
         *    a     |
         *  ------->X
         *         ^ \
         *      c /   \ b
         *       /     v
         *
         * If b is the successor of c and if d is the successor of a, then this here would require either no crossings or two crossings with opposite handedness, because by moving c, b slightly to the bottom right would resolve this by a Reidemeister II move. However, we would detect a single crossing here, namely a `LineSegmentsIntersectionFlag::Point_CornerCorner` crossing formed by directed edges b and d since their tails coincide.
         *
         * We also assume that each of the line segments `x_0 x_1` and `y_0 y_1` has nonzero length.
         */
        
        LineSegmentsIntersectionFlag IntersectionType(
            cptr<Real> x_0, cptr<Real> x_1, cptr<Real> y_0, cptr<Real> y_1
        )
        {
            flag = intersectionType(x_0,x_1,y_0,y_1);
            
            ++intersection_counts[ ToUnderlying(flag) ];
            
            return flag;
        }
        
    protected:
            
        template<bool timingQ = true>
        LineSegmentsIntersectionFlag intersectionType(
            cptr<Real> x_0, cptr<Real> x_1, cptr<Real> y_0, cptr<Real> y_1
        )
        {
            // CAUTION: We do not check whether any of the two lines has length 0!
            
            // TODO: What to do if u = 0 or v = 0?
            
            //  x_1     e     y_1
            //      X------>X
            //      ^^     ^^
            //      | \q p/ |
            //      |  \ /  |
            //    u |   /   | v
            //      |  / \  |
            //      | /   \ |
            //      |/     \|
            //      X------>X
            //  x_0     d     y_0

            u[0] = x_1[0] - x_0[0];
            u[1] = x_1[1] - x_0[1];
            
            v[0] = y_1[0] - y_0[0];
            v[1] = y_1[1] - y_0[1];
            
            p[0] = y_1[0] - x_0[0];
            p[1] = y_1[1] - x_0[1];
            
            q[0] = x_1[0] - y_0[0];
            q[1] = x_1[1] - y_0[1];
            
            d[0] = y_0[0] - x_0[0];
            d[1] = y_0[1] - x_0[1];
            
            e[0] = y_1[0] - x_1[0];
            e[1] = y_1[1] - x_1[1];
            
    //        signs[0] = Oriented2D_Kahan<SInt>( x_0, y_0, y_1 );
    //        signs[1] = Oriented2D_Kahan<SInt>( x_1, y_0, y_1 );
    //        signs[2] = Oriented2D_Kahan<SInt>( x_0, x_1, y_0 );
    //        signs[3] = Oriented2D_Kahan<SInt>( x_0, x_1, y_1 );

            
            signs[0] = DetSign_Kahan<SInt>(p,v);
            signs[1] = DetSign_Kahan<SInt>(v,q);
            signs[2] = DetSign_Kahan<SInt>(q,u);
            signs[3] = DetSign_Kahan<SInt>(u,p);
            
            
            const SInt s01 = signs[0]*signs[1];
            const SInt s23 = signs[2]*signs[3];
            
            if( (s01 == 1) || (s23 == 1) )
            {
                // x_0 and x_1 lie on the same side of line y_0 y_1. No intersection is possible.
                flag = F_T::Empty;
                
                return flag;
            }
            
            if( (s01 != 0) && (s23 != 0) )
            {
                // The generic case.
                
                flag = ( signs[1] == signs[2] ) && ( signs[0] == signs[3] )
                    ? F_T::Point_Transversal : F_T::Empty;
                
                return flag;
            }
            
            // If we arrive here then at least one of the points x_0, x_1, y_0, y_1 lies on both of the two segments.
            
            // We are only interested in cases where signs[0] == 0 or signs[2] == 0, which correspond to x_0 lying on Line(y_0,y_1) and y_0 lying on Line(x_0,x_1), respectively.
            
            // That is the corner cases (pun intended) and the interval case.
            
            if( signs[0] == 0 )
            {
                //  x_1     e     y_1
                //      X------>X
                //      ^^     ^^
                //      | \q p/ |
                //      |  \ /  |
                //    u |   /   | v
                //      |  / \  |
                //      | /   \ |
                //      |/     \|
                //      X------>X
                //  x_0     d     y_0
                
                const SInt dot_vd_sign = DotSign_Kahan<SInt>(v,d);
                const SInt dot_vp_sign = DotSign_Kahan<SInt>(v,p);

                const bool x_0_aft_y_0_Q = dot_vd_sign <= 0;
                const bool x_0_bef_y_1_Q = dot_vp_sign >  0;

                
                if( signs[2] == 0 )
                {
                    if( (signs[1] != 0) || (signs[3] != 0) )
                    {
                        flag =  F_T::Point_CornerCorner;
                        
                        return flag;
                    }
                    
                    // Both x_0, x_1, y_0, and y_1 all lie on the same line.
                    
                    const SInt dot_vq_sign = DotSign_Kahan<SInt>(v,q);
                    const SInt dot_ve_sign = DotSign_Kahan<SInt>(v,e);
                               
                    // CAUTION: Here we assume that v is a nonzero vector.
                    
                    if( (dot_vq_sign == 0) || (dot_vp_sign == 0) )
                    {
                        flag = F_T::Empty;
                        
                        return flag;
                    }
                    
                    const bool x_1_aft_y_0_Q = dot_vq_sign >= 0;
                    const bool x_1_bef_y_1_Q = dot_ve_sign >  0;
                    
                    // The only possible way without intersection is if either both of x_0 and x_1 lie stricly before y_0 or both lie after y_1.
                    flag = (!x_0_aft_y_0_Q && !x_1_aft_y_0_Q) || (!x_0_bef_y_1_Q && !x_1_bef_y_1_Q)
                        ? F_T::Empty : F_T::Interval;
                    
                    if( flag == F_T::Interval )
                    {
                        // TODO: Compute intersection time intervals.
                    }
                    
                    return flag;
                }
                else
                {
                    // signs[2] != 0, Overlap impossible
                    // We merely have to check where on Line(y_0,y_1) the point x_0 lies.
                    
                    flag = (x_0_aft_y_0_Q && x_0_bef_y_1_Q)
                        ? F_T::Point_AtCorner0 : F_T::Empty;
                    
                    return flag;
                }
            }
            else
            {
                if( signs[2] == 0 )
                {
                    //  x_1     e     y_1
                    //      X------>X
                    //      ^^     ^^
                    //      | \q p/ |
                    //      |  \ /  |
                    //    u |   /   | v
                    //      |  / \  |
                    //      | /   \ |
                    //      |/     \|
                    //      X------>X
                    //  x_0     d     y_0
                    
                    // signs[0] != 0, Overlap impossible.
                    
                    // We merely have to check where on Line(x_0,x_1) the point y_0 lies.
                    
                    const SInt dot_ud_sign = DotSign_Kahan<SInt>(u,d);
                    const SInt dot_uq_sign = DotSign_Kahan<SInt>(u,q);
                    
                    const bool y_0_aft_x_0_Q = dot_ud_sign >= 0;
                    const bool y_0_bef_x_1_Q = dot_uq_sign >  0;
                    
                    flag = (y_0_aft_x_0_Q && y_0_bef_x_1_Q)
                        ? F_T::Point_AtCorner1 : F_T::Empty;
                    
                    return flag;
                }
                else
                {
                    // signs[2] != 0, Overlap impossible
                    
                    // We may ignore this case as only x_1 and y_1 remain as potential intersection points and because we ignore them intentionally.
                    flag = F_T::Empty;
                    
                    return flag;
                }
            }
        }
        
    public:
        
        std::pair<Tiny::Vector<2,Real,Int>,SInt> IntersectionTimesAndSign()
        {
            Tiny::Vector<2,Real,Int> t;
            
            auto [det_plus,det_minus] = Det_Kahan_DiffPair(u,v);
            
            const SInt sign = DifferenceSign<SInt>(det_plus,det_minus);
            
            if( sign == 0 )
            {
                // TODO: Find a pair of actual intersection times.
                
                t[0] = 0.5;
                t[1] = 0.5;
                
                return {t, sign};
            }
            
            const Real det_inv = Inv<Real>(det_plus - det_minus);

            t[0] = Det_Kahan(d,v) * det_inv;
            t[1] = Det_Kahan(d,u) * det_inv;
            
            return {t, sign};
        }
        
    public:
        
        cref<std::array<Size_T,8>> IntersectionCounts()
        {
            return intersection_counts;
        }
        
        static std::string ClassName()
        {
            return std::string("PlanarLineSegmentIntersector")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // class PlanarLineSegmentIntersector
    
} // namespace KnotTools
