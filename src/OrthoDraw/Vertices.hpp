public:

bool VertexActiveQ( const Int v ) const
{
    return V_flag[v] != VertexFlag_T::Inactive;
}

std::string VertexString( const Int v ) const
{
    return "crossing " + Tools::ToString(v) +" = { { "
        + Tools::ToString(V_dE(v,East )) + ", "
        + Tools::ToString(V_dE(v,North)) + " }, { "
        + Tools::ToString(V_dE(v,West )) + ", "
        + Tools::ToString(V_dE(v,South)) + " } }"
        + " (" + (VertexActiveQ(v) ? "Active" : "Inactive") + ")";
}


private:

// TODO: Add connectivity checks.
template<bool required_activity, bool verboseQ = true>
bool CheckVertex( const Int v ) const
{
//        logprint(MethodName("AssertVertex<"+ToString(required_activity)+">(" + VertexString(v) + ")"));
    
    if( v == Uninitialized )
    {
        if constexpr( required_activity )
        {
            if constexpr( verboseQ )
            {
                eprint(MethodName("AssertVertex<1>") +" " + VertexString(v) + " is not active.");
            }
            return false;
        }
        else
        {
            return true;
        }
    }
    
    if( !InIntervalQ(v,Int(0),V_dE.Dim(0)) )
    {
        if constexpr( verboseQ )
        {
            eprint(MethodName("AssertVertex<1>") +": vertex index " + ToString(v) + " is out of bounds.");
        }
        return false;
    }
    
    if( required_activity != VertexActiveQ(v) )
    {
        if constexpr( verboseQ )
        {
            eprint(MethodName("AssertVertex<" + ToString(required_activity)+">") + ": " + VertexString(v) + " is not " + ( required_activity ? "active" : "inactive") + ".");
        }
        return false;
    }

    return true;
}
