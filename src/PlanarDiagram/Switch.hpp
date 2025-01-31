public:

bool SwitchCrossing( const Int c )
{
    // We intentionally remove the "Unchanged" attribute.
    
    switch( C_state[c] )
    {
        case CrossingState::RightHanded:
        {
            C_state[c] = CrossingState::LeftHanded;
            return true;
        }
        case CrossingState::RightHandedUnchanged:
        {
            C_state[c] = CrossingState::LeftHanded;
            return true;
        }
        case CrossingState::LeftHanded:
        {
            C_state[c] = CrossingState::RightHanded;
            return true;
        }
        case CrossingState::LeftHandedUnchanged:
        {
            C_state[c] = CrossingState::RightHanded;
            return true;
        }
        case CrossingState::Inactive:
        {
            return false;
        }
        default:
        {
            eprint( ClassName()+"::SwitchCrossing: Value " + ToString(C_state[c]) + " is invalid" );
            return true;
        }
    }
}
