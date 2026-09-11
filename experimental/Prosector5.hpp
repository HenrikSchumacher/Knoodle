#pragma once

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
     * The usage of the class is as follows: First one calls `ComputeLineSegments`. The returned flag tells us whether a valid intersection has been found or whether there were any issues. If the return value is `Flag_T::Intersection`, then one can call `GetIntersection` to get an instance of `struct` `Intersection_T` that contains the relevant information.
     *
     * @tparam Int_ Signed integral type used for coordinates of points.
     *
     * @tparam Idx_ Integral type used for indices.
     *
     * @tparam verboseQ Whether to log events more granulary. Only meant for debugging.
     */
    template<SignedIntQ Int_, IntQ Idx_ = Int64, bool verboseQ = false>
    class Prosector5 final
    {
    public:
        
        static_assert(SameQ<Int_,Int32> || SameQ<Int_,Int64>,"");
        
        /*!@brief Integral type used for coordinates.*/
        using Int    = Int_;
        /*!@brief Longer integral type used for internal computations.*/
        using LInt   = decltype(long_mul(Int{1},Int{1}));
        /*!@brief Even longer integral type used for internal computations.*/
        using LLInt  = decltype(long_mul(LInt{1},LInt{1}));
    
        static_assert(SameQ<LInt,decltype(long_fma(Int{1},Int{1},Int{1}))> ,"");
        static_assert(SameQ<LInt,decltype(long_det(Int{1},Int{0},Int{0},Int{1}))> ,"");
        
        using Idx    = Idx_;
        using Sign_T = FastInt8; // Solely for signs.
        
        using Class_T     = Prosector5;
        using Prosector_T = Class_T;
        using Vector3_T   = Tiny::Vector<3,Int,Int>;
        using LVector3_T  = Tiny::Vector<3,LInt,Int>;
        
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
        
#include "../src/Prosector/DepressedCubic.hpp"
#include "../src/Prosector/Helpers.hpp"
#include "../src/Prosector/DegeneracyChecks.hpp"
        
//#include "../src/Prosector/IntersectionTime.hpp"
//        using Time_T = IntersectionTime;
        
//#include "../src/Prosector/IntersectionTime_Double.hpp"
//        using Time_T = IntersectionTime_Double;
        
#include "../src/Prosector/IntersectionTime_Hybrid.hpp"
        using Time_T = IntersectionTime_Hybrid;

#include "../src/Prosector/Intersection.hpp"
        
    public:
        
        // Default constructor
        Prosector5() = default;
        // Default destructor
        ~Prosector5() = default;
        // Copy constructor
        Prosector5( const Prosector5 & other ) = default;
        // Copy assignment operator
        Prosector5 & operator=( const Prosector5 & other ) = default;
        // Move constructor
        Prosector5( Prosector5 && other ) = default;
        // Move assignment operator
        Prosector5 & operator=( Prosector5 && other ) = default;
        
    private:
        
        Vector3_T x_0;
        Vector3_T x_1;
        Vector3_T y_0;
        Vector3_T y_1;
        
        Vector3_T u;
        Vector3_T v;
        Vector3_T p;
        Vector3_T q;

        Idx k_;
        Idx l_;
        Flag_T flag;
        Intersection_T isec;
        
    public:
        
        /*!@brief Classify whether and how two oriented line segments in 3-space intersect when they are projected to the x-y-plane.
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
         *
         * @return `Flag_T f`, specified by the following:
         *
         * - `f = Flag_T::Empty` if and only if the planar projections of the line segments do not intersect after sufficiently small perturbation.
         *
         * - `f = Flag_T::Intersection` if and only if  the line segments have exactly one point in common after sufficiently small perturbation.
         *
         * - `f = Flag_T::Error` if and only if the line segments have a point in common in 3-space or at least one of them has length 0.
         */

        Flag_T ComputeIntersection(
            const Idx k, cptr<Int> x0, cptr<Int> x1,
            const Idx l, cptr<Int> y0, cptr<Int> y1
        )
        {
            if constexpr ( verboseQ )
            {
                Msgr::logprint("ComputeIntersection", "Running in verbose mode.");
            }
            
            LoadLineSegments( k, x0, x1, l, y0, y1 );
            
            Compute();
            
            return Flag();
        }
        
        /*!@brief Return the previously computed intersection. Use this only if `Flag()` is `Flag_T::Intersection`
         *
         * @return Instance of type `Intersection_T`, indicating which line segments intersect (by their index), which line segment is on top, time of intersection, and handedness of the resulting crossing.
         */
        cref<Intersection_T> GetIntersection()
        {
            return isec;
        }
        
        /*!@brief Return the current value of the internal state flag.*/
        Flag_T Flag() const
        {
            return flag;
        }

    private:

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
        
        void LoadLineSegments(
            const Idx k, cptr<Int> x0, cptr<Int> x1,
            const Idx l, cptr<Int> y0, cptr<Int> y1
        )
        {
            k_ = k;
            l_ = l;
            flag = Flag_T::Uninitialized;
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
            
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(x_0);
                TOOLS_LOGDUMP(x_1);
                TOOLS_LOGDUMP(y_0);
                TOOLS_LOGDUMP(y_1);
            }
        }

        /*!@brief Classify whether and how two oriented line segments in 3-space intersect when they are projected to the x-y-plane.
         *
         * - `f = Flag_T::Empty` if and only if the planar projections of the line segments do not intersect after sufficiently small perturbation.
         *
         * - `f = Flag_T::Intersection` if and only if  the line segments have exactly one point in common after sufficiently small perturbation.
         *
         * - `f = Flag_T::Error` if and only if the line segments have a point in common in 3-space or at least one of them has length 0.
         */

        void Compute()
        {
            if constexpr ( verboseQ )
            {
                Msgr::logprint("Compute", "Running in verbose mode.");
            }
            
            u = x_1 - x_0;
            p = y_1 - x_0;
            q = x_1 - y_0;
            
            auto sign_uxp = Sign_Perturbed(u,p);
//            auto sign_uxp = Sign_Perturbed_Kahan(u,p);
            auto [sign_uxq, uxq_2] = Sign_Det_Perturbed(u,q);
                        
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(sign_uxp);
                TOOLS_LOGDUMP(sign_uxq);
            }
        
            if( !ZeroQ(sign_uxp) )
            {
                if( !ZeroQ(sign_uxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.1.1: The \"generic\" case. Nothing to do here.");
                    }
                }
                else // if( ZeroQ(sign_uxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("A.1.2: y_0 lies on line(x_0,x_1), but y_1 does not.");
                    }
                    flag = PointOnLineTest(y_0, x_0, x_1) ? Flag_T::Error : Flag_T::Empty;
                    return;
                }
            }
            else // if( ZeroQ(sign_uxp) )
            {
                if( !ZeroQ(sign_uxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.2.1: y_1 lies on line(x_0,x_1), but y_0 does not.");
                    }
                    flag = PointOnLineTest(y_1, x_0, x_1) ? Flag_T::Error : Flag_T::Empty;
                    return;
                }
                else // if( ZeroQ(sign_uxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case A.2.2: The line segments are colinear. Do interval check.");
                    }
                    flag = LinesColinearTest() ? Flag_T::Error : Flag_T::Empty;
                    return;
                }
            }
            
            // Now we have sign_uxp != 0 and sign_uxq != 0.
            if( sign_uxp != sign_uxq )
            {
                // The points {y_0[0],y_0[1]} and {y_1[0],y_1[1]} lie on the same side of the line through {x_0[0],x_0[1]} and {x_1[0],x_1[1]} (after perturbation).
                flag = Flag_T::Empty;
                return;
            }
            
            v = y_1 - y_0;
            
            auto [sign_vxp,vxp_2] = Sign_Det_Perturbed(v,p);
            auto sign_vxq         = Sign_Perturbed(v,q);
            
            if constexpr ( verboseQ )
            {
                TOOLS_LOGDUMP(sign_vxp);
                TOOLS_LOGDUMP(sign_vxq);
            }
        
            if( !ZeroQ(sign_vxp) )
            {
                if( !ZeroQ(sign_vxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.1.1: The \"generic\" case; nothing to do here.");
                    }
                }
                else // if( ZeroQ(sign_vxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.1.2: x_1 lies on line(y_0,y_1), but x_0 does not.");
                    }
                    flag = PointOnLineTest(x_1, y_0, y_1) ? Flag_T::Error : Flag_T::Empty;
                    return;
                }
            }
            else // if( ZeroQ(sign_vxp) )
            {
                if( !ZeroQ(sign_vxq) )
                {
                    if constexpr ( verboseQ )
                    {
                        logprint("Case B.2.1: x_0 lies on line(y_0,y_1), but x_1 does not.");
                    }
                    flag = PointOnLineTest(x_0, y_0, y_1) ? Flag_T::Error : Flag_T::Empty;
                    return;
                }
                else // if( ZeroQ(sign_vxq) )
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
                return;
            }
            
            // At this point we know that the projected line segments intersect in the x-y-plane. We also have to check whether the line segments themselves intersect in 3-space.
            
            // This post https://math.stackexchange.com/a/1008869/447001
            // taught me how to determine which edge "goes over".

            const DepressedCubic Q = Det_Perturbed(u,v);
            
            if constexpr ( verboseQ ) { TOOLS_LOGDUMP(Q); }

            // Result has _three_ times as many limbs as Int, not four.
            // We only have to make sure that we have two extra bits here.
            const auto det_3 = long_mul( p[0], Q.c_1 )
                             + long_mul( p[1], Q.c_3 )
                             + long_mul( p[2], Q.c_0 );
            
            const Sign_T sign_3 = Sign<Sign_T>(det_3);
            
            if( ZeroQ(sign_3) )
            {
                // We know that we have an intersection in the x-y-plane.
                // If the line segments are coplanar in 3D, then there must be an intersection, too.
                if constexpr ( verboseQ )
                {
                    Msgr::logprint("ComputeIntersection", "The line segments ", k_, " and ", l_, " are coplanar.");
                }
                flag = Flag_T::Error;
                return;
            }
            
            const Sign_T sign_2 = Sign<Sign_T>(Q);
            
            if( ZeroQ(sign_2) )
            {
                // We should not get here
                Msgr::eprint("ComputeIntersection", "The projections of the line segments ", k_, " and ", l_, " are parallel. No handedness assignable. But this case should have been caught before, so we should not have gotton here.");
            }
            
            bool x_over_y_Q = (sign_3 != sign_2);
            
            using Tools::NegativeQ;
            
            // First edge must go over.
            if( x_over_y_Q )
            {
                isec = Intersection_T(k_,l_,!NegativeQ(sign_2));
            }
            else
            {
                isec = Intersection_T(l_,k_, NegativeQ(sign_2));
            }
            
            flag = Flag_T::Intersection;
            return;
        }
        
    public:
        
        void LoadFirstLineSegment(
            const Idx k, cptr<Int> x0, cptr<Int> x1
        )
        {
            k_ = k;
            x_0.Read(x0);
            x_1.Read(x1);
            u = x_1 - x_0;
        }

        Time_T ComputeIntersectionTime( const Idx l, cptr<Int> y0, cptr<Int> y1 )
        {
            l_ = l;
            y_0.Read(y0);
            y_1.Read(y1);
            p = y_1 - x_0;
            v = y_1 - y_0;
            // Quite expensive.
            return Time_T{ Det_Perturbed(p,v), Det_Perturbed(u,v) };
        }
        
    public:
        
        using Msgr = Tools::Messenger<Class_T>;
        
        static consteval auto ClassName()
        {
            return ct_string("Prosector5")
                + "<" + TypeName<Int>
                + "," + TypeName<Idx>
                + "," + to_ct_string(verboseQ)
                + ">";
        }
        
    }; // class Prosector5
    
} // namespace Knoodle
