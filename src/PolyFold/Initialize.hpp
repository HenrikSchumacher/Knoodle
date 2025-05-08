private:

template<Size_T t0>
void Initialize()
{
    constexpr Size_T t1 = t0 + 1;
    constexpr Size_T t2 = t0 + 2;
    constexpr Size_T t3 = t0 + 3;
    constexpr Size_T t4 = t0 + 4;
    
    TimeInterval T_init(0);
    
    Size_T x_byte_count   = 0;
    Size_T T_byte_count   = 0;
    Size_T L_byte_count   = 0;
    Size_T PD_byte_count  = 0;
    
    log_file = path / "Info.m";
    log.open( log_file, std::ios_base::out );
    
    if( !log )
    {
        throw std::runtime_error( 
            ClassName() + "::Initialize: Failed to create file \"" + log_file.string() + "\"."
        );
    }
    
    if( pdQ )
    {
        pd_file = path / "PDCodes.tsv";
        pd_stream.open( pd_file, std::ios_base::out );
        
        if( !pd_stream )
        {
            throw std::runtime_error(
                ClassName() + "::Initialize: Failed to create file \"" + pd_file.string() + "\"."
            );
        }
    }
    
    if( gaussQ )
    {
        gauss_file = path / "GaussCodes.txt";
        gauss_stream.open( gauss_file, std::ios_base::out );
        
        if( !gauss_stream )
        {
            throw std::runtime_error(
                ClassName() + "::Initialize: Failed to create file \"" + gauss_file.string() + "\"."
            );
        }
    }
    
    if constexpr ( Clisby_T::witnessesQ )
    {
        witness_file = path / "Witnesses.tsv";
        witness_stream.open( witness_file, std::ios_base::out );
        
        if( !witness_stream )
        {
            throw std::runtime_error(
                ClassName() + "::Initialize: Failed to create file \"" + witness_file.string() + "\"."
            );
        }
        
        witness_stream << "Pivot 0" << "\t" << "Pivot 1" << "\t" << "Witness 0" << "\t" << "Witness 1\n";
        
        pivot_file = path / "AcceptedPivotMoves.tsv";
        pivot_stream.open( pivot_file, std::ios_base::out );
        
        if( !pivot_stream )
        {
            throw std::runtime_error(
                ClassName() + "::Initialize: Failed to create file \"" + pivot_file.string() + "\"."
            );
        }
        
        pivot_stream   << "Pivot 0" << "\t" << "Pivot 1" << "\t" << "Angle\n";
    }
    
    log << ct_tabs<t0> + "<|";
    
    kv<t1,0>("Prescribed Edge Length",prescribed_edge_length);
    kv<t1>  ("Hard Sphere Diameter",hard_sphere_diam);
    kv<t1>  ("Edge Count",n);
    kv<t1>  ("Sample Count",N);
    kv<t1>  ("Burn-in Count",burn_in_accept_count);
    kv<t1>  ("Skip Count",skip);
    kv<t1>  ("Reflection Probability",reflection_probability);
    kv<t1>  ("Hierarchical Moves",BoolString(hierarchicalQ));
    kv<t1>  ("Shift Indices",BoolString(shiftQ));
    kv<t1>  ("Recenter",BoolString(recenterQ));
    kv<t1>  ("Collision Checks",BoolString(checksQ));
    kv<t1>  ("Verbosity",verbosity);
    
    log << ",\n" + ct_tabs<t1> + "\"Initialization\" -> <|";

    kv<t2,0>("Git Hash",
#ifdef GIT_VERSION
        GIT_VERSION
#else
        "\"unknown\""
#endif
    );
    
    log << ",\n" + ct_tabs<t2> + "\"PolyFold\" -> <|";
        kv<t3,0>("Class",ClassName());
        kv<t3>("Vector Extensions Enabled",vec_enabledQ);
        kv<t3>("Matrix Extensions Enabled",mat_enabledQ);
        kv<t3>("Forced Deallocation Enabled",force_deallocQ);
    log << "\n" + ct_tabs<t2> + "|>";
    
    log << ",\n" + ct_tabs<t2> + "\"Coordinate Buffer\" -> <|";
    
        x_byte_count = x.ByteCount();
        kv<t3,0>("Byte Count", x.ByteCount() );
    log << "\n" + ct_tabs<t2> + "|>";
    
    log << std::flush;

    log << ",\n" + ct_tabs<t2> + "\"Clisby Tree\" -> <|";
    
    kv<t3,0>("Class",Clisby_T::ClassName());
    log << std::flush;
    
    {
        Clisby_T T ( n, hard_sphere_diam );
        
        if( inputQ )
        {
            
            T.ReadVertexCoordinates(x.data());
            
            e_dev = T.MinMaxEdgeLengthDeviation( x.data() );
            
            const Real error = Max(Abs(e_dev.first),Abs(e_dev.second));

            kv<t3>("(Smallest Edge Length)/(Prescribed Edge Length) - 1", e_dev.first );
            kv<t3>("(Greatest Edge Length)/(Prescribed Edge Length) - 1", e_dev.second );
            
            // TODO: Check that loaded polygon has edgelengths close to 1.
            if( error > edge_length_tolerance )
            {
                throw std::runtime_error(ClassName() + "::Initialize: Relative edge length deviation of loaded polygon " + ToStringFPGeneral(error) + " is greater than tolerance " + ToStringFPGeneral(edge_length_tolerance) + ".");
            }
            
            if( checksQ )
            {
                if( T.template CollisionQ<true>() )
                {
                    kv<t3>("Hard Sphere Constraint Satisfied", "False" );
                    
                    throw std::runtime_error(ClassName() + "::Initialize: Loaded polygon does not satisfy the hard sphere constraint with diameter" + ToString(hard_sphere_diam) + ".");
                }
                else
                {
                    kv<t3>("Hard Sphere Constraint Satisfied", "True" );
                }
            }
        }
        
        prng = T.RandomEngine();
        
        PRNG_State_T state = State( prng );

        if( !prng_multiplierQ )
        {
            prng_init.multiplier = state.multiplier;
        }
        
        if( !prng_incrementQ )
        {
            prng_init.increment = state.increment;
        }
        
        if( !prng_stateQ )
        {
            prng_init.state = state.state;
        }
        
        if( prng_multiplierQ || prng_incrementQ || prng_stateQ)
        {
           if( SetState( prng, prng_init ) )
           {
               throw std::runtime_error( ClassName() + "::Initialize: Failed to initialize random engine with <|"
                      + "Multiplier -> " + prng_init.multiplier + ", "
                      + "Increment -> "  + prng_init.increment + ", "
                      + "State -> "      + prng_init.state + "|>."
               );
           }
        }
        
        T_byte_count = T.ByteCount();
        kv<t3>("Byte Count", T.ByteCount() );
        log << ",\n" + ct_tabs<t3> + "\"Byte Count Details\" -> ";
        log << T.template AllocatedByteCountDetails<t3>();
        
        kv<t3>("Euclidean Transformation Class",Clisby_T::Transform_T::ClassName());
        log << ",\n" + ct_tabs<t3> + "\"PCG64\" -> <|";
            kv<t4,0>("Multiplier", prng_init.multiplier);
            kv<t4>("Increment"   , prng_init.increment );
            kv<t4>("State"       , prng_init.state     );
        log << "\n" + ct_tabs<t3> + "|>";
        log << "\n" + ct_tabs<t2> + "|>";
        
        log << std::flush;
        
        T.WriteVertexCoordinates( x.data() );
        
        // Remember random engine for later use.
        prng = T.RandomEngine();
        
        if( force_deallocQ )
        {
            T = Clisby_T();
        }
    }

    {
        Link_T L ( n );

        L.ReadVertexCoordinates ( x.data() );

        log << ",\n" + ct_tabs<t2> + "\"Link\" -> <|";
            kv<t3,0>("Class", L.ClassName());
        log << std::flush;
        
        (void)L.FindIntersections();
        
            L_byte_count = L.ByteCount();
            kv<t3>("Byte Count", L.ByteCount() );
            log << ",\n" + ct_tabs<t3> + "\"Byte Count Details\" -> ";
            log << L.template AllocatedByteCountDetails<t3>();
        
        log << "\n" + ct_tabs<t2> + "|>";
        log << std::flush;

        // Deallocate tree-related data in L to make room for the PlanarDiagram.
        if( force_deallocQ )
        {
            L.DeleteTree();
        }

        log << ",\n" + ct_tabs<t2> + "\"Planar Diagram\" -> <|";
            kv<t3,0>("Class", PD_T::ClassName());
        
        // We delay the allocation until substantial parts of L have been deallocated.
        PD_T PD ( L );

        if( force_deallocQ )
        {
            L = Link_T();
        }
        PD_byte_count = PD.ByteCount();
            kv<t3>("Byte Count (Before Simplification)", PD.ByteCount() );
        log << "\n" + ct_tabs<t2> + "|>";
        log << std::flush;

        if( force_deallocQ )
        {
            PD = PD_T();
        }
    }
    
    T_init.Toc();
            
    kv<t2>("Initialization Seconds Elapsed", T_init.Duration() );

    log << "\n" + ct_tabs<t1> + "|>"  << std::flush;
    
    acc_intersec_counts.SetZero();
    
    
    Size_T max_byte_count = 0;
    max_byte_count += x_byte_count;
    max_byte_count += T_byte_count;
    max_byte_count += L_byte_count;
    max_byte_count += PD_byte_count;
    max_byte_count += curvature_hist.ByteCount();
    max_byte_count += torsion_hist.ByteCount();
    
    // Give 10% buffer to these calculcations.
    max_byte_count = static_cast<Size_T>(max_byte_count * 1.1);
    
    Size_T expected_byte_count = 0;
    expected_byte_count += x_byte_count;
    expected_byte_count += Max( T_byte_count, L_byte_count + PD_byte_count);
    expected_byte_count += curvature_hist.ByteCount();
    expected_byte_count += torsion_hist.ByteCount();
    
    // Give 10% buffer to these calculcations.
    expected_byte_count = static_cast<Size_T>(expected_byte_count * 1.1);
    
    print("Initialization done.");
    valprint<a>("Maximum Byte Count", max_byte_count);
    valprint<a>("Expected Byte Count", force_deallocQ ? expected_byte_count : max_byte_count);
    
    if( !force_deallocQ )
    {
        print("Run with command-line option \"-m\" to run with forced deallocation to reduce Expected Byte Count to " + ToString(expected_byte_count) + ".");
    }
    
    print("");
}
