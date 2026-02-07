#pragma once

namespace Knoodle
{
    using namespace Tools;
    using namespace Tensors;
    
    /*!
     * @brief Implements the _(Progressive) Action-Angle Method_. The main routines are `CreateRandomClosedPolygon` and `CreateRandomClosedPolygons` to generate one or many closed polygons with unit edge lengths.
     *
     * The class's only purpose is to initial the random number generator and to keep it alive during calls to `CreateRandomClosedPolygon`.
     *
     * @tparam Real_ A real floating point type.
     *
     * @tparam Int_  An integer type.
     *
     * @tparam Prng_T_ A class of a pseudorandom number generator.
     *
     *  @tparam progressiveQ_ If set to `false`, it uses the _Action-Angle Method_ by (Cantarella, Duplantier, Shonkwiler, and Uehara)[https://iopscience.iop.org/article/10.1088/1751-8113/49/27/275202]. Otherwise, it uses the the _Progressive Action-Angle Method_ sampler by Cantarella, Schumacher, and Shonkwiler.
     */
    
    template<
        typename Real_     = double,
        typename Int_      = Int64,
        typename Prng_T_   = Knoodle::PRNG_T,
        bool progressiveQ_ = true
    >
    class ActionAngleSampler
    {
        static_assert(FloatQ<Real_>,"");
        static_assert(IntQ<Int_>,"");
        
    public:
        
        using Real   = Real_;
        using Int    = Int_;
        using Prng_T = Prng_T_;
        
        static constexpr Int AmbDim = 3;
        static constexpr bool progressiveQ = progressiveQ_;
        
        struct Arg_T
        {
            bool wrap_aroundQ     = false;
            bool random_rotationQ = true;
            bool centralizeQ      = true;
        };
        
        using Vector_T = Tensors::Tiny::Vector<AmbDim,Real,Int>;
        
        ActionAngleSampler()
        :   random_engine { InitializedRandomEngine<Prng_T>() }
        {}
        
        ActionAngleSampler( Prng_T && random_engine_ )
        :   random_engine { random_engine_ }
        {}
        
        // We do not want copy constructors because of the random engine.
        ActionAngleSampler( const ActionAngleSampler & other ) = delete;
        
        ~ActionAngleSampler() = default;

    private:

        mutable Prng_T random_engine;
                    
        std::uniform_real_distribution<Real> dist_1   { -Real(1), Real(1) };
        
        std::uniform_real_distribution<Real> dist_2   { -Scalar::Pi<Real>, Scalar::Pi<Real> };
        
        std::normal_distribution<Real>       gaussian {Real(0),Real(1)};
        
        static constexpr Size_T max_trials = 1'000'000'000;
        
    public:
        
        /*! @brief Generates a single closed, equilateral random polygon.
         *
         *  This is a port of the routine `plc_random_equilateral_closed_polygon` from the C library [plCurve](https://jasoncantarella.com/wordpress/software/plcurve) by Ted Ashton, Jason Cantarella, Harrison Chapman, and Tom Eddy.
         *
         *  @param p Buffer for vertex positions; assumed to be of size `(n + wrap_aroundQ) * 3`. Coordinates are stored in interleaved form, i.e. the coordinates of each vertex lie contiguously in memory.
         *
         *  @param n Number of edges.
         *
         * @param args Struct of further arguments: If `args.wrap_aroundQ` is set to `true`, then the first vertex is repeated at the end. If `args.random_rotationQ` is set to `true`, then orientation of the polygon is randomized. If set to `false`, then the first edge always points to {1,0,0}, and the first triangle's normal will be `{0,0,1}. If `args.centralizeQ` is set to true, then the random polygon is translated so that its center of mass lies at the origin.
         */
        
        Int WriteRandomEquilateralPolygon( mptr<Real> p, const Int n, cref<Arg_T> args )
        {
            TOOLS_PTIMER(timer,MethodName("WriteRandomEquilateralPolygon"));
            
            if( args.wrap_aroundQ )
            {
                if( args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = true,
                            .centralizeQ         = true
                        }>(p,n);
                    }
                    else // if( !args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = true,
                            .centralizeQ         = false
                        }>(p,n);
                    }
                }
                else // if( !args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = false,
                            .centralizeQ         = true
                        }>(p,n);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = false,
                            .centralizeQ         = false
                        }>(p,n);
                    }
                }
            }
            else // if( !args.wrap_aroundQ )
            {
                if( args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = true,
                            .centralizeQ         = true
                        }>(p,n);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = true,
                            .centralizeQ         = false
                        }>(p,n);
                    }
                }
                else // if( !args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = false,
                            .centralizeQ         = true
                        }>(p,n);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygon_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = false,
                            .centralizeQ         = false
                        }>(p,n);
                    }
                }
            }
        }
        
    private:
        
        template<Arg_T args>
        Int WriteRandomEquilateralPolygon_impl( mptr<Real> p, const Int n )
        {
            // We use the user-supplied buffer as scratch space for the diagonal lengths d.
            // We need n-1 entries.
            // We have at least 3 * n space at disposal.
            // We write as much to the back as possible.

            mptr<Real> d = &p[2*n+1];
            
            bool rejectedQ = true;
            
            Size_T trials = 0;
            
            // We need to find d[0], d[1], ... , d[n-2] from the moment polytope.
            // d[0], d[1], ..., d[n-2] are diagonals of a fan polyhedron if and only if the following is satisfied:
            //
            // (1)      d[0  ] = 1;
            // (2)      d[n-2] = 1;
            
            // (4)      |d[i-1] - d[i]| <= 1    for i = 1,...,n-3
            // (5)      d[i-1] + d[i] >= 1      for i = 1,...,n-3
            // (6)      d[i] > 0                for i = 1,...,n-3 -- obsolete, follows from (4) and (5)!
            
            // (7)      |d[n-3] - d[n-2]| <= 1
            // (8)      d[n-3] + d[n-2] >= 1
            
            // Because d[n-2] == 1, the latter Real(2) reduce to
            
            // (7')     |d[n-3] - 1| <= 1
            // (8')     d[n-3] >= 0             obsolete, follows from (6) for i = n-3.
            
            // Because d[n-3] >0, the condition (4') reduces to
            
            //  (7'') d[n-3] <= 2.
            
            // For condition (7'') it is necessary that d[i] - (n - 3 - i ) <= d[n-3] <= 2.
            // So we may stop early if
            //
            // d[i] - (n - 3 - i ) > 2.
            //
            // which is equivalent to
            //
            //  (7''')  d[i] + i > n - 1.
            //
            // However, this has a chance to be satisfied only in step i roughly (n - 2)/2 or later.
            // Anyways, checking this comes at an extra cost; so (7''') does not seem to help.
            
            // This guarantees (1), (2).
            d[0  ] = Real(1);
            d[n-2] = Real(1);
            
            while( rejectedQ && (trials < max_trials) )
            {
                ++trials;
                
                if constexpr ( progressiveQ )
                {
                    for( Int i = 1; i < (n-2); ++i )
                    {
                        // This guarantees (4):
                        d[i] = d[i-1] + dist_1(random_engine);
                        
                        // Check condition (5).
                        rejectedQ = d[i-1] + d[i] < Real(1);
                        
                        if( rejectedQ )
                        {
                            break;
                        }
                    }
                }
                else
                {
                    for( Int i = 1; i < (n-2); ++i )
                    {
                        // This guarantees (4):
                        d[i] = d[i-1] + dist_1(random_engine);
                    }
                    
                    rejectedQ = false;
                    
                    // Check condition (5.
                    for( Int i = 1; i < (n-2); ++i )
                    {
                        rejectedQ = rejectedQ || (d[i-1] + d[i] < Real(1));
                    }
                }
                
                // Check condition (7'') for last diagonal:
                rejectedQ = rejectedQ || ( d[n-3] > Real(2) );
            }
            
            if( trials > max_trials )
            {
                eprint("No success after " + ToString(max_trials) + " trials.");
                return trials;
            }
            
            // Some buffer to compute the center of mass.
            Vector_T c { Real(0), Real(0), Real(0) };
            
            // Current edge vector.
            Vector_T e;
            // The current triangle's normal.
            Vector_T nu;
            // Some buffer for the cross product in Rodrigues' formula.
            Vector_T v;
            
            if constexpr ( args.random_rotationQ )
            {
                Real squared_norm;
                do
                {
                    e[0] = gaussian(random_engine);
                    e[1] = gaussian(random_engine);
                    e[2] = gaussian(random_engine);
                    squared_norm = e.SquaredNorm();
                }
                while( squared_norm == Real(0) );
                e /= Sqrt(squared_norm); // e is now a random unit vector
                
                do
                {
                    v[0] = gaussian(random_engine);
                    v[1] = gaussian(random_engine);
                    v[2] = gaussian(random_engine);
                    Cross(v,e,nu);
                    squared_norm = nu.SquaredNorm();
                }
                while( squared_norm == Real(0) );
                nu /= Sqrt(squared_norm); // nu is now a random unit vector perpendicular to e.
            }
            else
            {
                e[0] = Real(1);
                e[1] = Real(0);
                e[2] = Real(0);
                
                nu[0] = Real(0);
                nu[1] = Real(0);
                nu[2] = Real(1);
            }
            
            p[0] = Real(0);
            p[1] = Real(0);
            p[2] = Real(0);
            
            e.Write(&p[3]);
            
            if constexpr ( args.centralizeQ )
            {
                c = e;  //  c = { p[3], p[4], p[5] };
            }
            
            for( Int i = 0; i < n - 3; ++i )
            {
                // Next we compute the new unit vector that points to e by rotating e by the angle alpha about the unit normal of the triangle.
                
                // Compute angle alpha between diagonals d[i-1] and d[i] by the cosine identity.
                const Real cos_alpha = ( d[i] * d[i] + d[i+1] * d[i+1] - Real(1) )/( Real(2) * d[i] * d[i+1] );
                
                // 0 < alpha < Pi, so sin(alpha) is positive. Thus the following is safe.
                const Real sin_alpha = std::sqrt(Real(1) - cos_alpha * cos_alpha);
                
                Cross(nu,e,v);
                
                const Real factor = Dot(nu,e) * (Real(1)-cos_alpha);
                
                // Apply Rodrigues' formula
                e[0] = e[0] * cos_alpha + v[0] * sin_alpha + nu[0] * factor;
                e[1] = e[1] * cos_alpha + v[1] * sin_alpha + nu[1] * factor;
                e[2] = e[2] * cos_alpha + v[2] * sin_alpha + nu[2] * factor;
                
                // Normalize for stability
                e.Normalize();
                
                // Compute the new vertex position.
                p[3 * (i+2) + 0] = d[i+1] * e[0];
                p[3 * (i+2) + 1] = d[i+1] * e[1];
                p[3 * (i+2) + 2] = d[i+1] * e[2];
                
                if constexpr ( args.centralizeQ )
                {
                    c[0] += p[3 * (i+2) + 0];
                    c[1] += p[3 * (i+2) + 1];
                    c[2] += p[3 * (i+2) + 2];
                }
                
                // Now we also rotate the triangle's unit normal by theta[i] about the unit vector e.
                
                Cross(e,nu,v);
                
                const Real theta_i   = dist_2(random_engine);
                const Real cos_theta = std::cos(theta_i);
                const Real sin_theta = std::sin(theta_i);
                const Real factor_2  = Dot(e,nu) * (Real(1)-cos_theta);
                
                // Apply Rodrigues' formula
                nu[0] = nu[0] * cos_theta + v[0] * sin_theta + e[0] * factor_2;
                nu[1] = nu[1] * cos_theta + v[1] * sin_theta + e[1] * factor_2;
                nu[2] = nu[2] * cos_theta + v[2] * sin_theta + e[2] * factor_2;
                
                // Normalize for stability
                nu.Normalize();
            }
            
            // Finally, we have to compute the vertex (n-1). We need to apply only an alpha-rotation.
            
            // Compute angle alpha beteen diagonals d[i-1] and d[i] by the cosine identity.
            
            const Real cos_alpha = ( d[n-3] * d[n-3] + d[n-2] * d[n-2] - Real(1) )/( Real(2) * d[n-3] * d[n-2] );
            
            // 0 < alpha < Pi, so sin(alpha) is postive. Thus the following is safe.
            const Real sin_alpha = std::sqrt(Real(1) - cos_alpha * cos_alpha);
            
            // Cross product of nu and unit vector e.
            Cross(nu,e,v);
            
            const Real factor = Dot(nu,e) * (Real(1)-cos_alpha);
            
            // Apply Rodrigues' formula
            e[0] = e[0] * cos_alpha + v[0] * sin_alpha + nu[0] * factor;
            e[1] = e[1] * cos_alpha + v[1] * sin_alpha + nu[1] * factor;
            e[2] = e[2] * cos_alpha + v[2] * sin_alpha + nu[2] * factor;
            
            // Normalize for stability
            e.Normalize();
            
            // Compute the new vertex position.
            p[3 * (n-1) + 0] = d[n-2] * e[0];
            p[3 * (n-1) + 1] = d[n-2] * e[1];
            p[3 * (n-1) + 2] = d[n-2] * e[2];
            
            if constexpr ( args.centralizeQ )
            {
                c[0] += p[3 * (n-1) + 0];
                c[1] += p[3 * (n-1) + 1];
                c[2] += p[3 * (n-1) + 2];
                
                c /= Real(n);
                // Now c contains the center of mass.
                
                for( Int i = 0; i < n; ++i )
                {
                    p[3 * i + 0] -= c[0];
                    p[3 * i + 1] -= c[1];
                    p[3 * i + 2] -= c[2];
                }
                
                if constexpr ( args.wrap_aroundQ )
                {
                    p[3 * n + 0] = -c[0];
                    p[3 * n + 1] = -c[1];
                    p[3 * n + 2] = -c[2];
                }
            }
            
            return trials;
        }
        
    public:
        
        /*! @brief Generates `sample_count` closed, equilateral random polygons.
         *
         * This is a port of the routine `plc_random_equilateral_closed_polygon` from the C library [plCurve](https://jasoncantarella.com/wordpress/software/plcurve) by Ted Ashton, Jason Cantarella, Harrison Chapman, and Tom Eddy.
         *
         * @param p Buffer for vertex positions; assumed to be of size `m * n * 3`. Coordinates are stored in interleaved form, i.e. the coordinates of each vertex lie contiguously in memory.
         *
         * This initializes a new pseudorandom generator for each thread.
         *
         * @param n Number of edges.
         *
         * @param m Number of samples to generate.
         *
         * @param args Struct of further arguments: If `args.wrap_aroundQ` is set to `true`, then the first vertex is repeated at the end. If `args.random_rotationQ` is set to `true`, then orientation of the polygon is randomized. If set to `false`, then the first edge always points to {1,0,0}, and the first triangle's normal will be `{0,0,1}. If `args.centralizeQ` is set to true, then the random polygon is translated so that its center of mass lies at the origin.
         *
         * @param thread_count Number of threads to use. Best practice is to set this to the number of performance cores on your system.
         */
        
        Int WriteRandomEquilateralPolygons(
            mptr<Real> p, const Int m, const Int n, cref<Arg_T> args, Int thread_count = 1
        )
        {
            TOOLS_PTIMER(timer,MethodName("WriteRandomEquilateralPolygons"));
            
            if( args.wrap_aroundQ )
            {
                if( args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = true,
                            .centralizeQ         = true
                        }>(p,m,n,thread_count);
                    }
                    else // if( !args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = true,
                            .centralizeQ         = false
                        }>(p,m,n,thread_count);
                    }
                }
                else // if( !args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = false,
                            .centralizeQ         = true
                        }>(p,m,n,thread_count);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = true,
                            .random_rotationQ    = false,
                            .centralizeQ         = false
                        }>(p,m,n,thread_count);
                    }
                }
            }
            else // if( !args.wrap_aroundQ )
            {
                if( args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = true,
                            .centralizeQ         = true
                        }>(p,m,n,thread_count);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = true,
                            .centralizeQ         = false
                        }>(p,m,n,thread_count);
                    }
                }
                else // if( !args.random_rotationQ )
                {
                    if( args.centralizeQ )
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = false,
                            .centralizeQ         = true
                        }>(p,m,n,thread_count);
                    }
                    else
                    {
                        return this->template WriteRandomEquilateralPolygons_impl<{
                            .wrap_aroundQ        = false,
                            .random_rotationQ    = false,
                            .centralizeQ         = false
                        }>(p,m,n,thread_count);
                    }
                }
            }
        }
            
    private:
        
        template<Arg_T args>
        Int WriteRandomEquilateralPolygons_impl(
            mptr<Real> p, const Int m, const Int n, const Int thread_count
        )
        {
            const Int trials = ParallelDoReduce(
                [=]( const Int thread) -> Int
                {
                    const Int k_begin = JobPointer( m, thread_count, thread     );
                    const Int k_end   = JobPointer( m, thread_count, thread + 1 );
                    
                    // Create a new instance of the class with its own random number generator.
                    ActionAngleSampler C;
                
                    Int trials = 0;
                    
                    const Int step = AmbDim * (n + args.wrap_aroundQ);
                    
                    for( Int k = k_begin; k < k_end; ++k )
                    {
                        trials += C.template WriteRandomEquilateralPolygon_impl<args>(&p[step * k], n);
                    }
                    
                    return trials;
                },
                AddReducer<Int,Int>(),
                Scalar::Zero<Int>,
                thread_count
            );

            return trials;
        }
        
    public:
        
        /*! @brief Returns the dimension of the ambient space. */
        
        static constexpr Int AmbientDimension()
        {
            return AmbDim;
        }
        
        /*! @brief Returns a string that identifies the class's method as specified by `tag`. */
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
        /*! @brief Returns a string that identifies the class. Good for debugging and printing messages. */
        
        static std::string ClassName()
        {
            return std::string(progressiveQ ? "Progressive" : "") + "ActionAngleSampler<" + TypeName<Real> + "," + TypeName<Int>  +  ">";
        }
        
    }; // ActionAngleSampler
    
} // namespace Knoodle
