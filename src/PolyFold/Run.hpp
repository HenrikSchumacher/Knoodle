int Run()
{
    burn_in_time = 0;
    total_sampling_time = 0;
    total_analysis_time = 0;
    total_timing = 0;
    
    switch( verbosity )
    {
        case 1:
        {
            if( do_checksQ )
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,1,true,true>();
                }
                else
                {
                    return Run_impl<0,1,false,true>();
                }
            }
            else
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,1,true,false>();
                }
                else
                {
                    return Run_impl<0,1,false,false>();
                }
                    
            }
            break;
        }
        case 2:
        {
            if( do_checksQ )
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,2,true,true>();
                }
                else
                {
                    return Run_impl<0,2,false,true>();
                }
            }
            else
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,2,true,false>();
                }
                else
                {
                    return Run_impl<0,2,false,false>();
                }
            }
            break;
        }
        default:
        {
            if( do_checksQ )
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,0,true,true>();
                }
                else
                {
                    return Run_impl<0,0,false,true>();
                }
            }
            else
            {
                if( allow_reflectionsQ )
                {
                    return Run_impl<0,0,true,false>();
                }
                else
                {
                    return Run_impl<0,0,false,false>();
                }
            }
            break;
        }
    }
} // Run

private:

template<Size_T tab_count = 0, int my_verbosity, bool reflectionsQ, bool checksQ>
int Run_impl()
{
    T_run.Tic();
    
    BurnIn<tab_count+1,my_verbosity,reflectionsQ,checksQ>();
    
    int err = Sample<tab_count+1,my_verbosity,reflectionsQ,checksQ>();
    
    T_run.Toc();
    
    total_timing = T_run.Duration();
    
    FinalReport<tab_count+1>();
    
    if( err )
    {
        eprint(ClassName() + "::Run: Aborted because of error flag " + ToString(err) + ".");
        
        std::ofstream file ( path / "Aborted_Polygon.txt" );
        
        file << PolygonString(x);
    }
    
    return err;
    
} // Run_impl
   
