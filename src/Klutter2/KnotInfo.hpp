public:

Path_T KnotInfoNameFile() const
{
    return working_directory / ("KnotNames_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

Path_T KnotInfoPDCodeFile() const
{
    return working_directory / ("KnotPDCodes_" + StringWithLeadingZeroes(crossing_count,Size_T(2)) + ".txt");
}

void LoadKnotInfo()
{
    [[maybe_unused]] auto tag = [](){ return MethodName("LoadKnotInfo"); };
    
    if( knot_info_loadedQ )
    {
        wprint(tag() + ": KnotInfo data has already been loaded. Aborting.");
        return;
    }
    
    names.clear();
    subklutters.clear();
    I_S.clear();
    max_name_size = 0;
    Classifier classifier;
    
    Path_T name_file = KnotInfoNameFile();
    print(tag() + ": Loading KnotInfo's knot names from file \"" + name_file.c_str() + "\"." );
    
    std::ifstream name_stream ( name_file, std::ios::binary );
    if( !name_stream )
    {
        eprint(tag() + ": Could not open " + name_file.string() +". Aborting with incomplete table." );
        return;
    }
    
    Path_T pd_code_file = KnotInfoPDCodeFile();
    print(tag() + ": Loading KnotInfo's pd codes from file \"" + ToString(pd_code_file.c_str()) + "\"." );
    
    Tools::InString s (pd_code_file);
    if( s.EmptyQ() )
    {
        eprint(tag() + ": File  " + pd_code_file.string() + " is empty.");
        failedQ = true;
        return;
    }
    
    Tensor2<Int,Int> pd_code ( crossing_count, Int(4) );
    Name_T name;
    ID_T   id = 0;
    
//    tic("Reading inputs (sequential)");
    while( name_stream && !s.EmptyQ() && !s.FailedQ() )
    {
        name_stream >> name;
        max_name_size = std::max(max_name_size,name.size());
        names.push_back(name);
        s.TakeMatrixFunction(pd_code.WriteAccess(),crossing_count,Int(4), "","\n","\n", ""," ","");
        
        if( s.FailedQ() )
        {
            eprint(tag() + ": Reading pd code no. " + ToString(id) + " from file \"" + pd_code_file.c_str() + "\" failed. Current position = " + ToString(s.Position()) + "; current character code = " + ToString(static_cast<int>(s.CurrentChar())) + "."
            );
            failedQ = true;
            return;
        }
        
        PD_T pd = PD_T::template FromPDCode<{.signQ = false,.colorQ = false}>(
            pd_code.data(), crossing_count
        );
        
        if( pd.InvalidQ() )
        {
            eprint(tag() + ": Diagram no. " + ToString(id) + " is invalid. Aborting."
            );
            failedQ = true;
            return;
        }
        
        const Invariant_T invariant = classifier(pd);
        
        if( !I_S.contains(invariant) )
        {
            I_S[invariant] = subklutters.size();
            subklutters.emplace_back(crossing_count);
            subklutters.back().InsertIdentifiedKey(ToKey(pd),id);
        }
        else
        {
            subklutters[I_S[invariant]].InsertIdentifiedKey(ToKey(pd),id);
        }
        
        ++id;
        
        if( s.EmptyQ() ) { break; }
        
        s.SkipChar('\n');
    }
//    toc("Reading inputs (sequential)");
    
    knot_info_loadedQ = true;
}
