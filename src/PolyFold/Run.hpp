void Run()
{
    burn_in_time = 0;
    total_sampling_time = 0;
    total_analysis_time = 0;
    total_timing = 0;
    
    switch( verbosity )
    {
        case 1:  return Run_impl<0,1>();
    
        case 2:  return Run_impl<0,2>();
            
        default: return Run_impl<0,0>();
    }
} // Run

private:

template<Size_T tab_count = 0, int my_verbosity>
void Run_impl()
{
    try
    {
        T_run.Tic();
        
        BurnIn<tab_count+1,my_verbosity>();
        
        Sample<tab_count+1,my_verbosity>();
        
        T_run.Toc();
        
        total_timing = T_run.Duration();
        
        FinalReport<tab_count+1>();
        
    }
    catch( const std::exception & e )
    {
        std::ofstream file ( path / "Aborted_Polygon.txt" );
        file << PolygonString(x);
        throw;
    }
    
} // Run_impl
   
