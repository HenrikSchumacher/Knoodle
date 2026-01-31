#ifdef PD_COUNTERS

mutable std::vector<Int> dual_arc_aggregator;
mutable std::vector<Int> initial_face_size_aggregator;
mutable std::vector<Int> face_size_aggregator;
mutable std::vector<Int> pre_strand_size_aggregator;
mutable std::vector<Int> post_strand_size_aggregator;

public:

void ResetCounters()
{
//    dual_arc_counter = 0;
//    face_counter     = 0;
//    total_face_size  = 0;
    
    dual_arc_aggregator.clear();
    face_size_aggregator.clear();
    initial_face_size_aggregator.clear();
    pre_strand_size_aggregator.clear();
    post_strand_size_aggregator.clear();
}

private:

void RecordDualArc( const Int de ) const
{
    dual_arc_aggregator.push_back(de);
}

public:

cref<std::vector<Int>> RecordedDualArcs() const
{
    return dual_arc_aggregator;
}

private:

void RecordFaceSize( const Int f_size ) const
{
    face_size_aggregator.push_back(f_size);
}

public:

cref<std::vector<Int>> RecordedFaceSizes() const
{
    return face_size_aggregator;
}

private:

void RecordInitialFaceSize( const Int f_size ) const
{
    initial_face_size_aggregator.push_back(f_size);
}

public:

cref<std::vector<Int>> RecordedInitialFaceSizes() const
{
    return initial_face_size_aggregator;
}

//double AverageFaceSize() const
//{
//    Size_T total_face_size = 0;
//    for( Size_T f_size : face_size_aggregator )
//    {
//        total_face_size += f_size;
//    }
//    
//    return Frac<double>(total_face_size,face_size_aggregator.size());
//}

private:

void RecordPreStrandSize( const Int s_size ) const
{

    pre_strand_size_aggregator.push_back(s_size);
}

public:

cref<std::vector<Int>> RecordedPreStrandSizes() const
{
    return pre_strand_size_aggregator;
}

private:

void RecordPostStrandSize( const Int s_size ) const
{
    post_strand_size_aggregator.push_back(s_size);
}

public:

cref<std::vector<Int>> RecordedPostStrandSizes() const
{
    return post_strand_size_aggregator;
}

#endif // PD_COUNTERS
