public:

bool Switch( const Int c )
{
    if( RightHandedQ(c) )
    {
        C_state[c] = CrossingState::LeftHanded;
        touched_crossings.push_back(c);
        return true;
    }
    else if( LeftHandedQ(c) )
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
