template<bool io>
void SetMatchingPortTo( const Int c, const Int a, const Int b )
{
    mptr<Int> C = C_arcs.data(c,io);
    
    PD_ASSERT( (C[Left] == a) || (C[Right] == a) );
    
    C[C[Right] == a] = b;
}

void SetMatchingPortTo( const Int c, const bool io, const Int a, const Int b )
{
    mptr<Int> C = C_arcs.data(c,io);
    
    PD_ASSERT( (C[Left] == a) || (C[Right] == a) );
    
    C[C[Right] == a] = b;
}



/*!@brief Unplugs arc `a` from its head/tail and reconnects it to the head/tail of arc `b`, depending on the value of `headtail`. Depending on the value of `deactivateQ`, arc `b` will be deactivated or not. Mind that the other end of arc `b` might become dangling this way. It lies in the user's responsibility to take care of that.
 *
 * Arc `a` is assumed to be active, but the state of `b` can be anything.
 * (But the state `a` won't be checked explicitly for performance reasons and it is also not really exploited here in any way).
 *
 * @param a The arc that gets reconnected and that is to "survive" this operator.
 *
 * @param b The arc to be disconnected and that is to "die" in this operator.
 *
 * @param headtail If `headtail` is `true` (of `Head`), then the reconnection will happen at the heads of `a` and `b`; otherwise it will happen at their `tails.
 *
 * @tparam deactivateQ If set to to `true` (default), then arc `b` will be deactivated after the reconnection process.
 *
 * @tparam assertQ If set to `false`, then some error messages in debug mode (macro `PD_DEBUG` is defined) will be suppressed. As no effect whatsover if `PD_DEBUG` is undefined.
 */

template<bool deactivateQ = true, bool assertQ = true>
void Reconnect( const Int a, const bool headtail, const Int b )
{
    PD_DPRINT(std::string("Reconnect<")  + BoolString(deactivateQ) + "," + BoolString(assertQ) + ">( " + ArcString(a) + ", " + (headtail ? "Head" : "Tail") +  ", " + ArcString(b) + " )" );

    
    PD_ASSERT(a != b);
    
    PD_ASSERT(ArcActiveQ(a));
//    PD_assert( ArcActiveQ(b) ); // Could have been deactivated already.
    
    const Int c = A_cross(b,headtail);
    
    // This is a hack to suppress warnings in situations where we cannot guarantee that c is intact (but where we can guarantee that it will finally be deactivated.
    if constexpr ( assertQ )
    {
        AssertCrossing(c);
        
        PD_ASSERT(CheckArc(b));
        
        PD_ASSERT(CrossingActiveQ(A_cross(a, headtail)));
        PD_ASSERT(CrossingActiveQ(A_cross(a,!headtail)));
    }

    A_cross(a,headtail) = c;
    SetMatchingPortTo(c,headtail,b,a);
    
    DeactivateArc(b);
}


/*!@brief Unplugs arc `a` from its heads/tails and reconnects it to the head/tail of arc `b`, depending on the value of `headtail`. This is basically the same as `Reconnect( Int, bool, Int )`, except for the Boolean `headtail` is supplied as a template parameter for better compile time optimization.
 *
 * @param a The arc that gets reconnected and that is to "survive" this operator.
 *
 * @param b The arc to be disconnected and that is to "die" in this operator.
 *
 * @tparam headtail If `headtail` is `true` (of `PlanarDiagram<Int>::Head`), then the reconnection will happen at the heads of `a` and `b`; otherwise it will happen at their `tails.
 *
 * @tparam deactivateQ If set to to `true` (default), then arc `b` will be deactivated after the reconnection process.
 *
 *  @tparam assertQ If set to `false`, then some error messages in debug mode (macro `PD_DEBUG` is defined) will be suppressed. As no effect whatsover if `PD_DEBUG` is undefined.
 *
 */

template<bool headtail, bool deactivateQ = true, bool assertQ = true>
void Reconnect( const Int a, const Int b )
{
    // TODO: It is a bit annoying and violates the DRY principle that we have two functions `Reconnect` with almost 100% the same code. However, I made this intentionally so because I think that the compiler will optimize the two routines quite differently.
    
    PD_DPRINT(ClassName()+"::Reconnect<" + (headtail ? "Head" : "Tail") +  "," + BoolString(deactivateQ) + "," + BoolString(assertQ) + ">( " + ArcString(a) + ", " + ArcString(b) + " )" );
    
    PD_ASSERT(a != b);
    PD_ASSERT(ArcActiveQ(a));
    
    const Int c = A_cross(b,headtail);
    
    // This is a hack to suppress warnings in situations where we cannot guarantee that c is intact (but where we can guarantee that it will finally be deactivated.
    if constexpr ( assertQ )
    {
        AssertCrossing(c);
        
        PD_ASSERT(CheckArc(b));
        
        PD_ASSERT(CrossingActiveQ(A_cross(a, headtail)));
        PD_ASSERT(CrossingActiveQ(A_cross(a,!headtail)));
    }

    A_cross(a,headtail) = c;
    SetMatchingPortTo<headtail>(c,b,a);

    if constexpr( deactivateQ )
    {
        DeactivateArc(b);
    }
}
