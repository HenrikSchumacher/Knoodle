public:

Int TredgeCount() const
{
    return TRE_count;
}

Int TrdedgeCount() const
{
    return Int(2) * TRE_count;
}

cref<EdgeContainer_T> Tredges() const
{
    return TRE_V;
}

cref<EdgeContainer_T> TredgeTrfaces() const
{
    return TRE_TRF;
}

cref<FlagContainer_T> TredgeFlags() const
{
    return TRE_flag;
}
