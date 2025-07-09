template<Size_T t, bool appendQ = true>
void kv( const std::string & key, std::string & value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, std::string && value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, cref<Tiny::Vector<AmbDim,Real,Int>> value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ArrayToString(
            value.data(), {AmbDim},
            [](Real x){ return ToMathematicaString(x); }
        );
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, cref<char *> value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> \"" + value + "\"";
}

template<Size_T t, bool appendQ = true, typename T >
void kv( const std::string & key, cref<T> value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToString(value);
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, cref<double> value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToMathematicaString(value);
}

template<Size_T t, bool appendQ = true>
void kv( const std::string & key, cref<float> value )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> " + ToMathematicaString(value);
}




template<Size_T t, bool appendQ = true>
void PrintCallCounts( cref<typename Clisby_T::CallCounters_T> call_counters)
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"Call Counts\" -> <|";
        kv<t+1,0>("Transformation Loads", call_counters.load_transform);
        kv<t+1>("Matrix-Matrix Multiplications", call_counters.mm);
        kv<t+1>("Matrix-Vector Multiplications", call_counters.mv);
        kv<t+1>("Ball Overlap Checks", call_counters.overlap);
    log << "\n" + ct_tabs<t> + "|>";
}

template<Size_T t, bool appendQ = true>
void PrintClisbyFlagCounts( cref<FoldFlagCounts_T> counts )
{
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"Clisby Flag Counts\" -> <|";
        kv<t+1,0>("Accepted", counts[0]);
        kv<t+1>("Rejected by Input Check", counts[1]);
        kv<t+1>("Rejected by First Pivot Check", counts[2]);
        kv<t+1>("Rejected by Second Pivot Check", counts[3]);
        kv<t+1>("Rejected by Tree", counts[4]);
    log << "\n" + ct_tabs<t> + "|>";
}

template<Size_T t, bool appendQ = true>
void PrintIntersectionFlagCounts( cref<std::string> key, cref<IntersectionFlagCounts_T> counts )
{
    using F_T = Link_T::Intersector_T::F_T;
    
    auto get = [&counts]( F_T flag )
    {
        return counts[ToUnderlying(flag)];
    };
    
    log << (appendQ ? ",\n" : "\n") + ct_tabs<t> + "\"" + key + "\" -> <|";
        kv<t+1,0>("Empty Intersection", get(F_T::Empty));
        kv<t+1>("Transversal Intersection", get(F_T::Transversal) );
        kv<t+1>("Intersections on Corner of First Edge", get(F_T::AtCorner0) );
        kv<t+1>("Intersections on Corner of Second Edge", get(F_T::AtCorner1) );
        kv<t+1>("Intersections on Corners of Both Edges", get(F_T::CornerCorner) );
        kv<t+1>("Interval-like Intersections", get(F_T::Interval) );
        kv<t+1>("Spatial Intersections", get(F_T::Spatial) );
    log << "\n" + ct_tabs<t> + "|>";
    
}

//template<Size_T t, bool totalsQ, bool histogramQ, bool appendQ = true>
//void PrintCurvatureTorsion( cptr<Real> X )
//{
//    if constexpr ( !totalsQ && !histogramQ )
//    {
//        return;
//    }
////    
////    if( (bin_count <= Int(0)) || n < 4 )
////    {
////        return;
////    }
//    
//    if constexpr ( histogramQ )
//    {
//        curvature_hist.SetZero();
//        torsion_hist.SetZero();
//    }
//    
//    const Real scale = Frac<Real>(bin_count,Scalar::Pi<Real>);
//    
//    Real total_curvature = 0;
//    Real total_torsion = 0;
//
//    auto update = [&total_curvature,&total_torsion,X,scale,this](
//        Int i_A, Int i_B, Int i_C, Int i_D
//    )
//    {
//        auto [curvature,torsion] = CurvatureTorsion(X, i_A, i_B, i_C, i_D );
//        
//        total_curvature += curvature;
//        total_torsion   += Abs(torsion);
//        
//        if constexpr ( histogramQ )
//        {
//            const Int curvature_bin = Clamp(
//                static_cast<Int>( std::floor(curvature * scale) ),
//                Int(0), bin_count - Int(1)
//            );
//            
//            ++curvature_hist[curvature_bin];
//
//            const Int torsion_bin = Clamp(
//                static_cast<Int>( std::floor( (torsion + Scalar::Pi<Real>) * scale ) ),
//                Int(0), Int(2) * bin_count - Int(1)
//            );
//         
//            ++torsion_hist[torsion_bin];
//        }
//        else
//        {
//            (void)scale;
//        }
//    };
//    
//    update( n - 3, n - 2, n - 1, 0 );   // i = 0;
//    update( n - 2, n - 1, 0    , 1 );   // i = 1;
//    update( n - 1, 0,     1    , 2 );   // i = 2;
//    
//    for( Int i = 3; i < n; ++i )
//    {
//        update( i - 3, i - 2, i - 1, i - 0 );
//    }
//    
//    if constexpr ( totalsQ )
//    {
//        kv<t,appendQ>("Total Curvature",total_curvature);
//        kv<t>("Total Torsion",total_torsion);
//    }
//    
//    if constexpr ( histogramQ )
//    {
//        kv<t,totalsQ?appendQ:true>("Curvature Histogram",curvature_hist);
//        kv<t>("Torsion Histogram",torsion_hist);
//    }
//}


template<Size_T t, bool totalsQ, bool histogramQ, bool appendQ = true>
void PrintCurvatureTorsion( cptr<Real> X )
{
    if constexpr ( !totalsQ && !histogramQ )
    {
        return;
    }

    if( n < Int(4) )
    {
        return;
    }
    
    if constexpr ( histogramQ )
    {
        curvature_hist.SetZero();
        torsion_hist.SetZero();
    }
    
    const Real scale = Frac<Real>(bin_count,Scalar::Pi<Real>);
    
    Real total_curvature = 0;
    Real total_torsion = 0;
    Real total_squared_curvature = 0;
    Real total_squared_torsion = 0;
    
    Vector_T P;
    Vector_T Q;
    Vector_T u;
    
    P.Read( X, n - 3 );
    Q.Read( X, n - 2 );
    Vector_T v = (Q-P);
    v.Normalize();
    
    P = Q;
    Q.Read( X, n - 1 );
    Vector_T w = (Q-P);
    w.Normalize();
    
    // TODO: The two calls to std::atan2 in AngleBetweenUnitVectors make this quite slow.
    // TODO: Probably, we could use 2-vectorization here with a hand-made atan2 function here.
    
    for( Int i = 0; i < n; ++i )
    {
        u = v;
        v = w;
        P = Q;
        Q.Read( X, i );
        w = (Q-P);
        w.Normalize();
        
        // Hidden std::atan2 here.
        const Real curvature = AngleBetweenUnitVectors(v,w);
        
        Real torsion = 0;
        
        const Vector_T nu = Cross(u,v);
        const Vector_T mu = Cross(w,v);
        
        if( (nu.SquaredNorm() > Real(0)) && (mu.SquaredNorm() > Real(0)) )
        {
            torsion = std::atan2( Det(v,nu,mu), Dot(nu,mu) );
        }
        
        total_curvature += curvature;
        total_torsion   += Abs(torsion);
        
        total_squared_curvature += curvature * curvature;
        total_squared_torsion   += torsion * torsion;

        if constexpr ( histogramQ )
        {
            const Int curvature_bin = Clamp(
                static_cast<Int>( std::floor(curvature * scale) ),
                Int(0), bin_count - Int(1)
            );
            ++curvature_hist[curvature_bin];

            const Int torsion_bin = Clamp(
                static_cast<Int>( std::floor( (torsion + Scalar::Pi<Real>) * scale ) ),
                Int(0), Int(2) * bin_count - Int(1)
            );
            ++torsion_hist[torsion_bin];
        }
    }

    if constexpr ( totalsQ )
    {
        kv<t,appendQ>("Total Curvature",total_curvature);
        kv<t>("Total Squared Curvature",total_squared_curvature);
    }
    
    if constexpr ( histogramQ )
    {
        kv<t,totalsQ?appendQ:true>("Curvature Histogram",curvature_hist);
    }
    
    if constexpr ( totalsQ )
    {
        kv<t>("Total Torsion",total_torsion);
        kv<t>("Total Squared Torsion",total_squared_torsion);
    }
    
    if constexpr ( histogramQ )
    {
        kv<t>("Torsion Histogram",torsion_hist);
    }
}
