private:
    
    bool ProcessArc( const Int a_ )
    {
        if( !pd.ArcActiveQ(a_) ) return false;
        
        ++test_count;
        a = a_;
        
        PD_PRINT( "===================================================" );
        PD_PRINT( "Simplify a = " + ArcString(a) );
        
        AssertArc<1>(a);

        c_0 = A_cross(a,Tail);
        AssertCrossing<1>(c_0);
        
        c_1 = A_cross(a,Head);
        AssertCrossing<1>(c_1);
        
        load_c_0();
        
        /*              n_0
         *               O
         *               |
         *       w_0 O---X-->O a
         *               |c_0
         *               O
         *              s_0
         */
        
        // Check for Reidemeister I move
        if( R_I_center() )
        {
            PD_VALPRINT( "a  ", ArcString(n_0) );
            
            PD_VALPRINT( "c_0", CrossingString(c_0) );
            PD_VALPRINT( "n_0", ArcString(n_0) );
            PD_VALPRINT( "s_0", ArcString(s_0) );
            PD_VALPRINT( "w_0", ArcString(w_0) );
            
            return true;
        }
         
        if constexpr ( optimization_level < 2 )
        {
            return false;
        }
        
        load_c_1();
        
        /*              n_0           n_1
         *               O             O
         *               |      a      |
         *       w_0 O---X---O---->O---X-->O e_1
         *               |c_0          |c_1
         *               O             O
         *              s_0           s_1
         */
        
        // We want to check for twist move _before_ Reidemeister I because the twist move can remove both crossings.
        
        if( twist_at_a() )
        {
            PD_VALPRINT( "a  ", ArcString(n_0) );
            
            PD_VALPRINT( "c_0", CrossingString(c_0) );
            PD_VALPRINT( "n_0", ArcString(n_0) );
            PD_VALPRINT( "s_0", ArcString(s_0) );
            PD_VALPRINT( "w_0", ArcString(w_0) );
            
            PD_VALPRINT( "c_1", CrossingString(c_1) );
            PD_VALPRINT( "n_1", ArcString(n_1) );
            PD_VALPRINT( "e_1", ArcString(e_1) );
            PD_VALPRINT( "s_1", ArcString(s_1) );
            
            return true;
        }

        // Next we check for Reidemeister_I at crossings c_0 and c_1.
        // This will also remove some unpleasant cases for the Reidemeister II and Ia moves.
        
        if( R_I_left() )
        {
            PD_VALPRINT( "a  ", ArcString(n_0) );
            
            PD_VALPRINT( "c_0", CrossingString(c_0) );
            PD_VALPRINT( "n_0", ArcString(n_0) );
            PD_VALPRINT( "s_0", ArcString(s_0) );
            PD_VALPRINT( "w_0", ArcString(w_0) );
            
            PD_VALPRINT( "c_1", CrossingString(c_1) );
            PD_VALPRINT( "n_1", ArcString(n_1) );
            PD_VALPRINT( "e_1", ArcString(e_1) );
            PD_VALPRINT( "s_1", ArcString(s_1) );
            
            return true;
        }
        
        if( R_I_right() )
        {
            PD_VALPRINT( "a  ", ArcString(n_0) );
            
            PD_VALPRINT( "c_0", CrossingString(c_0) );
            PD_VALPRINT( "n_0", ArcString(n_0) );
            PD_VALPRINT( "s_0", ArcString(s_0) );
            PD_VALPRINT( "w_0", ArcString(w_0) );
            
            PD_VALPRINT( "c_1", CrossingString(c_1) );
            PD_VALPRINT( "n_1", ArcString(n_1) );
            PD_VALPRINT( "e_1", ArcString(e_1) );
            PD_VALPRINT( "s_1", ArcString(s_1) );
            
            return true;
        }
        
        // Neglecting asserts, this is the only time we access C_state[c_0].
        // Find out whether the vertical strand at c_0 goes over.
        c_0_state = pd.CrossingState(c_0);
        o_0 = ( u_0 != RightHandedQ(c_0_state) );
        PD_ASSERT(o_0 == pd.ArcUnderQ(a,Tail));
        
        // Neglecting asserts, this is the only time we access C_state[c_1].
        // Find out whether the vertical strand at c_1 goes over.
        c_1_state = pd.CrossingState(c_1);
        o_1 = ( u_1 != RightHandedQ(c_1_state) );
        PD_VALPRINT("o_1", o_1);
        PD_ASSERT(o_1 == pd.ArcUnderQ(a,Head));
        
        // Deal with the case that a is part of a loop of length 2.
        // This can only occur if the diagram has more than one component.
        if constexpr( mult_compQ )
        {
            // This requires o_0 and o_1 to be defined already.
            if(a_is_2loop())
            {
                PD_VALPRINT( "a  ", ArcString(n_0) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
                
                return true;
            }
        }
        
        if(o_0 == o_1)
        {
            /*       |     a     |             |     a     |
             *    -->|---------->|-->   or  -->----------->--->
             *       |c_0        |c_1          |c_0        |c_1
             */
            
            // Attempt the moves that rely on the vertical strands being both on top or both below the horizontal strand.
            if( strands_same_o() )
            {
                PD_VALPRINT( "a  ", ArcString(a) );
                
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
                
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
                
                return true;
            }
        }
        else // if(o_0 != o_1)
        {
            /*       |     a     |             |     a     |
             *    -->|---------->--->   or  -->----------->|-->
             *       |c_0        |c_1          |c_0        |c_1
             */
    
            // Attempt the moves that rely on the vertical strands being separated by the horizontal strand.
            if( strands_diff_o() )
            {
                PD_VALPRINT( "a  ", ArcString(a) );
    
                PD_VALPRINT( "c_0", CrossingString(c_0) );
                PD_VALPRINT( "n_0", ArcString(n_0) );
                PD_VALPRINT( "s_0", ArcString(s_0) );
                PD_VALPRINT( "w_0", ArcString(w_0) );
    
                PD_VALPRINT( "c_1", CrossingString(c_1) );
                PD_VALPRINT( "n_1", ArcString(n_1) );
                PD_VALPRINT( "e_1", ArcString(e_1) );
                PD_VALPRINT( "s_1", ArcString(s_1) );
    
                return true;
            }
        }
        
        AssertArc<1>(a);
        
        return false;
    }
