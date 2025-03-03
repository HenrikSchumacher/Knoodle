public:

static PRNG_State_T State( cref<PRNG_T> prng )
{
    std::stringstream s;

    s << prng;

    PRNG_State_T full_state;

    s >> full_state.multiplier;
    s >> full_state.increment;
    s >> full_state.state;

    return full_state;
}


static bool SetState( mref<PRNG_T> prng, cref<PRNG_State_T> state )
{
    std::stringstream s ( state.multiplier + " " + state.increment + " " + state.state );
    
    s >> prng;
    
    return s.fail();
}
