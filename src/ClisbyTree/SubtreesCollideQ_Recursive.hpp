private:

template<bool mQ>
bool SubtreesCollideQ_Recursive( const Int i )
{
    const bool interiorQ = InteriorNodeQ(i);
    
    if( interiorQ )
    {
        auto [L,R] = Children(i);
        
        PushTransform(i,L,R);
        
        const NodeSplitFlagMatrix_T F = NodeSplitFlagMatrix<mQ>(L,R);
        
        // TODO: We should prioritize SubtreesCollideQ_Recursive(i) if NodeBegin(i) is closer to a pivot than NodeEnd(i)-1.
        
        if( (F[0][0] && F[0][1]) && SubtreesCollideQ_Recursive<mQ>(L) )
        {
            return true;
        }
        
        if( (F[1][0] && F[1][1]) && SubtreesCollideQ_Recursive<mQ>(R) )
        {
            return true;
        }
        
        if( ( (F[0][0] && F[1][1]) || (F[0][1] && F[1][0]) ) && BallsOverlapQ(L,R) &&SubtreesCollideQ_Recursive<mQ>(L,R) )
        {
            return true;
        }
    }
    
    return false;
    
} // SubtreesCollideQ_Recursive

template<bool mQ>
bool SubtreesCollideQ_Recursive( const Int i, const Int j )
{
    const bool i_interiorQ = InteriorNodeQ(i);
    const bool j_interiorQ = InteriorNodeQ(j);
    
    if( i_interiorQ && j_interiorQ )
    {
        // Split both nodes.
        const Int c_i [2] = {LeftChild(i),RightChild(i)};
        const Int c_j [2] = {LeftChild(j),RightChild(j)};
        
        PushTransform(i,c_i[0],c_i[1]);
        PushTransform(j,c_j[0],c_j[1]);
        
        const NodeSplitFlagMatrix_T F_i = NodeSplitFlagMatrix<mQ>(c_i[0],c_i[1]);
        const NodeSplitFlagMatrix_T F_j = NodeSplitFlagMatrix<mQ>(c_j[0],c_j[1]);
        
        auto subdQ = [&c_i,&c_j,&F_i,&F_j,this]( const bool k, const bool l )
        {
            return
            ( (F_i[k][0] && F_j[l][1]) || (F_i[k][1] && F_j[l][0]) )
            &&
            this->BallsOverlapQ(c_i[k],c_j[l]);
        };
        
        // We want the compiler to generate tail calls with as little data as possible.
        // Also, we want that all the node's ball information is used right now (and not much later when the tail call is actually carried out.
        // So we enforce the collision checks right now.
        const bool subdivideQ [2][2] = {
            {subdQ(0,0),subdQ(0,1)},
            {subdQ(1,0),subdQ(1,1)}
        };
        
        // TODO: We should find a better order for these four calls.
        // TODO: E.g., we should first check nodes that contain a pivot.

        
        // Doing (c_i[0],c_j[1]) and (c_i[1],c_j[0]) should take us closer to the pivots, where many collisions happen.
        if( subdivideQ[0][1] && SubtreesCollideQ_Recursive<mQ>(c_i[0],c_j[1]) )
        {
            return true;
        }
        
        if( subdivideQ[1][0] && SubtreesCollideQ_Recursive<mQ>(c_i[1],c_j[0]) )
        {
            return true;
        }
        
        if( subdivideQ[0][0] && SubtreesCollideQ_Recursive<mQ>(c_i[0],c_j[0]) )
        {
            return true;
        }
        
        if( subdivideQ[1][1] && SubtreesCollideQ_Recursive<mQ>(c_i[1],c_j[1]) )
        {
            return true;
        }

        
//        if( subdivideQ[0][0] && SubtreesCollideQ_Recursive<mQ>(c_i[0],c_j[0]) )
//        {
//            return true;
//        }
//        
//        if( subdivideQ[1][1] && SubtreesCollideQ_Recursive<mQ>(c_i[1],c_j[1]) )
//        {
//            return true;
//        }
//        
//        if( subdivideQ[0][1] && SubtreesCollideQ_Recursive<mQ>(c_i[0],c_j[1]) )
//        {
//            return true;
//        }
//        
//        if( subdivideQ[1][0] && SubtreesCollideQ_Recursive<mQ>(c_i[1],c_j[0]) )
//        {
//            return true;
//        }
    }
    else if( !i_interiorQ && j_interiorQ )
    {
        // Split node j.
        const Int c_j [2] = {LeftChild(j),RightChild(j)};
        PushTransform(j,c_j[0],c_j[1]);

        const NodeSplitFlagVector_T f_i = NodeSplitFlagVector<mQ>(j);
        const NodeSplitFlagMatrix_T F_j = NodeSplitFlagMatrix<mQ>(c_j[0],c_j[1]);
        
        auto subdQ = [i,&c_j,&f_i,&F_j,this]( const bool l )
        {
            return
                ( (f_i[0] && F_j[l][1]) || (f_i[1] && F_j[l][0]) )
                &&
                this->BallsOverlapQ(i,c_j[l]);
        };
        
        // We want the compiler to generate tail calls with as little data as possible.
        // Also, we want that all the node's ball information is used right now (and not much later when the tail call is actually carried out.
        // So we enforce the collision checks right now.
        const bool subdivideQ [2] = { subdQ(0), subdQ(1) };
        
        // TODO: We could exploit here that we now that i, c_j[0], and c_j[1] are all leaf nodes.
        
        if( subdivideQ[0] && SubtreesCollideQ_Recursive<mQ>(i,c_j[0]) )
        {
            return true;
        }
        
        if( subdivideQ[1] && SubtreesCollideQ_Recursive<mQ>(i,c_j[1]) )
        {
            return true;
        }
    }
    else if( i_interiorQ && !j_interiorQ )
    {
        // Split node i.
        const Int c_i [2] = {LeftChild(i),RightChild(i)};
        PushTransform( i, c_i[0], c_i[1] );
        
        const NodeSplitFlagMatrix_T F_i = NodeSplitFlagMatrix<mQ>(c_i[0],c_i[1]);
        const NodeSplitFlagVector_T f_j = NodeSplitFlagVector<mQ>(j);
        
        auto subdQ = [&c_i,&F_i,j,&f_j,this]( const bool k )
        {
            return
                ( (F_i[k][0] && f_j[1]) || (F_i[k][1] && f_j[0]) )
                &&
                this->BallsOverlapQ(c_i[k],j);
        };
        
        // We want the compiler to generate tail calls with as little data as possible.
        // Also, we want that all the node's ball information is used right now (and not much later when the tail call is actually carried out.
        // So we enforce the collision checks right now.
        const bool subdivideQ [2] = { subdQ(0), subdQ(1) };

        // TODO: We could exploit here that we now that i, c_j[0], and c_j[1] are all leaf nodes.
        
        if( subdivideQ[0] && SubtreesCollideQ_Recursive<mQ>(c_i[0],j) )
        {
            return true;
        }
        
        if( subdivideQ[1] && SubtreesCollideQ_Recursive<mQ>(c_i[1],j) )
        {
            return true;
        }
    }
    else
    {
        // Nodes i and j are overlapping leaf nodes.
        
        // Rule out that tiny distance errors of neighboring vertices cause problems.
        const Int k = NodeBegin(i);
        const Int l = NodeBegin(j);
        
        const Int delta = Abs(k-l);
        
        if( Min( delta, VertexCount() - delta ) > Int(1) )
        {
            witness[0] = k;
            witness[1] = l;
            
            return true;
        }
    }
    
    return false;
    
} // SubtreesCollideQ_Recursive
