#pragma once

#include "Prosector4/Types.hpp"

namespace Knoodle
{
    // https://math.stackexchange.com/a/4498570/447001
    
    // "Geometry and the imagination" pp 14-15, Hilbert, David, 1862-1943, author; Cohn-Vossen, S. (Stephan), 1902-1936, author; Nemenyi, P., translator
    //
    // "Thus three skew straight lines always define a hyperboloid of one sheet, except in the case where they are all parallel to one plane (but not to each other). In this case they determine a new type of second-order surface, called the hyperbolic paraboloid, which does not include any surface of revolution as a special case."
    
    
    /*!@brief **EXPERIMENTAL.** A class for computing intersections of 3D line segments after projecting them to the plane.
     *
     * This class is part of the pipeline to convert closed polygonal curves in 3-space to a planar diagrams. Users of `Knoodle` will typically not use it directly. This documentation is targeted at developers.
     *
     * This class uses integer arithmetic to allow for exact computations. A symbolic perturbation is employed to handle all degeneracies except line segments that intersect already in 3-space; these are beyond repair, of course.
     *
     * Instead of parallel projecting along the vector `{0,0,1}` to the x-y-plane, the projection is done parallel to the x-y-plane along the perturbed vector `{eps,eps * eps * eps,1}`, i.e., a point `{x[0],x[1],x[2]}` is mapped to `{x[0] - eps * x[3], x[1] - eps * eps * eps * x[3]}`.
     * Since `{eps,eps * eps * eps,1}` is cubic in the symbolic parameter `eps`, there are only finitely many values of `eps` for which this projection results into degeneracies. Thus, it suffices to analyze the topoligical information in the limit eps -> 0+ (i.e., limit from the right). This handles the following degenerate cases consistently, as long as the line segments in 3-space are disjoint and have positive length:
     *
     *  - The projection of the two line segments have a line segment in common.
     *
     *  - An end point of a projected line segment lies on the other.
     *
     *  - A line segment is projected to a single point.
     *
     * The usage of the class is as follows: First one loads two line segments by calling `LoadLineSements`. Then one class `IntersectionType` to probe whether an intersection exists or whether something went wrong (see `Flag_T`). If the return value is `Flag_T::Intersection`, then one can call `ComputeIntersection` to get an instance of `struct` `Intersection` that contains the relevant information.
     *
     * @tparam Int_ Signed integral type used for coordinates of points.
     *
     * @tparam Idx_ Integral type used for indices.
     */
    template<SignedIntQ Int_, IntQ Idx_ = Int64, bool verboseQ = false>
    class Prosector4 final
    {
    public:
        
        static_assert(SameQ<Int_,Int32> || SameQ<Int_,Int64>,"");
        
//        static constexpr Size_T bitlength = bitlength_;
//        static_assert(bitlength <= Size_T(64),"");
//        
        
//        using Int    = std::conditional< bitlength <= Size_T(32), Int32, Int64>;
//        using LInt   = std::conditional< bitlength <= Size_T(16), Int32,
//                           std::conditional<bitlength <= Size_T(32), Int64, Int128 >
//                       >;
//        using LLInt  = std::conditional< bitlength <= Size_T(16), Int64,
//                           std::conditional<bitlength <= Size_T(32), Int128, Int256>
//                       >;
        
        /*!@brief Integral type used for coordinates.*/
        using Int    = Int_;
        /*!@brief Longer integral type used for internal computations.*/
        using LInt   = std::conditional_t<SameQ< Int,Int32>,WInt64,WInt128>;
        /*!@brief Even longer integral type used for internal computations.*/
        using LLInt  = std::conditional_t<SameQ<LInt,WInt64>,WInt128,WInt256>;
        
        using Idx    = Idx_;
        using Sign_T = FastInt8; // Solely for signs.
        
        using Prosector_T = Prosector4<Int,Idx,verboseQ>;
//        using Vector3_T   = Tiny::Vector<3,Int ,Idx>;
//        using LVector3_T  = Tiny::Vector<3,LInt,Idx>;
        
        using Vector3_T   = std::array<Int,3>;
        using LVector3_T  = std::array<LInt,3>;
        
        /*!@brief Flag that indicates whether an intersection was found or whether an error occurred.*/
        enum class Flag_T : int
        {
            Uninitialized =  0, /*!< Flag is uninitialized. */
            Empty         =  2, /*!< Empty intersection. */
            Intersection  =  1, /*!< Nontrivial intersection found. */
            Error         = -1  /*!< Lines must intersect in 3D. */
        };
        
        friend std::string ToString( Flag_T f )
        {
            switch (f)
            {
                case Flag_T::Uninitialized  :   return "Uninitialized";
                case Flag_T::Empty          :   return "Empty";
                case Flag_T::Intersection   :   return "Intersection";
                case Flag_T::Error          :   return "Error";
                default                     :   return "Unknown";
            }
        }
        
        
#include "Prosector4/DepressedCubic.hpp"
#include "Prosector4/IntersectionTime.hpp"
#include "Prosector4/IntersectionTime_Double.hpp"
#include "Prosector4/IntersectionTime_Hybrid.hpp"
        
//        using Time_T = IntersectionTime;
//        using Time_T = IntersectionTime_Double;
        using Time_T = IntersectionTime_Hybrid;
        
//        using Time_T = double;

#include "Prosector4/Intersection.hpp"
        
        // Default constructor
        Prosector4() = default;
        // Default destructor
        ~Prosector4() = default;
        
        // Copy constructor
        Prosector4( const Prosector4 & other ) = default;
        // Copy assignment operator
        Prosector4 & operator=( const Prosector4 & other ) = default;
        // Move constructor
        Prosector4( Prosector4 && other ) = default;
        // Move assignment operator
        Prosector4 & operator=( Prosector4 && other ) = default;
        
    protected:

        Idx k_;
        Idx l_;
        
        Vector3_T x_0;
        Vector3_T x_1;
        Vector3_T y_0;
        Vector3_T y_1;
        
        Vector3_T u;
        Vector3_T v;
        Vector3_T p;
        Vector3_T q;
        
//        LInt uxp_2;
//        LInt vxq_2;
        
        DepressedCubic P_0;
        DepressedCubic P_1;
        
//        LVector3_T pxq;
//        LVector3_T uxp;
//        LVector3_T uxq;
//        LVector3_T vxp;
//        LVector3_T vxq;
//        LVector3_T uxv;
        
//        Sign_T sign_uxp;
//        Sign_T sign_uxq;
//        Sign_T sign_vxp;
//        Sign_T sign_vxq;
        
        Flag_T flag { Flag_T::Uninitialized };

    public:
        
        /*!@brief Return the current value of the internal state flag.*/
        Flag_T Flag() const
        {
            return flag;
        }
        
        /*!@brief Load two line segments.
         *
         * @param k Index of the first line segment (in a upstream data structure).
         *
         * @param x0 Start point of the first line segment; assumed to be a 3-vector.
         *
         * @param x1 End point of the first line segment; assumed to be a 3-vector.
         
         * @param l Index of the second line segment (in a upstream data structure).
         *
         * @param y0 Start point of the second line segment; assumed to be a 3-vector.
         *
         * @param y1 End point of the second line segment; assumed to be a 3-vector.
         */
        
        void LoadLineSements(
            const Idx k, cptr<Int> x0, cptr<Int> x1, const Idx l, cptr<Int> y0, cptr<Int> y1
        )
        {
            flag = Flag_T::Uninitialized;

            k_ = k;
            l_ = l;
            
            copy_buffer<3>( x0, &x_0[0] );
            copy_buffer<3>( x1, &x_1[0] );
            copy_buffer<3>( y0, &y_0[0] );
            copy_buffer<3>( y1, &y_1[0] );
            
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
        }
        
        // Somewhat pointless.
        void LoadLineSements(
            const Idx i_, cref<Vector3_T> x0, cref<Vector3_T> x1,
            const Idx j_, cref<Vector3_T> y0, cref<Vector3_T> y1
        )
        {
            LoadLineSements( i_, x0.data(), x1.data(), j_, y0.data(), y1.data() );
        }
        
        /*!@brief Classify whether and how two oriented line segments in 3-space intersect when they are projected to the x-y-plane.
         *
         * @return `Flag_T f`, specified by the following:
         *
         * - `f = Flag_T::Empty` if and only if the planar projections of the line segments do not intersect after sufficiently small perturbation.
         *
         * - `f = Flag_T::Intersection` if and only if  the line segments have exactly one point in common after sufficiently small perturbation.
         *
         * - `f = Flag_T::Error` if and only if the line segments have a point in common in 3-space or at least one of them has length 0.
         */

        Flag_T IntersectionType()
        {
            [[maybe_unused]] auto tag = [](){ return MethodName("IntersectionType"); };
            
            if constexpr ( verboseQ )
            {
                logprint(tag() + " in verbose mode.");
            }
            
            
            u[0] = x_1[0] - x_0[0];
            u[1] = x_1[1] - x_0[1];
            u[2] = x_1[2] - x_0[2];
            
            p[0] = y_1[0] - x_0[0];
            p[1] = y_1[1] - x_0[1];
            p[2] = y_1[2] - x_0[2];
            
            q[0] = x_1[0] - y_0[0];
            q[1] = x_1[1] - y_0[1];
            q[2] = x_1[2] - y_0[2];
            
            auto sign_uxp = Sign_Perturbed(u,p);
//            auto sign_uxp = Sign_Perturbed_Kahan(u,p);
            auto [sign_uxq, uxq_2] = Sign_Det_Perturbed(u,q);
            // P_1 = Det_Perturbed(u,q);
            P_1.c_0 = uxq_2;
            // It is measurably slower to compute P_1 in full here.
                        
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(sign_uxp);
                TOOLS_LOGDUMP(sign_uxq);
            }
        
            if( sign_uxp != Sign_T(0) )
            {
                if( sign_uxq != Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.1.1: The \"generic\" case. Nothing to do here.");
                    }
                }
                else // if( sign_uxq == Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("A.1.2: y_0 lies on line(x_0,x_1), but y_1 does not.");
                    }
                    flag = PointOnLineTest(y_0, x_0, x_1) ? Flag_T::Error : Flag_T::Empty;
                    return flag;
                }
            }
            else // if( sign_uxp == Sign_T(0) )
            {
                if( sign_uxq != Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.2.1: y_1 lies on line(x_0,x_1), but y_0 does not.");
                    }
                    flag = PointOnLineTest(y_1, x_0, x_1) ? Flag_T::Error : Flag_T::Empty;
                    return flag;
                }
                else // if( sign_uxq == Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.2.2: The line segments are colinear. Do interval check.");
                    }
                    flag = LinesColinearTest() ? Flag_T::Error : Flag_T::Empty;
                    return flag;
                }
            }
            
            // Now we have sign_uxp != 0 and sign_uxq != 0.
            if( sign_uxp != sign_uxq )
            {
                // The points {y_0[0],y_0[1]} and {y_1[0],y_1[1]} lie on the same side of the line through {x_0[0],x_0[1]} and {x_1[0],x_1[1]} (after perturbation).
                flag = Flag_T::Empty;
                return flag;
            }
            
            v[0] = y_1[0] - y_0[0];
            v[1] = y_1[1] - y_0[1];
            v[2] = y_1[2] - y_0[2];
            
            auto [sign_vxp,vxp_2] = Sign_Det_Perturbed(v,p);
            auto sign_vxq         = Sign_Perturbed(v,q);
//            auto sign_vxq         = Sign_Perturbed_Kahan(v,q);
            // P_0 = Det_Perturbed(p,v);
            P_0.c_0 = -vxp_2;
            // It is measurably slower to compute P_0 in full here.
            
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(sign_vxp);
                TOOLS_LOGDUMP(sign_vxq);
            }
        
            if( sign_vxp != Sign_T(0) )
            {
                if( sign_vxq != Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.1.1: The \"generic\" case; nothing to do here.");
                    }
                }
                else // if( sign_vxq == Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.1.2: x_1 lies on line(y_0,y_1), but x_0 does not.");
                    }
                    flag = PointOnLineTest(x_1, y_0, y_1) ? Flag_T::Error : Flag_T::Empty;
                    return flag;
                }
            }
            else // if( sign_vxp == Sign_T(0) )
            {
                if( sign_vxq != Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.2.1: x_0 lies on line(y_0,y_1), but x_1 does not.");
                    }
                    flag = PointOnLineTest(x_0, y_0, y_1) ? Flag_T::Error : Flag_T::Empty;
                    return flag;
                }
                else // if( sign_vxq == Sign_T(0) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("B.2.2: Both line segments are colinear. We checked this case before and found no intersection. So we simply do nothing.");
                    }
                }
            }
            
            // Now we have sign_uxp != 0, sign_uxq != 0, sign_vxp != 0, and sign_vxq != 0.
            if( sign_vxp != sign_vxq )
            {
                // The points {x_0[0],x_0[1]} and {x_1[0],x_1[1]} lie on the same side of the line through {y_0[0],y_0[1]} and {y_1[0],y_1[1]} (after perturbation).
                flag = Flag_T::Empty;
                return flag;
            }
            
            flag = Flag_T::Intersection;
            return flag;
        }
        
    public:
        
        /*!@brief Compute the intersection (if the internal flag indicates that it exists).
         *
         * @return Instance of type `Intersection`, indicating which line segments intersect (by their index), which line segement is on top, time of intersection, and handedness of the resulting crossing.
         */
        Intersection ComputeIntersection()
        {
            if( flag != Flag_T::Intersection )
            {
                wprint(MethodName("ComputeIntersection") + ": trying to compute a nonexistent intersection.");
                return Intersection::InvalidIntersection(flag);
            }
            
            // This post https://math.stackexchange.com/a/1008869/447001
            // told me how to determine which edge "goes over".

//            const LVector3_T uxv = cross(u,v);   // Does not overflow.
            
            // {Q.c_0, Q.c_1, Q.c_3} == {uxv[2], uxv[0], uxv[1]}
            const DepressedCubic Q = Det_Perturbed(u,v);
            
            if constexpr ( verboseQ ) { TOOLS_LOGDUMP(Q); }

            // Result has _three_ times as many limbs as Int, not four.
            // We only have to make sure that we have two extra bits here.
            const auto det_3 = long_mul( p[0], Q.c_1 )
                             + long_mul( p[1], Q.c_3 )
                             + long_mul( p[2], Q.c_0 );
            
            const Sign_T sign_3 = Sign(det_3);
             
            if( sign_3 == Sign_T(0) )
            {
                error( MethodName("ComputeIntersection") + ": The line segments " + ToString(k_) + " and " + ToString(l_) + " are coplanar. Moreover, if we arrive here, then `IntersectionType()` has returned `Flag_T::Intersection`. Hence, we have an intersection also in 3D. But `IntersectionType()` should have detected this already and should have returned `Flag_T::Error`. So we should not have come here." );
            }
            
            const Sign_T sign_2 = Sign(Q);
            // sign_2 != Sign_T(0), otherwise sign_3 would be equal to 0, too.
            
            // Det_Perturbed(d,v) == Det_Perturbed(p - v,v) == Det_Perturbed(p,v)
            // Det_Perturbed(d,u) == Det_Perturbed(u - q,u) == Det_Perturbed(u,q)

            // At this point, we have computed P_0.c_0 and P_1.c_0 already. So we can save 33% of the integer operations in the next two lines. Very likely, the other entries have not yet been computed; so saving more is unlikely.
            
//            P_0 = Det_Perturbed(p,v);
//            P_1 = Det_Perturbed(u,q);
            
            P_0.c_1 = long_det(p[1],p[2],v[1],v[2]);
            P_0.c_3 = long_det(p[2],p[0],v[2],v[0]);

            P_1.c_1 = long_det(u[1],u[2],q[1],q[2]);
            P_1.c_3 = long_det(u[2],u[0],q[2],q[0]);
            
//            const double s    = double(1) / ToDouble(Q.c_0);
//            const double t_0  = ToDouble(P_0.c_0) * s;
//            const double t_1  = ToDouble(P_1.c_0) * s;
//            
            const bool x_under_y_Q = (sign_3 == sign_2);
            
            // First edge must go over.
            if( x_under_y_Q )
            {
                return Intersection{ l_, k_, Time_T{ P_1, Q }, Time_T{ P_0, Q }, -sign_2, flag };
            }
            else
            {
                return Intersection{ k_, l_, Time_T{ P_0, Q }, Time_T{ P_1, Q },  sign_2, flag };
            }
        }
        
#include "Prosector4/Helpers.hpp"
        
    public:
        
        static constexpr std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static constexpr std::string ClassName()
        {
            return std::string("Prosector4")
                + "<" + TypeName<Int>
                + "," + TypeName<Idx>
                + "," + ToString(verboseQ)
                + ">";
        }
        
    }; // class Prosector4
    
} // namespace Knoodle
