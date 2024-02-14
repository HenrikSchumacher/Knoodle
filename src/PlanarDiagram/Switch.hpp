bool Switch( const Int c )
{   
    if( C_state[c] == CrossingState::Positive)
    {
        C_state[c] = CrossingState::Negative;
        touched_crossings.push_back(c);
        return true;
    }
    else if( C_state[c] == CrossingState::Negative)
    {
        C_state[c] = CrossingState::Positive;
        touched_crossings.push_back(c);
        return true;
    }
    else
    {
        return false;
    }
}
