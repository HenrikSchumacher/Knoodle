public:

void RecomputeArcState( Int a )
{
    const Int  c_0 = A_cross          (a,Tail);
    const bool s_0 = ArcSide_Reference(a,Tail);
    const Int  c_1 = A_cross          (a,Head);
    const bool s_1 = ArcSide_Reference(a,Head);

    ArcState_T a_state = ArcState_T::Inactive();
    a_state.Set(Tail,s_0,C_state[c_0]);
    a_state.Set(Head,s_1,C_state[c_1]);
    A_state[a] = a_state;
}

void RecomputeArcStates()
{
    TOOLS_PTIMER(timer,MethodName("RecomputeArcStates"));

    for( Int c = 0; c < MaxCrossingCount(); ++c )
    {
        if( CrossingActiveQ(c) )
        {
            Int A [2][2];

            copy_buffer<4>( C_arcs.data(c), &A[0][0] );

            const CrossingState_T c_state = C_state[c];

            /* A[Out][Left ]         A[Out][Right]
             *               O     O
             *                ^   ^
             *                 \ /
             *                  X c
             *                 ^ ^
             *                /   \
             *               O     O
             * A[In ][Left ]         A[In ][Right]
             */

            A_state[A[Out][Left ]].Set(Tail,Left ,c_state);
            A_state[A[Out][Right]].Set(Tail,Right,c_state);
            A_state[A[In ][Left ]].Set(Head,Left ,c_state);
            A_state[A[In ][Right]].Set(Head,Right,c_state);
        }
    }
}
