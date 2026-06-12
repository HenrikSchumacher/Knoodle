public:

template<IntQ T>
static PDC_T FromJenkinsCode(
    cptr<T> jenkins_code,
    const bool splitQ = true,
    const bool farfalle_to_anelliQ = true
)
{
    PDC_T pdc ( PD_T::FromJenkinsCode( jenkins_code ) );
    
    if( splitQ )
    {
        pdc.Split();
        if( farfalle_to_anelliQ ) { pdc.FarfalleToAnelli(); }
    }
    
    return pdc;
}


template<IntQ T>
Tensor1<T,Size_T> JenkinsCode()
{
    return ToSingleDiagram().template JenkinsCode<T>();
}

void ToJenkinsCodeFile( cref<std::filesystem::path> file ) const
{
    std::ofstream stream ( file );
    stream << ToSingleDiagram().ToJenkinsCodeString();
}

static PDC_T FromJenkinsCodeString( mref<InString> s, const bool splitQ = true )
{
    PDC_T pdc( PD_T::FromJenkinsCodeString(s) );
    if( splitQ ) { pdc.Split(); }
    return pdc;
}

static PDC_T FromJenkinsCodeFile( cref<std::filesystem::path> file, const bool splitQ = true )
{
    InString s ( file );
    
    if( s.FailedQ() || s.EmptyQ() )
    {
        eprint(MethodName("FromJenkinsCodeFile") + ": Reading failed.");
        return PDC_T();
    }
    
    return FromJenkinsCodeString( s, splitQ );
}
