#pragma once

#include "Prosector2/Types.hpp"

namespace Knoodle
{
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
    class Prosector final
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
        using LInt   = std::conditional_t<SameQ< Int,Int32>,Int64 ,Int128>;
        /*!@brief Even longer integral type used for internal computations.*/
        using LLInt  = std::conditional_t<SameQ<LInt,Int64>,Int128,Int256>;
        
//        /*!@brief Longer integral type used for internal computations.*/
//        using LInt   = std::conditional_t<SameQ< Int,Int32>,Int128,Int128>;
//        /*!@brief Even longer integral type used for internal computations.*/
//        using LLInt  = std::conditional_t<SameQ<LInt,Int64>,Int256,Int256>;
        
        using Idx    = Idx_;
        using Sign_T = FastInt8; // Solely for signs.
        
//        using Prosector_T = Prosector<Idx>;
        using Vector3_T   = Tiny::Vector<3,Int ,Idx>;
        using LVector3_T  = Tiny::Vector<3,LInt,Idx>;
        
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
        
        
#include "Prosector2/Polynomial3.hpp"
#include "Prosector2/IntersectionTime.hpp"
#include "Prosector2/Intersection.hpp"

        
        // Default constructor
        Prosector() = default;
        // Default destructor
        ~Prosector() = default;
        
        // Copy constructor
        Prosector( const Prosector & other ) = default;
        // Copy assignment operator
        Prosector & operator=( const Prosector & other ) = default;
        // Move constructor
        Prosector( Prosector && other ) = default;
        // Move assignment operator
        Prosector & operator=( Prosector && other ) = default;
        
    protected:

        Vector3_T x_0;
        Vector3_T x_1;
        Vector3_T y_0;
        Vector3_T y_1;
        
        LVector3_T uxv;
        LVector3_T uxp;
        LVector3_T uxq;
        LVector3_T vxp;
        LVector3_T vxq;
        
        Idx k_;
        Idx l_;
        
        Sign_T sign_uxp;
        Sign_T sign_uxq;
        Sign_T sign_vxp;
        Sign_T sign_vxq;
        
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
            
            x_0.Read(x0);
            x_1.Read(x1);
            y_0.Read(y0);
            y_1.Read(y1);
            
            
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
//            
            LVector3_T u { LInt{x_1[0]-x_0[0]}, LInt{x_1[1]-x_0[1]}, LInt{x_1[2]-x_0[2]} };
            LVector3_T v { LInt{y_1[0]-y_0[0]}, LInt{y_1[1]-y_0[1]}, LInt{y_1[2]-y_0[2]} };
            LVector3_T p { LInt{y_1[0]-x_0[0]}, LInt{y_1[1]-x_0[1]}, LInt{y_1[2]-x_0[2]} };
            LVector3_T q { LInt{x_1[0]-y_0[0]}, LInt{x_1[1]-y_0[1]}, LInt{x_1[2]-y_0[2]} };
            
            // TODO: It should be possible to compute this with only 3 cross products.
            uxv = Cross(u,v);   // Does not overflow.
            
            uxp = Cross(u,p);   // Does not overflow.
//            uxq = Cross(u,q);   // Does not overflow.
            //   q ==   v -   p +   u
            // uxq == uxv - uxp + uxu
            uxq = uxv - uxp;

            vxp = Cross(v,p);   // Does not overflow.
//            vxq = Cross(v,q);   // Does not overflow.
            //   q ==   v -   p +   u
            // vxq == vxv - vxp + vxu
            vxq = -vxp - uxv;

            
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(k_);
                TOOLS_LOGDUMP(l_);
                
                TOOLS_LOGDUMP(x_0);
                TOOLS_LOGDUMP(x_1);
                TOOLS_LOGDUMP(y_0);
                TOOLS_LOGDUMP(y_1);
                
                TOOLS_LOGDUMP(u);
                TOOLS_LOGDUMP(v);
                TOOLS_LOGDUMP(p);
                TOOLS_LOGDUMP(q);
                
                TOOLS_LOGDUMP(uxv);
                TOOLS_LOGDUMP(uxp);
                TOOLS_LOGDUMP(uxq);
                TOOLS_LOGDUMP(vxp);
                TOOLS_LOGDUMP(vxq);
            }
        }
        
        // Somewhat pointless.
        void LoadLineSements(
            const Idx i_, cref<Vector3_T> x0, cref<Vector3_T> x1,
            const Idx j_, cref<Vector3_T> y0, cref<Vector3_T> y1
        )
        {
            LoadLineSements(i_, x0.data(), x1.data(), j_, y0.data(), y1.data());
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
            
            // Precondition: x_0 != x_1 and y_0 != y_1.
            
            sign_uxp = Sign_Perturbed(uxp);
            sign_uxq = Sign_Perturbed(uxq);
            
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
            
            sign_vxp = Sign_Perturbed(vxp);
            sign_vxq = Sign_Perturbed(vxq);
            
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
            
            if( sign_uxp != sign_uxq )
            {
                // The points {y_0[0],y_0[1]} and {y_1[0],y_1[1]} lie on the same side of the line through {x_0[0],x_0[1]} and {x_1[0],x_1[1]} (after perturbation).
                flag = Flag_T::Empty;
                return flag;
            }
            
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
            
            if constexpr ( verboseQ ) { TOOLS_LOGDUMP(uxv); }
            
//            LLInt det_3   = LLInt{y_1[0]-x_0[0]} * uxv[0]
//                          + LLInt{y_1[1]-x_0[1]} * uxv[1]
//                          + LLInt{y_1[2]-x_0[2]} * uxv[2];
            
            LLInt det_3   = LLInt{y_1[0]-x_0[0]} * LLInt{uxv[0]}
                          + LLInt{y_1[1]-x_0[1]} * LLInt{uxv[1]}
                          + LLInt{y_1[2]-x_0[2]} * LLInt{uxv[2]};
            
//            Vector3_T d = p - v; // == u - q
//            LLInt det_3_d = LLInt{d[0]} * LLInt{uxv[0]}
//                          + LLInt{d[1]} * LLInt{uxv[1]}
//                          + LLInt{d[2]} * LLInt{uxv[2]};
//
//            std::cout << "det_3 = " << det_3 << "\n";
//            std::cout << "det_3_d = " << det_3_d << std::endl;
            
            Sign_T sign_3 = Sign(det_3);
             
            if( sign_3 == Sign_T(0) )
            {
                // TODO: Better message and error handling.
                wprint(MethodName("ComputeIntersection") + ": The line segments " + ToString(k_) + " and " + ToString(l_) + " are coplanar.");
            }
                        
            Polynomial3 Q { uxv[2], uxv[0], uxv[1] };
            Sign_T sign_2 = Q.Sign();
            
            if( sign_2 == Sign_T(0) )
            {
                eprint(MethodName("ComputeIntersection") + ": The projections of the line segments " + ToString(k_) + " and " + ToString(l_) + " are parallel. No handedness assignable.");
            }
            
            bool x_under_y_Q = (sign_3 == sign_2);
            
            // Det_Perturbed(d,v) == Det_Perturbed(p - v,v) == Det_Perturbed(p,v)
            // Det_Perturbed(d,u) == Det_Perturbed(u - q,u) == Det_Perturbed(u,q)

            
            IntersectionTime t_0 { Polynomial3{ -vxp[2], -vxp[0], -vxp[1] }, Q };
            IntersectionTime t_1 { Polynomial3{  uxq[2],  uxq[0],  uxq[1] }, Q };
            
            // First edge must go over.
            if( x_under_y_Q )
            {
                return Intersection{ l_, k_, t_1, t_0, -sign_2, flag };
            }
            else
            {
                return Intersection{ k_, l_, t_0, t_1,  sign_2, flag };
                
            }
        }
        
#include "Prosector2/Helpers.hpp"
        
    public:
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        static std::string ClassName()
        {
            return ct_string("Prosector")
                + "<" + TypeName<Int>
                + "," + TypeName<Idx>
                + ">";
        }
        
    }; // class Prosector
    
} // namespace Knoodle
