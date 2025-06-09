// Book: Di Battista, Eades, Tamassia, Tollis - Graph Drawing
// Algorithm 6.2 (p.185): Saturate-Face

// Hashemi, Tahmasbi - A better heuristic for area-compaction of orthogonal representations

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



//Dir_T G_l_Direction( Int de ) const
//{
//    Dir_T e_dir = TRE_dir[de];
//
//    if( e_dir == South )
//    {
//        return North;
//    }
//    else if( e_dir == East )
//    {
//        return West;
//    }
//
//    return e_dir;
//}
//
//Dir_T G_r_Direction( Int de ) const
//{
//    Dir_T e_dir = TRE_dir[de];
//
//    if( e_dir == South )
//    {
//        return North;
//    }
//    else if( e_dir == West )
//    {
//        return East;
//    }
//
//    return e_dir;
//}


Switch_T G_l_SwitchType( Int da )
{
    cptr<Turn_T> dTRE_turn = TRE_turn.data();
    
    // Int db = E_left_dE_reg.data()[da];
    
    auto [a,d] = FromDiArc(da);
    
    Dir_T a_dir = TRE_dir[a];
    
    if(!d)
    {
        a_dir = (a_dir + Dir_T(2)) % Dir_T(4);
    }
    
    constexpr Switch_T None = Switch_T::None;
    constexpr Switch_T s_S  = Switch_T::s_S;
    constexpr Switch_T s_L  = Switch_T::s_L;
    constexpr Switch_T t_S  = Switch_T::t_S;
    constexpr Switch_T t_L  = Switch_T::t_L;
    

//    constexpr Switch_T lut [4][4] = {
//    //        East  North West  South
//    /* 0*/  { None, None, None, None },
//    /* 1*/  { s_S , None, t_S,  None },
//    /* 2*/  { None, None, None, None },
//    /*-1*/  { None, t_L , None, s_L  }
//    };
    
    
    if( dTRE_turn[da] == Turn_T(-1) )
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
            return t_L;
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
            return None;
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

            return s_L;
        }
        else
        {
            return None;
        }
    }
    else if( dTRE_turn[da] == Turn_T(1) )
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
            
            return s_S;
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
            return None;
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
            return t_S;
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

            return None;
        }
        else
        {
            return None;
        }
    }
    else if( dTRE_turn[da] == Turn_T(2) )
    {
        eprint(ClassName() + "::G_l_SwitchType: turn = 2; case not handled at the moment.");
        return None;
    }
    else
    {
        return None;
    }
}


Switch_T G_r_SwitchType( Int da )
{
    cptr<Turn_T> dTRE_turn = TRE_turn.data();
    
    // Int db = E_left_dE_reg.data()[da];
    
    auto [a,d] = FromDiArc(da);
    
    Dir_T a_dir = TRE_dir[a];
    
    if(!d)
    {
        a_dir = (a_dir + Dir_T(2)) % Dir_T(4);
    }
    
    constexpr Switch_T None = Switch_T::None;
    constexpr Switch_T s_S  = Switch_T::s_S;
    constexpr Switch_T s_L  = Switch_T::s_L;
    constexpr Switch_T t_S  = Switch_T::t_S;
    constexpr Switch_T t_L  = Switch_T::t_L;
    

//    constexpr Switch_T lut [4][4] = {
//    //        East  North West  South
//    /* 0*/  { None, None, None, None },
//    /* 1*/  { None, t_S , None, s_S  },
//    /* 2*/  { None, None, None, None },
//    /*-1*/  { t_L,  None, s_L , None }
//    };
    
    if( dTRE_turn[da] == Turn_T(-1) )
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
            
            return t_L;
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
            return None;
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
            return s_L;
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

            return None;
        }
        else
        {
            return None;
        }
    }
    else if( dTRE_turn[da] == Turn_T(1) )
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
            
            return None;
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
            return t_S;
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
            return None;
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

            return s_S;
        }
        else
        {
            return None;
        }
    }
    else if( dTRE_turn[da] == Turn_T(2) )
    {
        eprint(ClassName() + "::G_l_SwitchType: turn = 2; case not handled at the moment.");
        return None;
    }
    else
    {
        return None;
    }
}


// Saturating edges are of the form
//
//  s_S --> s_L or t_L --> s_S
//



// Di Battista, Liotta - Upward Planarity Checking: “Faces Are More than Polygons”

//             x
//  X-------->X         X
//            |         ^
//            |         |
//            |         |
//            v         |
//            X-------->X
//           y           z


//             x
//  X-------->X         X
//            ^         ^
//            |         |
//            |         |
//            |         |
//            X-------->X
//           y           z


//def saturate_face(face_info):
//    # Normalize so it starts with a -1 turn, if any
////    for i, a in enumerate(face_info):
////        if a.turn == -1:
////            face_info = face_info[i:] + face_info[:i]
////            break
//    for i in range(len(face_info) - 2):
//        x, y, z = face_info[i:i + 3]
//        if x.turn == -1 and y.turn == z.turn == 1:
//            a, b = (x, z) if x.kind == 'sink' else (z, x)
//            remaining = face_info[:i] + [LabeledFaceVertex(z.index, z.kind, 1)
//                 ] + face_info[i + 3:]
//            return [(a.index, b.index)] + saturate_face(remaining)
//    return []


// TODO: Make private:
public:

Tensor1<UInt8,Int> TRF_TRE_G_l_SwitchTypes()
{
    TOOLS_PTIC(ClassName() + "::TRF_TRE_G_l_SwitchTypes");
    
    Aggregator<UInt8,Int> agg ( 4 * vertex_count );

    TRF_TraverseAll(
        [](){},
        [&agg,this]( const Int de )
        {
            agg.Push( ToUnderlying(G_l_SwitchType(de)) );
        },
        [](){}
    );
    
    TOOLS_PTOC(ClassName() + "::TRF_TRE_G_l_SwitchTypes");
    
    return agg.Get();
}

Tensor1<UInt8,Int> TRF_TRE_G_r_SwitchTypes()
{
    TOOLS_PTIC(ClassName() + "::TRF_TRE_G_r_SwitchTypes");
    
    Aggregator<UInt8,Int> agg ( 4 * vertex_count );

    TRF_TraverseAll(
        [](){},
        [&agg,this]( const Int de )
        {
            agg.Push( ToUnderlying(G_r_SwitchType(de)) );
        },
        [](){}
    );
    
    TOOLS_PTOC(ClassName() + "::TRF_TRE_G_r_SwitchTypes");
    
    return agg.Get();
}
