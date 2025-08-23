
// TODO: Statistics about bounding boxes.

private:

std::string PDCodeString( mref<PD_T> P ) const
{
    if( P.CrossingCount() <= 0 )
    {
        return std::string();
    }
    if( tally_trefoilsQ && P.ProvenTrefoilQ() )
    {
        return std::string();
    }
    if( tally_F8Q && P.ProvenFigureEightQ() )
    {
        return std::string();
    }
    
    auto pdcode = P.PDCode();
    
    cptr<Int> code = pdcode.data();
    
    std::string s;
    s+= "\ns ";
    s+= ToString(P.ProvenMinimalQ());
    
    const Int c_count = pdcode.Dim(0);
    
    for( Int c = 0; c < c_count; ++c )
    {
        s+= VectorString<5>(&code[5 * c + 0], "\n", "\t", "" );
    }
    
    return s;
}

std::string GaussCodeString( mref<PD_T> P ) const
{
    if( P.CrossingCount() <= 0 )
    {
        return std::string();
    }
    if( tally_trefoilsQ && P.ProvenTrefoilQ() )
    {
        return std::string();
    }
    if( tally_F8Q && P.ProvenFigureEightQ() )
    {
        return std::string();
    }
    
    std::string s;
    s+= "\ns ";
    s+= ToString(P.ProvenMinimalQ());
    s+= " | ";
    auto gauss_code = P.ExtendedGaussCode();
    
    s+= VectorString( gauss_code.data(), "", " ", "", gauss_code.Size() );
    
    return s;
}

std::string MacLeodCodeString( mref<PD_T> P ) const
{
    if( P.CrossingCount() <= 0 )
    {
        return std::string();
    }
    if( tally_trefoilsQ && P.ProvenTrefoilQ() )
    {
        return std::string();
    }
    if( tally_F8Q && P.ProvenFigureEightQ() )
    {
        return std::string();
    }
    
    std::string s;
    s+= "\ns ";
    s+= ToString(P.ProvenMinimalQ());
    s+= " | ";
    auto macleod_code = P.ShortMacLeodCode();
    
    s+= VectorString( macleod_code.data(), "", " ", "", macleod_code.Size() );
    
    return s;
}

void WriteUnknots()
{
    if( pdQ )
    {
        pd_stream << "u " << unknot_counter << "\n";
        pd_stream << std::flush;
    }
    if( gaussQ )
    {
        gauss_stream << "u " << unknot_counter << "\n";
        gauss_stream << std::flush;
    }
    if( macleodQ )
    {
        macleod_stream << "u " << unknot_counter << "\n";
        macleod_stream << std::flush;
    }
}

std::string TrefoilString() const
{
    std::string s;
    if( T_p_counter > LInt(0) )
    {
        s += "\nT+ ";
        s += ToString(T_p_counter);
    }
    if( T_m_counter > LInt(0) )
    {
        s += "\nT- ";
        s += ToString(T_m_counter);
    }
    return s;
}

std::string FigureEightString() const
{
    std::string s;
    if( F8_counter > LInt(0) )
    {
        s += "\nF8 ";
        s += ToString(F8_counter);
    }
    return s;
}



template<Size_T t0, int my_verbosity>
void Analyze( const LInt i )
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    
    constexpr bool V1Q = my_verbosity >= 1;
    constexpr bool V2Q = my_verbosity >= 2;
    
    std::string s;
    
    TimeInterval T_link;
    TimeInterval T_intersection;
    TimeInterval T_delete;
    TimeInterval T_link_dealloc;
    TimeInterval T_pd;
    TimeInterval T_simplify;
    TimeInterval T_pd_write;
    TimeInterval T_gauss_write;
    TimeInterval T_macleod_write;
    TimeInterval T_pd_dealloc;
    TimeInterval T_gyradius;
    TimeInterval T_curvature_torsion;
    
    
    bool snapshotQ = (print_ctr >= steps_between_print) || ( (steps_between_print >= LInt(0)) && (i == N));
    
    if( snapshotQ )
    {
        PolygonSnapshot<t1>(i);
    }
    
    TimeInterval T_analysis (0);
    
    
    T_curvature_torsion.Tic<V2Q>();
    if( bin_count > Int(0) )
    {
        PrintCurvatureTorsion<t1,true,true>( x.data() );
    }
    else
    {
        if( anglesQ )
        {
            PrintCurvatureTorsion<t1,true,false>( x.data() );
        }
    }
    T_curvature_torsion.Toc<V2Q>();

    
    if( squared_gyradiusQ )
    {
        T_gyradius.Tic<V2Q>();
        Real g = SquaredGyradius(x.data());
        T_gyradius.Toc<V2Q>();
        kv<t1>("Squared Gyradius",g);
    }
    
    if( bounding_boxesQ )
    {
        Tiny::Vector<AmbDim,Real,Int> lower (x,0);
        Tiny::Vector<AmbDim,Real,Int> upper (x,0);
        Tiny::Vector<AmbDim,Real,Int> u;
        
        const Int j_count = x.Dim(0);
        
        for( Int j = 0; j < j_count; ++j )
        {
            u.Read(x,j);
            lower.ElementwiseMin(u);
            upper.ElementwiseMax(u);
        }
        
        kv<t1>("Bounding Box Lower Corner",lower);
        kv<t1>("Bounding Box Upper Corner",upper);
    }
    
    if( pdQ || gaussQ || macleodQ )
    {
        T_link.Tic<V2Q>();
        Link_T L ( n );

        // Read coordinates into `Link_T` object `L`...
        L.ReadVertexCoordinates ( x.data() );
        T_link.Toc<V2Q>();
        allocation_time += T_link.Duration();
        
        T_intersection.Tic<V2Q>();
        
        int err = L.template FindIntersections<true>();
        
        T_intersection.Toc<V2Q>();
        
        const IntersectionFlagCounts_T intersection_flag_counts = L.IntersectionFlagCounts();
        
        acc_intersec_counts += intersection_flag_counts;
        
        if( (err != 0) || V1Q )
        {
            log << ",\n" + ct_tabs<t1> + "\"Link\" -> <|";
                kv<t2,0>("Byte Count", L.ByteCount() );
            if( (err != 0) || V2Q )
            {
                log << ",\n" + ct_tabs<t2> + "\"Byte Count Details\" -> ";
                log << L.template AllocatedByteCountDetails<t2>();
                
                PrintIntersectionFlagCounts<t2>(
                    "Intersection Flag Counts", intersection_flag_counts
                );
                
                PrintIntersectionFlagCounts<t2>(
                    "Accumulated Intersection Flag Counts", acc_intersec_counts
                );
            }
            log << "\n" + ct_tabs<t1> + "|>";
        }
        
        // TODO: This lets the simulation continue if closeby intersection times have been detected. Should should make this check more robust in the future.
        if( (err != 0) && (err != 8) )
        {
            kv<t1>("FindIntersections Error Flag", err);
            log << std::flush;
            throw std::runtime_error(ClassName()+"::Analyze(" + ToString(i) + "): Error in creating the planar diagram.");
        }
        
        // Deallocate tree-related data in L to make room for the PlanarDiagram.
        if( force_deallocQ )
        {
            T_delete.Tic<V2Q>();
            L.DeleteTree();
            T_delete.Toc<V2Q>();
            deallocation_time += T_delete.Duration();
        }
        
        T_pd.Tic<V2Q>();
        // We delay the allocation until substantial parts of L have been deallocated.
        PD_T PD ( L );
        T_pd.Toc<V2Q>();
    
        // Delete remainder of L to make room for the simplification.
        if( force_deallocQ )
        {
            T_link_dealloc.Tic<V2Q>();
            L = Link_T();
            T_link_dealloc.Toc<V2Q>();
            deallocation_time += T_link_dealloc.Duration();
        }
    
        if constexpr ( V1Q )
        {
            log << ",\n" + ct_tabs<t1> + "\"PlanarDiagram\" -> <|";
            kv<t2,0>("Byte Count (Before Simplification)", PD.ByteCount() );
            kv<t2>("Crossing Count (Before Simplification)", PD.CrossingCount() );
            log << std::flush;
        }
        
        std::vector<PD_T> pd_list;
        
        T_simplify.Tic<V2Q>();
        PD.Simplify5( pd_list );
        pd_list.push_back(std::move(PD));
        T_simplify.Toc<V2Q>();
        

        Size_T byte_count  = 0;
        Int crossing_count = 0;
        
        if( pdQ || gaussQ || macleodQ )
        {
            T_p_counter = 0;
            T_m_counter = 0;
            F8_counter  = 0;
            
            for( auto & P : pd_list )
            {
                byte_count     += P.ByteCount();
                crossing_count += P.CrossingCount();
                
                if( tally_trefoilsQ && P.ProvenTrefoilQ() )
                {
                    if( P.CrossingRightHandedQ(Int(0)) )
                    {
                        ++T_p_counter;
                    }
                    else
                    {
                        ++T_m_counter;
                    }
                }
                if( tally_F8Q && P.ProvenFigureEightQ() )
                {
                    ++F8_counter;
                }
            }
        }
        else
        {
            for( auto & P : pd_list )
            {
                byte_count     += P.ByteCount();
                crossing_count += P.CrossingCount();
            }
        }
                
        
        if constexpr ( V2Q )
        {
            kv<t2>("Byte Count (After Simplification)", byte_count );
            kv<t2>("Crossing Count (After Simplification)", crossing_count );
        }
        
        if constexpr ( V1Q )
        {
            log << "\n" + ct_tabs<t1> + "|>";
            log << std::flush;
        }
        
        if( !tally_unknotsQ || (crossing_count > Int(0)) )
        {
            if( tally_unknotsQ && (unknot_counter > LInt(0)) )
            {
                WriteUnknots();
                unknot_counter = 0;
            }
            
            if( pdQ )
            {
                if constexpr ( V2Q ) { T_pd_write.Tic(); }
                
                // Writing the PD codes to file.
                pd_stream << "k";
                if( tally_trefoilsQ )
                {
                    pd_stream << TrefoilString();
                }
                if( tally_F8Q )
                {
                    pd_stream << FigureEightString();
                }
                for( auto & P : pd_list )
                {
                    pd_stream << PDCodeString(P);
                }
                
                pd_stream << "\n";
                pd_stream << std::flush;
                if( !pd_stream )
                {
                    throw std::runtime_error(ClassName()+"::Analyze(" + ToString(i) + "): Failed to write to file \"" + pd_file.string() + "\".");
                }
                
                if constexpr ( V2Q ) { T_pd_write.Toc(); }
            }
            
            if ( gaussQ )
            {
                if constexpr ( V2Q ) { T_gauss_write.Tic(); }
                
                // Writing the PD codes to file.
                gauss_stream << "k";
                if( tally_trefoilsQ )
                {
                    gauss_stream << TrefoilString();
                }
                if( tally_F8Q )
                {
                    gauss_stream << FigureEightString();
                }
                for( auto & P : pd_list )
                {
                    gauss_stream << GaussCodeString(P);
                }
                gauss_stream << "\n";
                gauss_stream << std::flush;
                if( !gauss_stream )
                {
                    throw std::runtime_error( ClassName()+"::Analyze(" + ToString(i) + "): Failed to write to file \"" + gauss_file.string() + "\".");
                }
                
                if constexpr ( V2Q ) { T_gauss_write.Toc(); }
            }
            
            if ( macleodQ )
            {
                if constexpr ( V2Q ) { T_macleod_write.Tic(); }
                
                // Writing the PD codes to file.
                macleod_stream << "k";
                if( tally_trefoilsQ )
                {
                    macleod_stream << TrefoilString();
                }
                if( tally_F8Q )
                {
                    macleod_stream << FigureEightString();
                }
                for( auto & P : pd_list )
                {
                    macleod_stream << MacLeodCodeString(P);
                }
                macleod_stream << "\n";
                macleod_stream << std::flush;
                if( !macleod_stream )
                {
                    throw std::runtime_error( ClassName()+"::Analyze(" + ToString(i) + "): Failed to write to file \"" + macleod_file.string() + "\".");
                }
                
                if constexpr ( V2Q ) { T_macleod_write.Toc(); }
            }
        }
        else
        {
            ++unknot_counter;
        }
        
        if( force_deallocQ )
        {
            T_pd_dealloc.Tic<V2Q>();
            PD = PD_T();
            pd_list = std::vector<PD_T>();
            T_pd_dealloc.Toc<V2Q>();
            deallocation_time += T_pd_dealloc.Duration();
        }
    }
    
    T_analysis.Toc();
    
    total_analysis_time += T_analysis.Duration();
    
    if constexpr ( V1Q )
    {
        kv<t1>("Analysis Seconds Elapsed",T_analysis.Duration());
    }
    
    if constexpr ( V2Q )
    {
        log << ",\n" + ct_tabs<t1> + "\"Analysis Time Details\" -> <|";
        
                kv<t2,0>("Curvature/Torsion Time Elapsed", T_curvature_torsion.Duration() );
        
        if( squared_gyradiusQ )
        {
                kv<t2>("Compute Squared Gyradius", T_gyradius.Duration());
        }
        
        if( pdQ || gaussQ )
        {
                kv<t2>("Create Link", T_link.Duration());
                kv<t2>("Compute Intersections",T_intersection.Duration());
            if( force_deallocQ )
            {
                kv<t2>("Deallocate Link", T_delete.Duration() + T_link_dealloc.Duration());
            }
                kv<t2>("Create PlanarDiagram", T_pd.Duration());
                kv<t2>("Simplify PlanarDiagram", T_simplify.Duration());
            if( force_deallocQ )
            {
                kv<t2>("Deallocate PlanarDiagram",T_pd_dealloc.Duration());
            }
            if( pdQ )
            {
                kv<t2>("Write PD Code", T_pd_write.Duration());
            }
            if( gaussQ )
            {
                kv<t2>("Write Gauss Code", T_gauss_write.Duration());
            }
            if( macleodQ )
            {
                kv<t2>("Write MacLeod Code", T_macleod_write.Duration());
            }
        }
        log << "\n" + ct_tabs<t1> + "|>";
    }

} // Analyze
