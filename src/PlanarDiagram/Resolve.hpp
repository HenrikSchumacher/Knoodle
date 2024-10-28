public:

bool ResolveCrossing( const Int c )
{
    PD_PRINT(ClassName()+"::ResolveCrossing");
    PD_VALPRINT("c",c);
    
    PD_ASSERT(CrossingActiveQ(c));
    
    if( CrossingActiveQ(c) )
    {

//           O----<----O       O---->----O
//               e_0    ^     ^    e_1
//                       \   /
//                        \ /
//                       c X
//                        / \
//                       /   \
//               e_2    /     \    e_3
//           O---->----O       O----<----O

        const Int e_0 = C_arcs(c,Out,Left );
        const Int e_1 = C_arcs(c,Out,Right);
        const Int e_2 = C_arcs(c,In ,Left );
        const Int e_3 = C_arcs(c,In ,Right);
//        
//        valprint("c  ",CrossingString(c));
//        valprint("e_0",ArcString(e_0));
//        valprint("e_1",ArcString(e_1));
//        valprint("e_2",ArcString(e_2));
//        valprint("e_3",ArcString(e_3));
        
        if( e_0 == e_2 )
        {
            ++unlink_count;
            DeactivateArc(e_0);
        }
        else
        {
            Reconnect<Head>(e_2,e_0);
        }
        
        
        if( e_1 == e_3 )
        {
            ++unlink_count;
            DeactivateArc(e_1);
        }
        else
        {
            Reconnect<Head>(e_3,e_1);
        }
        
        DeactivateCrossing(c);
//
//        valprint("c  ",CrossingString(c));
//        valprint("e_0",ArcString(e_0));
//        valprint("e_1",ArcString(e_1));
//        valprint("e_2",ArcString(e_2));
//        valprint("e_3",ArcString(e_3));
        
        return true;
    }
    else
    {
        eprint(ClassName()+"::ResolveCrossing: Crossing " + CrossingString(c) + " was already deactivated.");
        return false;
    }
}
