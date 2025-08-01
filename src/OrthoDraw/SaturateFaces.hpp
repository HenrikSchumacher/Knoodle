// Book: Di Battista, Eades, Tamassia, Tollis - Graph Drawing
// Algorithm 6.2 (p.185): Saturate-Face

// Hashemi, Tahmasbi - A better heuristic for area-compaction of orthogonal representations

public:

bool SaturateFacesQ() const
{
    return settings.saturate_facesQ;
}

void SetSaturateFacesQ( const bool val )
{
    settings.saturate_facesQ = val;
}

bool SaturateExteriorFaceQ() const
{
    return settings.saturate_exterior_faceQ;
}

void SetSaturateExteriorFaceQ( const bool val )
{
    settings.saturate_exterior_faceQ = val;
}

enum class Switch_T : UInt8
{
    None = 0,
    s_S  = 4 * Tail + 2 * false + 1,
    s_L  = 4 * Tail + 2 * true  + 1,
    t_S  = 4 * Head + 2 * false + 1,
    t_L  = 4 * Head + 2 * true  + 1
};

static std::string SwitchString( Switch_T t )
{
    switch(t)
    {
        case Switch_T::s_S: return "s_S";
        case Switch_T::s_L: return "s_L";
        case Switch_T::t_S: return "t_S";
        case Switch_T::t_L: return "t_L";
        default           : return " ";
    }
}



template<bool GrQ>
Switch_T SwitchType( const Int de ) const
{
    if constexpr ( GrQ )
    {
        return GrSwitchType(de);
    }
    else
    {
        return GlSwitchType(de);
    }
}

private:

Switch_T GlSwitchType( Int da ) const
{
    cptr<Turn_T> dE_turn = E_turn.data();
    
    // Int db = E_left_dE_reg.data()[da];
    
    auto [a,d] = FromDedge(da);
    
    Dir_T a_dir = E_dir[a];
    
    if(!d)
    {
        a_dir = (a_dir + Dir_T(2)) % Dir_T(4);
    }
    
//    constexpr Switch_T None = Switch_T::None;
//    constexpr Switch_T s_S  = Switch_T::s_S;
//    constexpr Switch_T s_L  = Switch_T::s_L;
//    constexpr Switch_T t_S  = Switch_T::t_S;
//    constexpr Switch_T t_L  = Switch_T::t_L;
//
//    constexpr Switch_T lut [4][4] = {
//    //        East  North West  South
//    /* 0*/  { None, None, None, None },
//    /* 1*/  { s_S , None, t_S,  None },
//    /* 2*/  { None, None, None, None },
//    /*-1*/  { None, t_L , None, s_L  }
//    };
    
    
    if( dE_turn[da] == Turn_T(-1) )
    {
        if( a_dir == East )
        {
//              In H:               In G_l:
//
//                   f
//              X-------->X         X<--------X
//                  da    |                   ^
//                        | db                |     (no switch)
//                        |                   |
//                        v                   |
//                        X                   X
            
            return Switch_T::None;
        }
        else if( a_dir == North )
        {
//              In H:               In G_l:
//                  db
//              X-------->X     t_L X<--------X
//              ^                   ^
//              |                   |
//           f  | da                |               (sink (t), (L)arge angle)
//              |                   |
//              X                   X
//
            return Switch_T::t_L;
        }
        else if( a_dir == West )
        {
//              In H:               In G_l:
//              X                   X
//              ^                   ^
//              | db                |
//              |                   |               (no switch)
//              |   da              |
//              X<------->X         X<--------X
//
//                  f
            return Switch_T::None;
        }
        else if( a_dir == South )
        {
//              In H:               In G_l:
//                        X                   X
//                        |                   ^
//                     da | f                 |
//                        |                   |     (sourse (s), (L)arge angle)
//                  db    v                   |
//              X<--------X         X<--------X s_L

            return Switch_T::s_L;
        }
        else
        {
            return Switch_T::None;
        }
    }
    else if( dE_turn[da] == Turn_T(1) )
    {
        if( a_dir == East )
        {
//              In H:               In G_l:
//
//
//                        X                   X
//                        ^                   ^
//                   f    | db                |     (source (s), (S)mall angle)
//                        |                   |
//                  da    |                s_S|
//              X-------->X         X<--------X
            
            return Switch_T::s_S;
        }
        else if( a_dir == North )
        {
//              In H:               In G_l:
//                  db
//              X<--------X         X<--------X
//                        ^                   ^
//                        |                   |
//                   f    | da                |     (no switch)
//                        |                   |
//                        X                   X
//
            return Switch_T::None;
        }
        else if( a_dir == West )
        {
//              In H:               In G_l:
//              X<--------X         X<--------X
//              |   da              ^t_S
//              |                   |
//           db |    f              |               (sink (t), (S)mall angle)
//              v                   |
//              X                   X
//
//                  f
            return Switch_T::t_S;
        }
        else if( a_dir == South )
        {
//              In H:               In G_l:
//              X                   X
//              |                   ^
//           da |    f              |
//              |                   |               (no switch)
//              v   db              |
//              X-------->X         X<--------X

            return Switch_T::None;
        }
        else
        {
            return Switch_T::None;
        }
    }
    else if( dE_turn[da] == Turn_T(2) )
    {
        eprint(ClassName()+"::GlSwitchType: turn = 2; case not handled at the moment.");
        return Switch_T::None;
    }
    else
    {
        return Switch_T::None;
    }
}


Switch_T GrSwitchType( Int da ) const
{
    cptr<Turn_T> dE_turn = E_turn.data();
    
    // Int db = E_left_dE_reg.data()[da];
    
    auto [a,d] = FromDedge(da);
    
    Dir_T a_dir = E_dir[a];
    
    if(!d)
    {
        a_dir = (a_dir + Dir_T(2)) % Dir_T(4);
    }
    
//    constexpr Switch_T None = Switch_T::None;
//    constexpr Switch_T s_S  = Switch_T::s_S;
//    constexpr Switch_T s_L  = Switch_T::s_L;
//    constexpr Switch_T t_S  = Switch_T::t_S;
//    constexpr Switch_T t_L  = Switch_T::t_L;
//
//    constexpr Switch_T lut [4][4] = {
//    //        East  North West  South
//    /* 0*/  { None, None, None, None },
//    /* 1*/  { None, t_S , None, s_S  },
//    /* 2*/  { None, None, None, None },
//    /*-1*/  { t_L,  None, s_L , None }
//    };
    
    if( dE_turn[da] == Turn_T(-1) )
    {
        if( a_dir == East )
        {
//              In H:               In G_r:
//
//                   f                         t_L
//              X-------->X         X-------->X
//                  da    |                   ^
//                        | db                |     (sink (t), (L)arge angle)
//                        |                   |
//                        v                   |
//                        X                   X
            
            return Switch_T::t_L;
        }
        else if( a_dir == North )
        {
//              In H:               In G_r:
//                  db
//              X-------->X         X-------->X
//              ^                   ^
//              |                   |
//           f  | da                |               (no switch)
//              |                   |
//              X                   X
//
            return Switch_T::None;
        }
        else if( a_dir == West )
        {
//              In H:               In G_r:
//              X                   X
//              ^                   ^
//              | db                |
//              |                   |               (source (s), (L)arge angle)
//              |   da              |
//              X<------->X         X-------->X
//                               s_L
//                  f
            return Switch_T::s_L;
        }
        else if( a_dir == South )
        {
//              In H:               In G_r:
//                        X                   X
//                        |                   ^
//                     da | f                 |
//                        |                   |     (no switch)
//                  db    v                   |
//              X<--------X         X-------->X

            return Switch_T::None;
        }
        else
        {
            return Switch_T::None;
        }
    }
    else if( dE_turn[da] == Turn_T(1) )
    {
        if( a_dir == East )
        {
//              In H:               In G_r:
//
//
//                        X                   X
//                        ^                   ^
//                   f    | db                |     (no switch)
//                        |                   |
//                  da    |                   |
//              X-------->X         X-------->X
            
            return Switch_T::None;
        }
        else if( a_dir == North )
        {
//              In H:               In G_r:
//                  db
//              X<--------X         X-------->X
//                        ^                t_S^
//                        |                   |
//                   f    | da                |     (sink (t), (S)mall angle)
//                        |                   |
//                        X                   X
//
            return Switch_T::t_S;
        }
        else if( a_dir == West )
        {
//              In H:               In G_r:
//              X<--------X         X<--------X
//              |   da              ^
//              |                   |
//           db |    f              |               (no switch)
//              v                   |
//              X                   X
//
//                  f
            return Switch_T::None;
        }
        else if( a_dir == South )
        {
//              In H:               In G_r:
//              X                   X
//              |                   ^
//           da |    f              |
//              |                   |               (source (s), (S)mall angle)
//              v   db              |s_S
//              X-------->X         X-------->X

            return Switch_T::s_S;
        }
        else
        {
            return Switch_T::None;
        }
    }
    else if( dE_turn[da] == Turn_T(2) )
    {
        eprint(ClassName()+"::GrSwitchType: turn = 2; case not handled at the moment.");
        return Switch_T::None;
    }
    else
    {
        return Switch_T::None;
    }
}

public:

template<bool GrQ>
RaggedList<UInt8,Int> FaceSwitchTypes()
{
    TOOLS_PTIMER(timer,ClassName()+"::FaceSwitchTypes<" + ToString(GrQ) + ">");
    
    RaggedList<UInt8,Int> F_types ( Int(2) * E_V.Dim(0), Int(2) * E_V.Dim(0) );

    TraverseAllFaces(
        []( const Int f ){ (void)f; },
        [&F_types,this]( const Int f, const Int k, const Int de )
        {
            (void)f;
            (void)k;
            F_types.Push( ToUnderlying(SwitchType<GrQ>(de)) );
        },
        [&F_types]( const Int f )
        {
            (void)f;
            F_types.FinishSublist();
        },
        false
    );

    return F_types;
}


template<bool GrQ, bool verboseQ = false>
EdgeContainer_T SaturatingEdges() const
{
    TOOLS_PTIMER(timer,ClassName()+"::SaturatingEdges<" + ToString(GrQ) + ">");

    Aggregator<std::array<Int,2>,Int> edge_agg (EdgeCount());
    
    // We make these buffers two steps longer to emulate a cyclic buffer.
    Tensor1<Int,Int>      f_dE( max_face_size * Int(2) );
    Tensor1<Switch_T,Int> f_S ( max_face_size * Int(2) );
    
    
    Int f_size = 0;
    Int f_n = 0; // count of labels
    
    bool exteriorQ = false;
    
    TraverseAllFaces(
        [&exteriorQ,&f_size,&f_n]( const Int f )
        {
            (void)f;
            f_size = 0;
            f_n = 0;
            exteriorQ = false;
        },
        [&exteriorQ,&f_size,&f_n,&f_dE,&f_S,this](
            const Int f, const Int k, const Int de
        )
        {
            (void)f;
            (void)k;
            
            exteriorQ = DedgeExteriorQ(de);
            ++f_size;
            
            Switch_T s = this->template SwitchType<GrQ>(de);
            if( s != Switch_T::None )
            {
                f_dE[f_n] = de;
                f_S [f_n] = s;
                ++f_n;
            }
        },
        [&exteriorQ,&f_size,&f_n,&f_dE,&f_S,&edge_agg,this]( const Int f )
        {
            if( !settings.saturate_exterior_faceQ && exteriorQ ){ return; }
            
            // No saturating edges are needed in sufficiently small faces.
            // BEWARE: This might exploit something specific to knot-diagram!
            if( settings.filter_saturating_edgesQ )
            {
                if( f_size <= Int(7) ) { return; }
            }
//            
//            // DEBUGGING
//            if( f_n < Int(2) )
//            {
//                eprint(this->MethodName("SaturatingEdges") + ": f_n < Int(2)!");
//            }
//            
//            // DEBUGGING
//            if( f_size > max_face_size )
//            {
//                eprint(this->MethodName("SaturatingEdges") + ": f_size > max_face_size !");
//            }
            
            // Make a complete copy to emulate a cyclic list.
            copy_buffer(&f_dE[0],&f_dE[f_n],f_n);
            copy_buffer(&f_S [0],&f_S [f_n],f_n);

            this->template SaturateFace<GrQ,verboseQ>(f,f_dE,f_S,f_n,edge_agg);
        },
        false
    );

    EdgeContainer_T saturating_edges ( edge_agg.Size() );
    
    saturating_edges.Read(&edge_agg[0][0]);
    
    return saturating_edges;
}


template<bool GrQ, bool verboseQ>
void SaturateFace(
    const Int                               f,
    mref<Tensor1<Int,Int>>                  f_dE,
    mref<Tensor1<Switch_T,Int>>             f_S,
    mref<Int>                               f_n,
    mref<Aggregator<std::array<Int,2>,Int>> edge_agg
) const
{
    if constexpr ( !verboseQ ) { (void)f; }
    
    constexpr Switch_T s_L = Switch_T::s_L;
    constexpr Switch_T t_L = Switch_T::t_L;
    constexpr Switch_T s_S = Switch_T::s_S;
    constexpr Switch_T t_S = Switch_T::t_S;
    
    if constexpr ( verboseQ )
    {
        valprint("face",f);
        valprint("face size",f_n);
        
        print( ArrayToString(f_dE.data(), {f_n*Int(2)} ) );
        
        auto s_str = []( const Switch_T x )
        {
            return SwitchString(x);
        };
        
        print( ArrayToString(f_S.data(), {f_n*Int(2)}, s_str) );
    }
        
    bool recurseQ = false;
    
    auto update_buffer = [&f_n,&f_dE,&f_S]( const Int i_2 )
    {
        // Face has positions [0,1,2,...,i_0,i_1,i_2,...f_n + 2[, where
        // i_1 = i_0 + 1 and i_2 = i_0 + 2.
        //
        // Positions i_0 and i_1 must go away.
        
        // First idea was this:
        //      copy_buffer( &f_dE[i_2], &f_dE[i_0], f_n - i_0 );
        //      copy_buffer( &f_S [i_2], &f_S [i_0], f_n - i_0 );
        // (and then duplicated the beginning after the end).
        
        // Problem: We can have f_dE[0] == f_dE[i_1], i.e., we have to remove positions [1,...,i_0[ one step to the left and copy [i_2,...,i_2 + f_n - i_0[ to position i_1 instead. Or something like this.
        // This is super error-prone, so I go for a full duplicate of the face:
        
        // Face has positions [0,1,2,...,i_0,i_1,i_2,...2 * f_n[.
        
        // We have f_dE[0] != f_dE[i_0], so i_0 <= f_n - 1.
        // Thus, i_2 <= f_n + 1.
        // Thus, i_2 + f_n - 2 <= 2 * f_n - 1.

        // This face loses exactly two edges.
        f_n -= Int(2);

        // Take the valid part of the list and move it to the front.
        // Since we guarantee to copy to the left, (0 < i_2), we can safely use std::copy.
        std::copy( &f_dE[i_2], &f_dE[i_2+f_n], &f_dE[0] );
        std::copy( &f_S [i_2], &f_S [i_2+f_n], &f_S [0] );

        // Duplicate whole list to emulate cyclic buffer.
        // We have guarantee of no overlap, so copy_buffer is safe.
        copy_buffer( &f_dE[0], &f_dE[f_n], f_n );
        copy_buffer( &f_S [0], &f_S [f_n], f_n );
    };
    
    auto push = [&edge_agg]( const Int v_0, const Int v_1 )
    {
        if constexpr( verboseQ )
        {
            print("Pushing edge { " + ToString(v_0) + ", " + ToString(v_1) + " }.");
        }
        
        edge_agg.Push({v_0,v_1});
    };
    
    for( Int i_0 = 0; i_0 < f_n; ++i_0 )
    {
        const Int i_1 = i_0 + Int(1);
        const Int i_2 = i_0 + Int(2);
        
        if( (f_S[i_0]==s_L) && (f_S[i_1]==t_S) && (f_S[i_2]==s_S) )
        {
            const Int de_0 = f_dE[i_2];
            const Int de_1 = f_dE[i_0];
            
            const Int v_0 = E_V.data()[de_0];
            const Int v_1 = E_V.data()[de_1];
            
            if constexpr( verboseQ )
            {
                print("Case a: {s_L,t_S,s_S}");
                valprint("i_0",i_0);
                print(
                    "f_dE[i_0] = " + ToString(f_dE[i_0]) + "; " +
                     "f_S[i_0] = " + SwitchString(f_S[i_0])
                );
                print(
                    "f_dE[i_1] = " + ToString(f_dE[i_1]) + "; " +
                     "f_S[i_1] = " + SwitchString(f_S[i_2])
                );
                print(
                    "f_dE[i_2] = " + ToString(f_dE[i_2]) + "; " +
                     "f_S[i_2] = " + SwitchString(f_S[i_2])
                );
            }
            
            if( settings.filter_saturating_edgesQ )
            {
                // BEWARE: Here we exploit that out orthogonal representation
                // comes from a link diagram. Otherwise, we could have
                // T-junctions like this:
                //
                //    +<--------------------------------------+
                //    | \                                   / ^
                //    |   \                               /   |
                //    |     \             T             /     |
                //    |       \           V           /       |
                //    |         +-------->X-------->+         |
                //    |         ^         |         |         |
                //    |         |         |         |         |
                //    |         |         |         |         |
                //    v         |         |         |         |
                //
                // The two diagonal edges are essential, but
                // they would be discarded by our check.
                //
                // Since we come from a knot diagram, the only
                // possible T-junctions come from kitty corners and
                // they look like this:
                //                        ^
                //                        |
                //             f_0        |
                //                        |
                //    --------->+........>+<--------
                //              |    e
                //              |        f_1
                //              |
                //              |
                //
                // In particular, these T-junctions come in pairs and
                // the one leg points to one side of the dotted edge e and
                // the other leg points to the other side.
                // So the maximal segment of e in face f_0 is definitely
                // constrained.
                
                const bool essentialQ =
                       DedgeUnconstrainedQ(de_0)
                    || DedgeUnconstrainedQ(E_left_dE.data()[de_0])
                    || DedgeUnconstrainedQ(de_1)
                    || DedgeUnconstrainedQ(E_left_dE.data()[de_1]);
                
                if( essentialQ )
                {
                    push(v_0,v_1);
                }
                else
                {
                    if constexpr (verboseQ)
                    {
                        print("Discarding nonessential edge { " + ToString(v_0) + ", " + ToString(v_1) + " }.");
                    }
                }
            }
            else
            {
                push(v_0,v_1);
            }
            
            if( f_n >= Int(4) )
            {
                update_buffer(i_2);
                
                recurseQ = true;
                break;
            }
            else
            {
                return;
            }
        }
        else if( (f_S[i_0]==t_L) && (f_S[i_1]==s_S) && (f_S[i_2]==t_S) )
        {
            const Int de_0 = f_dE[i_0];
            const Int de_1 = f_dE[i_2];
            
            const Int v_0 = E_V.data()[de_0];
            const Int v_1 = E_V.data()[de_1];
            
            if constexpr( verboseQ )
            {
                print("Case b: {t_L,s_S,t_S}");
                valprint("i_0",i_0);
                print(
                    "f_dE[i_0] = " + ToString(f_dE[i_0]) + "; " +
                     "f_S[i_0] = " + SwitchString(f_S[i_0])
                );
                print(
                    "f_dE[i_1] = " + ToString(f_dE[i_1]) + "; " +
                     "f_S[i_1] = " + SwitchString(f_S[i_1])
                );
                print(
                    "f_dE[i_2] = " + ToString(f_dE[i_2]) + "; " +
                     "f_S[i_2] = " + SwitchString(f_S[i_2])
                );
            }
            
            if( settings.filter_saturating_edgesQ )
            {
                // BEWARE: Here we exploit that out orthogonal representation
                // comes from a link diagram. Otherwise, we could have
                // dangerous T-junctions (see above).
                
                const bool essentialQ =
                    DedgeUnconstrainedQ(de_0)
                    || DedgeUnconstrainedQ(E_left_dE.data()[de_0])
                    || DedgeUnconstrainedQ(de_1)
                    || DedgeUnconstrainedQ(E_left_dE.data()[de_1]);
                
                if( essentialQ )
                {
                    push(v_0,v_1);
                }
                else
                {
                    if constexpr (verboseQ)
                    {
                        print("Discarding nonessential edge { " + ToString(v_0) + ", " + ToString(v_1) + " }.");
                    }
                }
            }
            else
            {
                push(v_0,v_1);
            }

            if( f_n >= Int(4) )
            {
                update_buffer(i_2);
                
                recurseQ = true;
                break;
            }
            else
            {
                return;
            }
        }
    }
    
    if ( recurseQ )
    {
        this->template SaturateFace<GrQ,verboseQ>(f,f_dE,f_S,f_n,edge_agg);
    }
    else
    {
        if constexpr( verboseQ )
        {
            print(std::string("G") + (GrQ ? "r" : "l") + "Saturation of face " + ToString(f) + " completed.");
        }
    }
}
