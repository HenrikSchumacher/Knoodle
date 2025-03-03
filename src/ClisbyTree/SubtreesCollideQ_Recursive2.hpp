private:

bool SubtreesCollideQ_Rec2( const Int N, cref<Transform_T> F )
{
    const bool interiorQ = InteriorNodeQ(N);
    
    if( interiorQ )
    {
        print("Z");
        auto [L,R] = Children(N);
        
        const NodeSplitFlagMatrix_T a = NodeSplitFlagMatrix(L,R);
        
        {
            Vector_T c = F(NodeCenter(L));
            Vector_T x = NodeCenterAbsoluteCoordinates(L);
            
            Vector_T z = x - c;
            
            if( z.Norm() > 0.00000000001 )
            {
                eprint("Z_L!");
                dump(L);
                dump(x);
                dump(c);
                exit(3);
                
            }
        }
        
        {
            Vector_T c = F(NodeCenter(R));
            Vector_T x = NodeCenterAbsoluteCoordinates(R);
            
            Vector_T z = x - c;
            
            if( z.Norm() > 0.00000000001 )
            {
                eprint("Z_R!");
                dump(L);
                dump(x);
                dump(c);
                exit(3);
                
            }
        }
        
        const Transform_T F_L = F(NodeTransform(L));
        
        // Left child vs. left child.
        if( (a[0][0] && a[0][1]) && SubtreesCollideQ_Rec2(L,F_L) )
        {
            return true;
        }
        
        const Transform_T F_R = F(NodeTransform(R));
        
        // Right child vs. right child.
        if( (a[1][0] && a[1][1]) && SubtreesCollideQ_Rec2(R,F_R) )
        {
            return true;
        }

        
        
        
        // Left child vs. right child.
        if( ( (a[0][0] && a[1][1]) || (a[0][1] && a[1][0]) )
            &&
            BallsOverlapQ(
                F(NodeCenter(L)), NodeRadius(L),
                F(NodeCenter(R)), NodeRadius(R),
                hard_sphere_diam
            )
            &&
            SubtreesCollideQ_Rec2( L, F_L, R, F_R ) )
        {
            return true;
        }
    }
    
    return false;
    
} // SubtreesCollideQ_Rec2

bool SubtreesCollideQ_Rec2(
    const Int N_0, cref<Transform_T> F_0,
    const Int N_1, cref<Transform_T> F_1
)
{
    const bool intQ_0 = InteriorNodeQ(N_0);
    const bool intQ_1 = InteriorNodeQ(N_1);
    
    if( intQ_0 && intQ_1 )
    {
        print("A");
        // Split both nodes.
        
        const Int C [2][2] = {
            { LeftChild(N_0), RightChild(N_0) },
            { LeftChild(N_1), RightChild(N_1) }
        };
        
//        valprint("C",ArrayToString(&C[0][0],{2,2}));
        
        const Transform_T F [2][2] = {
            { F_0( NodeTransform(C[0][0]) ), F_0( NodeTransform(C[0][1]) ) },
            { F_1( NodeTransform(C[1][0]) ), F_1( NodeTransform(C[1][1]) ) }
        };
        
        const Vector_T c [2][2] = {
            { F_0( NodeCenter(C[0][0]) ), F_0( NodeCenter(C[0][1]) ) },
            { F_1( NodeCenter(C[1][0]) ), F_1( NodeCenter(C[1][1]) ) }
        };
        
        for( Int k = 0; k < 2; ++k)
        {
            for( Int l = 0; l < 2; ++l)
            {
                Vector_T x = NodeCenterAbsoluteCoordinates(C[k][l]);
                
                Vector_T z = x - c[k][l];
                
                if( z.Norm() > 0.00000000001 )
                {
                    eprint("!!!!");
                    dump(x);
                    dump(c[k][l]);
                    exit(3);
                    
                }
            }
        }
        
//        valprint("c",ArrayToString(&c[0][0],{2,2}));
        
        const Real r [2][2] = {
            { NodeRadius(C[0][0]), NodeRadius(C[0][1]) },
            { NodeRadius(C[1][0]), NodeRadius(C[1][1]) }
        };
        
//        valprint("r",ArrayToString(&r[0][0],{2,2}));
        
        const NodeSplitFlagMatrix_T A_0 = NodeSplitFlagMatrix(C[0][0],C[0][1]);
        const NodeSplitFlagMatrix_T A_1 = NodeSplitFlagMatrix(C[1][0],C[1][1]);
        
//        dump(NodeBegin(N_0));
//        dump(NodeEnd  (N_0));
//        dump(NodeBegin(N_1));
//        dump(NodeEnd  (N_1));
//        
//        dump(A_0);
//        dump(A_1);
//        
        auto subdQ = [&c,&r,&A_0,&A_1,this]( const bool k, const bool l )
        {
            return
            ( (A_0[k][0] && A_1[l][1]) || (A_0[k][1] && A_1[l][0]) )
            &&
            this->BallsOverlapQ(
                c[0][k], r[0][k], c[1][l], r[1][l], this->hard_sphere_diam
            );
        };
        
        const bool subdivideQ [2][2] = {
            {subdQ(0,0),subdQ(0,1)},
            {subdQ(1,0),subdQ(1,1)}
        };
        
//        valprint("subdivideQ",ArrayToString(&subdivideQ[0][0],{2,2}));
        

        //             +-----------------------------------+-------+
        //             |                                   |       |
        if( subdivideQ[0][0] && SubtreesCollideQ_Rec2(C[0][0],F[0][0],C[1][0],F[1][0]) )
        {               //|                                                |       |
            return true;//+------------------------------------------------+-------+
        }
        
        //             +-----------------------------------+-------+
        //             |                                   |       |
        if( subdivideQ[1][1] && SubtreesCollideQ_Rec2(C[0][1],F[0][1],C[1][1],F[1][1]) )
        {               //|                                                |       |
            return true;//+------------------------------------------------+-------+
        }
        
        //             +-----------------------------------+-------+
        //             |                                   |       |
        if( subdivideQ[0][1] && SubtreesCollideQ_Rec2(C[0][0],F[0][0],C[1][1],F[1][1]) )
        {               //|                                                |       |
            return true;//+------------------------------------------------+-------+
        }
        
        if( ( N_0 != N_1 ) &&
        //             +-----------------------------------+-------+
        //             |                                   |       |
            subdivideQ[1][0] && SubtreesCollideQ_Rec2(C[0][1],F[0][1],C[1][0],F[1][0] )
                        //|                                                |       |
                        //+------------------------------------------------+-------+
        )
        {
            return true;
        }
    }
    else if( !intQ_0 && intQ_1 )
    {
        print("B");
        // Split node N_1.
        const Int C [2] = { LeftChild(N_1), RightChild(N_1) };

        const Transform_T F [2] =
            { F_1( NodeTransform(C[0]) ), F_1( NodeTransform(C[1]) ) };
        
        const Vector_T c [2] =
            { F_1( NodeCenter(C[0]) ), F_1( NodeCenter(C[1]) ) };
        
        const Real r [2] = { NodeRadius(C[0]), NodeRadius(C[1]) };
        
        const NodeSplitFlagVector_T a_0 = NodeSplitFlagVector(N_0);
        const NodeSplitFlagMatrix_T A_1 = NodeSplitFlagMatrix(C[0],C[1]);
        
        auto subdQ = [N_0,&c,&r,&a_0,&A_1,this]( const bool l )
        {
            return
            ( (a_0[0] && A_1[l][1]) || (a_0[1] && A_1[l][0]) )
            &&
            this->BallsOverlapQ(
                NodeCenter(N_0), NodeRadius(N_0), c[l], r[l], this->hard_sphere_diam
            );
        };
        
        const bool subdivideQ [2] = { subdQ(0), subdQ(1) };
        
        if( subdivideQ[0] && SubtreesCollideQ_Rec2( N_0, F_0, C[0], F[0] ) )
        {
            return true;
        }
        
        if( subdivideQ[1] && SubtreesCollideQ_Rec2( N_0, F_0, C[1], F[1] ) )
        {
            return true;
        }
    }
    else if( intQ_0 && !intQ_1 )
    {
        print("C");
        // Split node N_0.
        const Int C [2] = { LeftChild(N_0), RightChild(N_0) };
        
        const Transform_T F [2] =
            { F_0( NodeTransform(C[0]) ), F_0( NodeTransform(C[1]) ) };
        
        const Vector_T c [2] =
            { F_0( NodeCenter(C[0]) ), F_0( NodeCenter(C[1]) ) };
        
        const Real r [2] = { NodeRadius(C[0]), NodeRadius(C[1]) };
        
        const NodeSplitFlagMatrix_T A_0 = NodeSplitFlagMatrix(C[0],C[1]);
        const NodeSplitFlagVector_T a_1 = NodeSplitFlagVector(N_1);

        
        auto subdQ = [N_1,&c,&r,&A_0,&a_1,this]( const bool k )
        {
            return
                ( (A_0[k][0] && a_1[1]) || (A_0[k][1] && a_1[0]) )
                &&
                this->BallsOverlapQ(
                    c[k], r[k], NodeCenter(N_1), NodeRadius(N_1), this->hard_sphere_diam
                );
        };
        
        const bool subdivideQ [2] = { subdQ(0), subdQ(1) };

        if( subdivideQ[0] && SubtreesCollideQ_Rec2( C[0], F[0], N_1, F_1 ) )
        {
            return true;
        }
        
        if( subdivideQ[1] && SubtreesCollideQ_Rec2( C[1], F[1], N_1, F_1 ) )
        {
            return true;
        }
    }
    else
    {
        print("D");
        // Nodes M and N are overlapping leaf nodes.
        
        // Rule out that tiny distance errors of neighboring vertices cause problems.
        const Int i_0 = NodeBegin(N_0);
        const Int i_1 = NodeBegin(N_1);
        
        const Int delta = Abs(i_0 - i_1);
        
        if( Min( delta, VertexCount() - delta ) > Int(1) )
        {
            witness_0 = i_0;
            witness_1 = i_1;
            
            
            dump(i_0);
            dump(i_1);
            
            Vector_T x = NodeCenter(N_0);
            Vector_T y = NodeCenter(N_1);
            Vector_T z = x - y;
            
            dump(x);
            dump(y);
            dump(z);
            
            Vector_T u = F_0( x );
            Vector_T v = F_1( y );
            Vector_T w = u - v;
            dump(u);
            dump(v);
            dump(w);
            
            dump((x-y).SquaredNorm());
            dump(z.SquaredNorm());
            dump((u-v).SquaredNorm());
            dump(w.SquaredNorm());
            
            return true;
        }
    }
    
    return false;
    
} // SubtreesCollideQ_Rec2

