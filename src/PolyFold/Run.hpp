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
                return Run_impl<0,1,true>();
            }
            else
            {
                return Run_impl<0,1,false>();
            }
            break;
        }
        case 2:
        {
            if( do_checksQ )
            {
                return Run_impl<0,2,true>();
            }
            else
            {
                return Run_impl<0,2,false>();
            }
            break;
        }
        default:
        {
            if( do_checksQ )
            {
                return Run_impl<0,0,true>();
            }
            else
            {
                return Run_impl<0,0,false>();
            }
            break;
        }
    }
} // Run

private:

template<Size_T tab_count = 0, int my_verbosity, bool checksQ>
int Run_impl()
{
    T_run.Tic();
    
    BurnIn<tab_count+1,my_verbosity,checksQ>();
    
    int err = Sample<tab_count+1,my_verbosity,checksQ>();
    
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
   
