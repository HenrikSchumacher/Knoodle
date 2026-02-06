#pragma once

namespace Knoodle
{
    enum class LineSegmentsIntersectionFlag : int
    {
        Empty        = 0, // Empty intersection.
        Transversal  = 1, // Exactly one intersec. point X and X != x_1 and X != y_1.
        AtCorner0    = 2, // Exactly one intersec. point X and X == x_0 and X != y_0.
        AtCorner1    = 3, // Exactly one intersec. point X and X != x_0 and X == y_0.
        CornerCorner = 4, // Exactly one intersec. point X == x_0 == y_0.
        Interval     = 5,   // The intersection between the two lines consists of more than two points.
        Spatial      = 6,   // The intersection between the two lines is tranversal in 2D, but also exists in 3D.
        OOBounds     = 7,   // The intersection times are out of bounds, indicating that rounding errors occurred.
    };
    
    inline bool IntersectingQ( LineSegmentsIntersectionFlag f )
    {
        return ToUnderlying(f) >= Underlying_T<LineSegmentsIntersectionFlag>(1);
    }
    
    inline bool DegenerateQ( LineSegmentsIntersectionFlag f )
    {
        return ToUnderlying(f) >= Underlying_T<LineSegmentsIntersectionFlag>(2);
    }
    
    template<typename Real_,typename Int_>
    class PlanarLineSegmentIntersector final
    {
    public:
        
        using Real      = Real_;
        using Int       = Int_;
        
        using Sign_T    = typename Intersection<Real,Int>::Sign_T; // Solely for signs.
        
        using F_T       = LineSegmentsIntersectionFlag;
        
        using Vector2_T = Tiny::Vector<2,Real,Int>;
        
        // Default constructor
        PlanarLineSegmentIntersector() = default;
        // Destructor (virtual because of inheritance)
        ~PlanarLineSegmentIntersector()
        {
//            Size_T k = intersection_counts[2] + intersection_counts[3];
//
//
//            if( k > 0 )
//            {
//                wprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(k) + " edge-corner intersections."
//                );
//            }
//
//            if( intersection_counts[4] > 0 )
//            {
//                wprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[4]) + " corner-corner intersections."
//                );
//            }
//
//            if( intersection_counts[5] > 0 )
//            {
//                eprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[5]) + " interval edge-edge intersections."
//                );
//            }
//
//            if( intersection_counts[7] > 0 )
//            {
//                eprint(ClassName()+"::FindIntersectingEdges_DFS found " + ToString(intersection_counts[7]) + " invalid 3D intersections."
//                );
//            }
        }
        // Copy constructor
        PlanarLineSegmentIntersector( const PlanarLineSegmentIntersector & other ) = default;
        // Copy assignment operator
        PlanarLineSegmentIntersector & operator=( const PlanarLineSegmentIntersector & other ) = default;
        // Move constructor
        PlanarLineSegmentIntersector( PlanarLineSegmentIntersector && other ) = default;
        // Move assignment operator
        PlanarLineSegmentIntersector & operator=( PlanarLineSegmentIntersector && other ) = default;
        
    protected:
        
        Vector2_T u;
        Vector2_T v;
        Vector2_T p;
        Vector2_T q;
        Vector2_T d;
        Vector2_T e;
        
        Sign_T pxv;
        Sign_T vxq;
        Sign_T qxu;
        Sign_T uxp;
        Sign_T pxv_vxq;
        Sign_T qxu_uxp;
        
//        IntersectionFlagCounts_T intersection_counts = {};
        
        F_T flag;
        
        
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
         *  `f = LineSegmentsIntersectionFlag::Transversal` if and only if  the line segments have exactly one point in common _and if this point does not falls together with `x_1` or `y_1`_.
         *
         * `f = LineSegmentsIntersectionFlag::AtCorner0` if and only if the line segments have only the point `x_0` in common _and if `x_0 != y_0`_.
         *
         * `f = LineSegmentsIntersectionFlag::AtCorner1` if and only if the line segments have only the point `y_0` in common _and if `x_0 != y_0`_.
         *
         * `f = LineSegmentsIntersectionFlag::CornerCorner` if and only if the line segments have only a single pointin common and it is `x_0 = y_0`.
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
         * That is, four line segments have a point in common. Here we assume that b is the successor of a and that d is the successor of c. Since intesection detection between direct neighbors in the link should be avoided, only the following pairs can ever be checked (a,c), (b,c), (a,d), (b,d) will be tested. We made it so that the only successfully detected intersections is (b,d); the flag `LineSegmentsIntersectionFlag::CornerCorner` will be issued in this case.
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
         * If b is the successor of c and if d is the successor of a, then this here would require either no crossings or two crossings with opposite handedness, because by moving c, b slightly to the bottom right would resolve this by a Reidemeister II move. However, we would detect a single crossing here, namely a `LineSegmentsIntersectionFlag::CornerCorner` crossing formed by directed edges b and d since their tails coincide.
         *
         * We also assume that each of the line segments `x_0 x_1` and `y_0 y_1` has nonzero length.
         */
        
    public:
            
        template<bool verboseQ = false>
        LineSegmentsIntersectionFlag IntersectionType(
            cptr<Real> x_0, cptr<Real> x_1, cptr<Real> y_0, cptr<Real> y_1
        )
        {
            if constexpr ( verboseQ )
            {
                wprint(ClassName()+"::IntersectionType in verbose mode.");
            }
            
            // Caution:
            // We should have checked that x_0 != x_1 and y_0 != y_1 before we arrive here
            
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
            
            pxv = DetSign_Kahan<Sign_T>(p,v);
            vxq = DetSign_Kahan<Sign_T>(v,q);
            qxu = DetSign_Kahan<Sign_T>(q,u);
            uxp = DetSign_Kahan<Sign_T>(u,p);
            
            // pxv == 0    <=>     (y_0 == y_1) or (x_0 in Line(y_0,y_1))
            // vxq == 0    <=>     (y_0 == y_1) or (x_1 in Line(y_0,y_1))
            // qxu == 0    <=>     (x_0 == x_1) or (y_0 in Line(x_0,x_1))
            // uxp == 0    <=>     (x_0 == x_1) or (y_1 in Line(x_0,x_1))
            
            pxv_vxq = pxv * vxq;
            qxu_uxp = qxu * uxp;
            
            if( pxv_vxq == 1 )
            {
                // v != 0 => y_0 != y_1 => Line(y_0,y_1) is nondegenerate.
                //
                // => x_0 and x_1 lie on the same side of Line(y_0,y_1).
                //
                // So no intersection possible.
                
                flag = F_T::Empty;
                
                if constexpr ( verboseQ )
                {
                    print("A");
                }
                
                return flag;
            }
                
            if( qxu_uxp == 1 )
            {
                // u != 0 => x_0 != x_1 => Line(x_0,x_1) is nondegenerate.
                //
                // => y_0 and y_1 lie on the same side of Line(x_0,x_1).
                //
                // So no intersection possible.
                
                flag = F_T::Empty;
                
                if constexpr ( verboseQ )
                {
                    print("B");
                }
                
                return flag;
            }
            
            if( (pxv_vxq != 0) && (qxu_uxp != 0) )
            {
                // The generic case.
                
                flag = ( vxq == qxu ) && ( pxv == uxp )
                    ? F_T::Transversal
                    : F_T::Empty;
                
                if constexpr ( verboseQ )
                {
                    print("C");
                }
                
                return flag;
            }
            
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

            
            // If we arrive here then at least one of the points x_0, x_1, y_0, y_1 lies on both of the two segments.
            
            // We purposefully ignore the cases vxq == 0 or uxp == 0,
            // which correspond to x_1 in Line(y_0,y_1) or y_1 in Line(x_0,x_1).
            // They will be handled by the edged following these ones.
            // So we are only interested in the cases where pxv == 0 or qxu == 0, which correspond to x_0 lying on Line(y_0,y_1) and y_0 lying on Line(x_0,x_1), respectively.
            
            // That is the corner cases (pun intended) and the interval case.
            
            if( pxv == 0 ) // <=> x_0 in Line(y_0,y_1)
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
                
                const Sign_T dot_vd_sign = DotSign_Kahan<Sign_T>(v,d);
                const Sign_T dot_vp_sign = DotSign_Kahan<Sign_T>(v,p);

                const bool x_0_aft_y_0_Q = dot_vd_sign <= 0;
                const bool x_0_bef_y_1_Q = dot_vp_sign >  0;

                
                if( qxu == 0 ) // <=> y_0 in Line(x_0,x_1)
                {
                    if( (vxq != 0) || (uxp != 0) )
                    {
                        flag =  F_T::CornerCorner;
                        
                        if constexpr ( verboseQ )
                        {
                            print("D");
                        }
                        
                        return flag;
                    }
                    
                    // Both x_0, x_1, y_0, and y_1 all lie on the same line.
                    
                    const Sign_T dot_vq_sign = DotSign_Kahan<Sign_T>(v,q);
                    const Sign_T dot_ve_sign = DotSign_Kahan<Sign_T>(v,e);
                               
                    // CAUTION: Here we assume that v is a nonzero vector.
                    
                    if( (dot_vq_sign == Sign_T(0)) || (dot_vp_sign == Sign_T(0)) )
                    {
                        flag = F_T::Empty;
                        
                        if constexpr ( verboseQ )
                        {
                            print("E");
                        }
                        
                        return flag;
                    }
                    
                    const bool x_1_aft_y_0_Q = dot_vq_sign >= Sign_T(0);
                    const bool x_1_bef_y_1_Q = dot_ve_sign >  Sign_T(0);
                    
                    // The only possible way without intersection is if either both of x_0 and x_1 lie stricly before y_0 or both lie after y_1.
                    flag = (!x_0_aft_y_0_Q && !x_1_aft_y_0_Q) || (!x_0_bef_y_1_Q && !x_1_bef_y_1_Q)
                        ? F_T::Empty : F_T::Interval;
                    
                    if( flag == F_T::Interval )
                    {
                        // TODO: Compute intersection time intervals.
                    }
                    
                    if constexpr ( verboseQ )
                    {
                        print("F");
                    }
                    
                    return flag;
                }
                else
                {
                    // qxu != 0, Overlap impossible
                    // We merely have to check where on Line(y_0,y_1) the point x_0 lies.
                    
                    flag = (x_0_aft_y_0_Q && x_0_bef_y_1_Q)
                        ? F_T::AtCorner0 : F_T::Empty;
                    
                    if constexpr ( verboseQ )
                    {
                        print("G");
                    }
                    
                    return flag;
                }
            }
            else // pxv != 0
            {
                if( qxu == 0 ) // <=> y_0 in Line(x_0,x_1)
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
                    
                    // pxv != 0, Overlap impossible.
                    
                    // We merely have to check where on Line(x_0,x_1) the point y_0 lies.
                    
                    const Sign_T dot_ud_sign = DotSign_Kahan<Sign_T>(u,d);
                    const Sign_T dot_uq_sign = DotSign_Kahan<Sign_T>(u,q);
                    
                    const bool y_0_aft_x_0_Q = dot_ud_sign >= Sign_T(0);
                    const bool y_0_bef_x_1_Q = dot_uq_sign >  Sign_T(0);
                    
                    flag = (y_0_aft_x_0_Q && y_0_bef_x_1_Q)
                        ? F_T::AtCorner1 : F_T::Empty;
                    
                    if constexpr ( verboseQ )
                    {
                        print("H");
                    }
                    
                    return flag;
                }
                else
                {
                    // (pxv != 0) and (qxu != 0), Overlap impossible
                    
                    // We may ignore this case as only x_1 and y_1 remain as potential intersection points and because we ignore them intentionally.
                    flag = F_T::Empty;
                    
                    if constexpr ( verboseQ )
                    {
                        print("I");
                    }
                    
                    return flag;
                }
            }
            
            if constexpr ( verboseQ )
            {
                print("Z");
            }
        }
        
    public:
        
        std::pair<Tiny::Vector<2,Real,Int>,Sign_T> IntersectionTimesAndSign()
        {
            Tiny::Vector<2,Real,Int> t;
            
            auto [det_plus,det_minus] = Det_Kahan_DiffPair(u,v);
            
            const Sign_T sign = DifferenceSign<Sign_T>(det_plus,det_minus);
            
            if( sign == 0 )
            {
                // TODO: Find a pair of actual intersection times.
                
                t[0] = Scalar::Half<Real>;
                t[1] = Scalar::Half<Real>;
                
                return {t, sign};
            }
            
            const Real det_inv = Inv<Real>(det_plus - det_minus);

            t[0] = Det_Kahan(d,v) * det_inv;
            t[1] = Det_Kahan(d,u) * det_inv;
            
            return {t, sign};
        }
        
    public:
        
//        cref<IntersectionFlagCounts_T> IntersectionCounts()
//        {
//            return intersection_counts;
//        }
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("PlanarLineSegmentIntersector")
                + "<" + TypeName<Real>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // class PlanarLineSegmentIntersector
    
} // namespace Knoodle
