

private:
    
//        void Deactivate( Int node )
//        {
//            if( node == NIL ) { return; }
//
//            node_buffer[node].state = State_T::Inactive;
//            deleted_nodes.Push(node);
//        }
    
//        template<typename F, typename C>
//        void Delete( cref<Data_T> data, F && f, C && cmp )
//        {
//            auto [parent, node] = Find( data, std::forward<F>(f), std::forward<C>(cmp) );
//
//            if( node == NIL ) { return; }
//
//            const bool leftQ  = (Child(node,Left ) != NIL);
//            const bool rightQ = (Child(node,Right) != NIL);
//
//            if( leftQ )
//            {
//                if( parent != NIL )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = Child(node,Left );
//                }
//                Deactivate(node);
//
//            }
//            else if( rightQ )
//            {
//                if( parent != NIL )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = Child(node,Right);
//                }
//                Deactivate(node);
//            }
//            else if( leftQ && rightQ )
//            {
//                // Node `node` has two children.
//                // Not so obvious what to do next!
//
//                auto [prev_parent,prev] = FindMax( Child(node,Left) );
//                // v and prev cannot be NIL.
//                {
//                    const bool side = (Child(prev_parent,Right) == prev);
//                    Child(prev_parent,side) = Child(prev,Left);
//                }
//
//                if( parent != NIL )
//                {
//                    const bool side = (Child(parent,Right) == node);
//                    Child(parent,side) = prev;
//                }
//
//                Deactivate(node);
//            }
//            else
//            {
//                if( parent != Uninitialized )
//                {
//                    const bool side = (Child(parent,Right) == node)
//                    Child(parent,side) = NIL;
//                }
//                Deactivate(node);
//            }
//        }
