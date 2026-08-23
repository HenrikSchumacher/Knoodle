private:

static constexpr UInt ToPathNode( const Int node, const bool side )
{
    return static_cast<UInt>(node) | (side ? side_mask : UInt{0});
}

static constexpr Int NodeFromPathNode( const UInt p_node )
{
    return static_cast<Int>(p_node & node_mask);
}

static constexpr bool SideFromPathNode( const UInt p_node )
{
    return static_cast<bool>(p_node & side_mask);
}

static constexpr std::pair<Int,bool> FromPathNode( const UInt p_node )
{
//    return { NodeFromPathNode(p_node), SideFromPathNode(p_node) };
    return {
        static_cast<Int >(p_node & node_mask),
        static_cast<bool>(p_node & side_mask)
    };
}

void InitializePath()
{
    path[0]  = PNIL; // GreatGrandParen()
    path[1]  = PNIL; // GrandParent()
    path[2]  = PNIL; // Parent()
//    path_ptr = 2;
//    root     = NIL;
//    current  = root;        // Current()
}

void ResetPath()
{
    path_ptr = 2;
    path[path_ptr] = PNIL;
    current = root;
}

void PushPath( const bool side )
{
    const Int P = current;
    path[++path_ptr] = ToPathNode(P,side);
    current = Child(P,side);
}

void PopPath()
{
    current = NodeFromPathNode(path[path_ptr--]);
}

public:

Int Current() const { return current; }

std::pair<Int,bool> ParentData() const
{
    return FromPathNode(path[path_ptr]);
}

Int Parent() const
{
    return NodeFromPathNode(path[path_ptr]);
}

bool ParentSide() const
{
    return SideFromPathNode(path[path_ptr]);
}


Int GrandParent() const
{
    return NodeFromPathNode(path[path_ptr-Int{1}]);
}

bool GrandParentSide() const
{
    return SideFromPathNode(path[path_ptr-Int{1}]);
}

std::pair<Int,bool> GrandParentData() const
{
    return FromPathNode(path[path_ptr-Int{1}]);
}


Int GreatGrandParent() const
{
    return NodeFromPathNode(path[path_ptr-Int{2}]);
}

bool GreatGrandParentSide() const
{
    return SideFromPathNode(path[path_ptr-Int{2}]);
}

std::pair<Int,bool> GreatGrandParentData() const
{
    return FromPathNode(path[path_ptr-Int{2}]);
}

Int Sibling() const
{
    const UInt P = path[path_ptr];
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return Child(N, !side);
}

Int Uncle() const
{
    const UInt P = path[path_ptr-Int{1}];
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return Child(N, !side);
}

Int GreatUncle() const
{
    const UInt P = path[path_ptr-Int{2}];
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return Child(N, !side);
}

private:

template<bool side>
void WalkToEnd()
{
    while( Current() != NIL ) { PushPath(side); }

    if( path[path_ptr] != PNIL ) { PopPath(); }
}
