public:

template<IntQ ExtInt, IntQ ExtInt2>
static Tetrahedralization FromSimplices( cptr<ExtInt> simplices, const ExtInt2 simplex_count )
{
//    print("");
//    print("FromSimplices");
    Tetrahedralization T;
    
    const Int n = int_cast<Int>(simplex_count);
    T.SetTetCapacity(n);
    T.tet_count = n;
    T.tet_state.Fill(true);
    
    Tiny::Vector<4,Int,Int> v_0;
    Tiny::Vector<4,Int,Int> v_1;
    
    using Triangle_T = std::array<Int,3>;
    
    SortNet<3> S;
    
    AssociativeContainer<Triangle_T,Int> lut;
    
    for( Int t_0 = 0; t_0 < n; ++t_0 )
    {
        v_0.Read(&simplices[Int(4) * t_0]);
        
//        // DEBUGGING
//        logprint("");
//        TOOLS_LOGDUMP(t_0);
//        TOOLS_LOGDUMP(v_0);
        
        for( Int f_0 = 0; f_0 < Int{4}; ++f_0 )
        {
            Dface_T df_0 (t_0, f_0);
            
            
            const Triangle_T triangle_0 = {
                v_0[df_0.Vertex(Z3{0}).v], v_0[df_0.Vertex(Z3{1}).v], v_0[df_0.Vertex(Z3{2}).v]
            };
            
            Triangle_T key;
            S(triangle_0.data(),key.data());
            
//            // DEBUGGING
//            TOOLS_LOGDUMP(df_0);
//            TOOLS_LOGDUMP(triangle_0);
//            TOOLS_LOGDUMP(key);
            
            if( !lut.contains(key) )
            {
                lut[key] = df_0.Compress();
            }
            else
            {
                Dface_T df_1 (lut[key]);
                lut.erase(key);
                
                v_1.Read(&simplices[Int(4) * df_1.t]);
                
                const Triangle_T triangle_1 = {
                    v_1[df_1.Vertex(Z3{0}).v], v_1[df_1.Vertex(Z3{1}).v], v_1[df_1.Vertex(Z3{2}).v]
                };
            
                
//                // DEBUGGING
//                TOOLS_LOGDUMP(df_1);
//                TOOLS_LOGDUMP(v_1);
//                TOOLS_LOGDUMP(triangle_1);
                
                // Rotation r must statisfy triangle_1[0] == triangle_0[r].
                
                if( triangle_1[0] == triangle_0[0] )
                {
                    T.Glue_Private<false>(df_0,df_1,Z3{0});
                }
                else if( triangle_1[0] == triangle_0[1] )
                {
                    T.Glue_Private<false>(df_0,df_1,Z3{1});
                }
                else if( triangle_1[0] == triangle_0[2] )
                {
                    T.Glue_Private<false>(df_0,df_1,Z3{2});
                }
                else
                {
                    eprint(MethodName("FromSimplices"),": Something went wrong.");
                }
            }
        }
    }
    
//    print("");
    
    return T;
}


template<IntQ ExtInt, IntQ ExtInt2>
static Tetrahedralization FromTetGluings(
    cptr<ExtInt>  tet_glue_,
    cptr<bool>    tet_state_,
    const ExtInt2 tet_count_,
    const ExtInt  uninitialized = Tetrahedralization<ExtInt>::Uninitialized
)
{
    Tetrahedralization T;
    
    const Int n = int_cast<Int>(tet_count_);
    
    T.SetTetCapacity(n);
    T.tet_count = n;
    
    for( Int t = 0; t < n; ++t )
    {
        if( !tet_state_[t] ) { continue; }
        T.tet_state[t] = true;
        for( Int f = 0; f < Int{4}; ++f )
        {
            const ExtInt g = tet_glue_[Int(4) * t + f];
            
            T.tet_glue(t,f) = (g == uninitialized) ? Uninitialized : int_cast<Int>(g);
        }
    }
    
    return T;
}
