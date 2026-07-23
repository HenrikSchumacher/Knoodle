#pragma once

#include <boost/multiprecision/cpp_int.hpp>

namespace Knoodle
{
    using Int128 = boost::multiprecision::int128_t;
    using Int256 = boost::multiprecision::int256_t;
    
//    using Int128 = _BitInt(128);
//    using Int256 = _BitInt(256);
}

namespace Tools
{
    
    template<> constexpr const char * TypeName<Knoodle::Int128>  = "I128";
    template<> constexpr const char * FullTypeName<Knoodle::Int128>  = "boost::multiprecision::int128_t";
    
    template<> constexpr const char * TypeName<Knoodle::Int256>  = "I256";
    template<> constexpr const char * FullTypeName<Knoodle::Int256>  = "boost::multiprecision::int256_t";
    
    std::string ToString( cref<Knoodle::Int128> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }

    std::string ToString( cref<Knoodle::Int256> number )
    {
        std::stringstream s;
        s << number;
        return s.str();
    }
}

namespace Knoodle
{
    /*!@brief A class for computing intersections of 3D line segments after projecting them to the plane.
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
     * @param Int_ Signed integral type used for coordinates of points.
     *
     * @param Idx_ Integral type used for indices.
     */
    template<SignedIntQ Int_, IntQ Idx_ = Int64>
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
        
        
#include "Prosector/Polynomial3.hpp"
#include "Prosector/IntersectionTime.hpp"
#include "Prosector/Intersection.hpp"

        
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

        Vector3_T x_0_;
        Vector3_T x_1_;
        Vector3_T y_0_;
        Vector3_T y_1_;
        
        Vector3_T u;
        Vector3_T v;
        Vector3_T p;
        Vector3_T q;
  
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
         * @param x_0 Start point of the first line segment; assumed to be a 3-vector.
         *
         * @param x_1 End point of the first line segment; assumed to be a 3-vector.
         
         * @param l Index of the second line segment (in a upstream data structure).
         *
         * @param y_0 Start point of the second line segment; assumed to be a 3-vector.
         *
         * @param y_1 End point of the second line segment; assumed to be a 3-vector.
         */
        
        void LoadLineSements(
            const Idx k, cptr<Int> x_0, cptr<Int> x_1,
            const Idx l, cptr<Int> y_0, cptr<Int> y_1
        )
        {
            flag = Flag_T::Uninitialized;

            k_ = k;
            l_ = l;
            
            x_0_.Read(x_0);
            x_1_.Read(x_1);
            y_0_.Read(y_0);
            y_1_.Read(y_1);
            
            
            //  x_1_     e     y_1_
            //      X------>X
            //      ^^     ^^
            //      | \q p/ |
            //      |  \ /  |
            //    u |   /   | v
            //      |  / \  |
            //      | /   \ |
            //      |/     \|
            //      X------>X
            //  x_0_     d     y_0_
            
            u = x_1_ - x_0_;
            v = y_1_ - y_0_;
            
            p = y_1_ - x_0_;
            q = x_1_ - y_0_;
            
            // TODO: We could precompute the relevant cross products, Prosector::Cross(u,v), Prosector::Cross(u,p), Prosector::Cross(u,q), Prosector::Cross(v,p), Prosector::Cross(v,q). However, IntersectionType() needs to know only signs of DetSign_Perturbed(u,p), etc. and it might terminate with Flag_T::Empty, in which case we had done some computations in vein.
        }
        
        // Somewhat pointless.
        void LoadLineSements(
            const Idx i_, cref<Vector3_T> x_0, cref<Vector3_T> x_1,
            const Idx j_, cref<Vector3_T> y_0, cref<Vector3_T> y_1
        )
        {
            LoadLineSements(i_, x_0.data(), x_1.data(), j_, y_0.data(), y_1.data());
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

        template<bool verboseQ = false>
        Flag_T IntersectionType()
        {
            if constexpr ( verboseQ )
            {
                wprint(ClassName()+"::IntersectionType in verbose mode.");
            }
            
            // Caution:
            // We should have checked that x_0_ != x_1_ and y_0_ != y_1_ before we arrive here.
            // Otherwise this returns Error.
            // Note: we could detect whether x_0_ == x_1_ or y_0_ == y_1_ also here, but this would lead to running these checks mutliple times. They should really carried out somewhere else.

            sign_uxp = DetSign_Perturbed(u,p);
            if( sign_uxp == Sign_T(0) )
            {
                // x_0_ == x_1_ or y_1_ lies on the line through x_0_ and x_1_ in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then x_0_ != x_1_.
            
            sign_uxq = DetSign_Perturbed(u,q);
            if( sign_uxq == Sign_T(0) )
            {
                // y_0_ lies on the line through x_0_ and x_1_ in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            sign_vxp = DetSign_Perturbed(v,p);
            if( sign_vxp == Sign_T(0) )
            {
                // y_0_ == y_1_ or x_0_ lies on the line through y_0_ and y_1_ in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then y_0_ != y_1_.
            
            sign_vxq = DetSign_Perturbed(v,q);
            if( sign_vxq == Sign_T(0) )
            {
                // x_1_ lies on the line through y_0_ and y_1_ in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then none of the line segments has length 0 in 3-space.
            // We intentionally check all of uxp, uxq, vxp, vxq before moving on to detect the case of degenerate line segments.
            
            if( sign_uxp != sign_uxq )
            {
                // The points {y_0_[0],y_0_[1]} and {y_1_[0],y_1_[1]} lie on the same side of the line through {x_0_[0],x_0_[1]} and {x_1_[0],x_1_[1]} (after perturbation).
                flag = Flag_T::Empty;
                return flag;
            }
            
            if( sign_vxp != sign_vxq )
            {
                // The points {x_0_[0],x_0_[1]} and {x_1_[0],x_1_[1]} lie on the same side of the line through {y_0_[0],y_0_[1]} and {y_1_[0],y_1_[1]} (after perturbation).
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
            
            LVector3_T uxv = Prosector::Cross(u,v);
            
            LLInt det_3   = LLInt{p[0]} * LLInt{uxv[0]}
                          + LLInt{p[1]} * LLInt{uxv[1]}
                          + LLInt{p[2]} * LLInt{uxv[2]};
            
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
                wprint("The line segments are coplanar");
            }
            
//            Polynomial2 uxv_perturbed = Det_Perturbed(u,v);
            Polynomial3 uxv_perturbed { uxv[2], uxv[0], uxv[1] };
            
            Sign_T sign_2 = uxv_perturbed.Sign();
            
            if( sign_2 == Sign_T(0) )
            {
                wprint("The projected line segments are parallel.");
            }
            
            bool x_under_y_Q = (sign_3 == sign_2);
            
            // Det_Perturbed(d,v) == Det_Perturbed(p - v,v) == Det_Perturbed(p,v)
            // Det_Perturbed(d,u) == Det_Perturbed(u - q,u) == Det_Perturbed(u,q)
            
            IntersectionTime t_0 { Det_Perturbed(p,v), uxv_perturbed };
            IntersectionTime t_1 { Det_Perturbed(u,q), uxv_perturbed };
            
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
        
#include "Prosector/Helpers.hpp"
        
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
