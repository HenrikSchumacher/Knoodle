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
    Path(path_ptr) = PNIL;
    current = root;
//    
//    // DEBUGGING
//    fill_buffer( &path[0], PNIL, max_path_size + Int{2});
}

template<bool verboseQ = false>
void PushPath( const bool side )
{
    if constexpr ( verboseQ )
    {
        logprint(MethodName("PushPath(" + (side ? std::string("Right") : std::string("Left")) + ")"));
        
        logvalprint("path_ptr",path_ptr);
    }
    
    if constexpr ( bound_checksQ )
    {
        if ( current == NIL )
        {
            wprint(MethodName("PushPath") + ": current == NIL.");
        }
        
        if( path_ptr >= max_path_size )
        {
            std::string msg = MethodName("PushPath") + ": Path overflow.";
            eprint(msg);
            logvalprint("max_path_size",max_path_size);
            logvalprint("path_ptr",path_ptr);
            logvalprint("path",
                std::string(OutString::FromVector(&path[0],max_path_size))
            );
            
            throw std::runtime_error(msg);
        }
    }
    
    const Int parent = current;
    Path(++path_ptr) = ToPathNode(parent,side);
    current = GetChild(parent,side);
    
    if constexpr ( verboseQ )
    {
        TOOLS_LOGDUMP(current);
    }
    
    if constexpr ( bound_checksQ )
    {
        if( parent == current )
        {
            eprint(MethodName("PushPath") + ": parent == current = " + ToString(parent) + ".");
            
            TOOLS_LOGDUMP(GetState(parent));
            TOOLS_LOGDUMP(GetChild(parent,Left ));
            TOOLS_LOGDUMP(GetChild(parent,Right));
            TOOLS_LOGDUMP(GetData(parent));
        }
    }
}

void PopPath()
{
    if constexpr ( bound_checksQ )
    {
        if( path_ptr < Int{2} )
        {
            std::string msg = MethodName("PopPath") + ": Path underflow.";
            eprint(msg);
            logvalprint("path_ptr",path_ptr);
            
            throw std::runtime_error(msg);
        }
    }
    current = NodeFromPathNode(Path(path_ptr--));
}

public:

Int Current() const { return current; }

std::pair<Int,bool> ParentData() const
{
    return FromPathNode(Path(path_ptr));
}

Int Parent() const
{
    return NodeFromPathNode(Path(path_ptr));
}

bool ParentSide() const
{
    return SideFromPathNode(Path(path_ptr));
}


Int GrandParent() const
{
    return NodeFromPathNode(Path(path_ptr-Int{1}));
}

bool GrandParentSide() const
{
    return SideFromPathNode(Path(path_ptr-Int{1}));
}

std::pair<Int,bool> GrandParentData() const
{
    return FromPathNode(Path(path_ptr-Int{1}));
}


Int GreatGrandParent() const
{
    return NodeFromPathNode(Path(path_ptr-Int{2}));
}

bool GreatGrandParentSide() const
{
    return SideFromPathNode(Path(path_ptr-Int{2}));
}

std::pair<Int,bool> GreatGrandParentData() const
{
    return FromPathNode(Path(path_ptr-Int{2}));
}

Int Sibling() // const
{
    const UInt P = Path(path_ptr);
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return GetChild(N, !side);
}

Int Uncle() // const
{
    const UInt P = Path(path_ptr-Int{1});
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return GetChild(N, !side);
}

Int GreatUncle() // const
{
    const UInt P = Path(path_ptr-Int{2});
    if(P == PNIL) return NIL;
    auto [N,side] = FromPathNode(P);
    return GetChild(N, !side);
}

private:

void WalkToEnd( const bool side )
{
    while( Current() != NIL ) { PushPath(side); }

    if( Path(path_ptr) != PNIL ) { PopPath(); }
}


UInt & Path( const Int i )
{
    if constexpr ( bound_checksQ )
    {
        if( (i < Int{0}) || (i >= max_path_size) )
        {
            eprint(this->MethodName("Path") + ": index = " + ToString(i)+ " is out of bounds.");
            return dummy_path;
        }
    }
    return path[i];
}

const UInt & Path( const Int i ) const
{
    if constexpr ( bound_checksQ )
    {
        if( (i < Int{0}) || (i >= max_path_size) )
        {
            eprint(this->MethodName("Path") + ": index = " + ToString(i)+ " is out of bounds.");
            return dummy_path;
        }
    }
    return path[i];
}
