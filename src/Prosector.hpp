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
    template<IntQ Int_, IntQ Idx_ = Int64>
    class Prosector final
    {
    public:
        
        static_assert(SameQ<Int_,Int32> || SameQ<Int_,Int64>,"");
        
//        static constexpr Size_T bitlength = bitlength_;
//        static_assert(bitlength <= Size_T(64),"");
//        

//        
//        using Int    = std::conditional< bitlength <= Size_T(32), Int32, Int64>;
//        using LInt   = std::conditional< bitlength <= Size_T(16), Int32,
//                           std::conditional<bitlength <= Size_T(32), Int64, Int128 >
//                       >;
//        using LLInt  = std::conditional< bitlength <= Size_T(16), Int64,
//                           std::conditional<bitlength <= Size_T(32), Int128, Int256>
//                       >;
        using Int    = Int_;
        using LInt   = std::conditional_t<SameQ< Int,Int32>,Int64 ,Int128>;
        using LLInt  = std::conditional_t<SameQ<LInt,Int64>,Int128,Int256>;
        
        using Idx    = Idx_;
        using Sign_T = FastInt8; // Solely for signs.
        
//        using Prosector_T = Prosector<Idx>;
        using Vector3_T   = Tiny::Vector<3,Int ,Idx>;
        using LVector3_T  = Tiny::Vector<3,LInt,Idx>;
        
        enum class Flag_T : int
        {
            Uninitialized =  0,
            Empty         =  2, // Empty intersection.
            Intersection  =  1, // Nontrivial intersection found.
            Error         = -1  // Lines must intersect in 3D; treat this as error.
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

        Vector3_T x_0;
        Vector3_T x_1;
        Vector3_T y_0;
        Vector3_T y_1;
        
        Vector3_T u;
        Vector3_T v;
        Vector3_T p;
        Vector3_T q;
  
        Idx k;
        Idx l;
        
        Sign_T sign_uxp;
        Sign_T sign_uxq;
        Sign_T sign_vxp;
        Sign_T sign_vxq;
        
        Flag_T flag { Flag_T::Uninitialized };
        
        
    public:
        
        Flag_T Flag() const
        {
            return flag;
        }
        
        /**
         * @brief Classify whether and how two oriented line segments in 3-space intersect when they are parallel projected to the x-y-plane.
         *
         * This applies a symbolic perturbation technique. In fact, the projection is done parallel to the x-y-plane along the perturbed vector `{eps,eps * eps,1}`, i.e., a point `{x[0],x[1],x[2]}` is mapped to `{x[0] - eps * x[3], x[1] - eps * eps * x[3]}`, and the intersection is analyzed in the limit eps -> 0 from the right. This handles the following degenerate cases consistently, as long as the line segments  in 3-space are disjoint and have positive length:
         *
         *  - The projection of the two line segments have a line segment in common.
         *
         *  - An end point of a projected line segment lies on the other.
         *
         *  - A line segment is projected to a single point.
         *
         * @param x_0 Start point of the first line segment; assumed to be a 3-vector.
         *
         * @param x_1 end point of the first line segment; assumed to be a 3-vector.
         *
         * @param y_0 Start point of the second line segment; assumed to be a 3-vector.
         *
         * @param y_1 end point of the second line segment; assumed to be a 3-vector.
         *
         * @return `Flag_T f`, specified by the following:
         *
         * `f = Flag_T::Empty` if and only if the planar projections of the line segments do not intersect after sufficiently small perturbation.
         *
         * `f = Flag_T::Intersection` if and only if  the line segments have exactly one point in common after sufficiently small perturbation.
         *
         * `f = Flag_T::Error` if and only if the line segments have a point in common in 3-space or at least one of them has length 0.
         */
        
    public:
        
        void LoadSegments(
            const Idx k_, cptr<Int> x_0_, cptr<Int> x_1_,
            const Idx l_, cptr<Int> y_0_, cptr<Int> y_1_
        )
        {
            flag = Flag_T::Uninitialized;

            k = k_;
            l = l_;
            
            x_0.Read(x_0_);
            x_1.Read(x_1_);
            y_0.Read(y_0_);
            y_1.Read(y_1_);
            
            
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
            
            u = x_1 - x_0;
            v = y_1 - y_0;
            
            p = y_1 - x_0;
            q = x_1 - y_0;
            
            // TODO: We could precompute the relevant cross products, Prosector::Cross(u,v), Prosector::Cross(u,p), Prosector::Cross(u,q), Prosector::Cross(v,p), Prosector::Cross(v,q). However, IntersectionType() needs to know only signs of DetSign_Perturbed(u,p), etc. and it might terminate with Flag_T::Empty, in which case we had done some computations in vein.
        }
        
        // Somewhat pointless.
        void LoadSegments(
            const Idx i_, cref<Vector3_T> x_0_, cref<Vector3_T> x_1_,
            const Idx j_, cref<Vector3_T> y_0_, cref<Vector3_T> y_1_
        )
        {
            LoadSegments(i_, x_0_.data(), x_1_.data(), j_, y_0_.data(), y_1_.data());
        }

        template<bool verboseQ = false>
        Flag_T IntersectionType()
        {
            if constexpr ( verboseQ )
            {
                wprint(ClassName()+"::IntersectionType in verbose mode.");
            }
            
            // Caution:
            // We should have checked that x_0 != x_1 and y_0 != y_1 before we arrive here.
            // Otherwise this returns Error.
            // Note: we could detect whether x_0 == x_1 or y_0 == y_1 also here, but this would lead to running these checks mutliple times. They should really carried out somewhere else.

            sign_uxp = DetSign_Perturbed(u,p);
            if( sign_uxp == Sign_T(0) )
            {
                // x_0 == x_1 or y_1 lies on the line through x_0 and x_1 in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then x_0 != x_1.
            
            sign_uxq = DetSign_Perturbed(u,q);
            if( sign_uxq == Sign_T(0) )
            {
                // y_0 lies on the line through x_0 and x_1 in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            sign_vxp = DetSign_Perturbed(v,p);
            if( sign_vxp == Sign_T(0) )
            {
                // y_0 == y_1 or x_0 lies on the line through y_0 and y_1 in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then y_0 != y_1.
            
            sign_vxq = DetSign_Perturbed(v,q);
            if( sign_vxq == Sign_T(0) )
            {
                // x_1 lies on the line through y_0 and y_1 in 3-space.
                flag = Flag_T::Error;
                return flag;
            }
            
            // If we arrive here, then none of the line segments has length 0 in 3-space.
            // We intentionally check all of uxp, uxq, vxp, vxq before moving on to detect the case of degenerate line segments.
            
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
        
        Intersection ComputeIntersection()
        {
            if( flag != Flag_T::Intersection )
            {
                eprint(MethodName("ComputeIntersection") + ": trying to compute an nonexistent intersection.");
                return Intersection::InvalidIntersection(flag);
            }
            
            // This post https://math.stackexchange.com/a/1008869/447001
            // told me how to determine which edge ``goes over''.
            
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
                return Intersection{ l, k, t_1, t_0, -sign_2, flag };
            }
            else
            {
                return Intersection{ k, l, t_0, t_1,  sign_2, flag };
                
            }
        }
        
#include "Prosector/Helpers.hpp"
        
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
            return ct_string("Prosector")
                + "<" + TypeName<Int>
                + "," + TypeName<Idx>
                + ">";
        }
        
    }; // class Prosector
    
} // namespace Knoodle
