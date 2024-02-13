bool Switch( const Int c )
{   
    if( C_state[c] == Crossing_State::Positive)
    {
        C_state[c] = Crossing_State::Negative;
        touched_crossings.push_back(c);
        return true;
    }
    else if( C_state[c] == Crossing_State::Negative)
    {
        C_state[c] = Crossing_State::Positive;
        touched_crossings.push_back(c);
        return true;
    }
    else
    {
        return false;
    }
}
