public:

bool Switch( const Int c )
{
    if( C_state[c] == CrossingState::RightHanded)
    {
        C_state[c] = CrossingState::LeftHanded;
        touched_crossings.push_back(c);
        return true;
    }
    else if( C_state[c] == CrossingState::LeftHanded)
    {
        C_state[c] = CrossingState::RightHanded;
        touched_crossings.push_back(c);
        return true;
    }
    else
    {
        return false;
    }
}
