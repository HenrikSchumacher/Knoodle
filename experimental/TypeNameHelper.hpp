namespace Tools
{
    
    //    // default implementation
    template<typename T>
    struct TypeNameHelper
    {
        static constexpr std::string name() { return "UnknownType"; }
    };
    
    template<typename T>
    constexpr const std::string newTypeName = TypeNameHelper<T>::name();
    
    
    template<> constexpr std::string TypeNameHelper<Int64>::name()
    {
        return "Int64";
    }
    
    template<> constexpr std::string TypeNameHelper<Int128>::name()
    {
        return "Int128";
    }
    
    template<int limb_count, UnsignedIntQ Limb_T, UnsignedIntQ Comp_T, bool signQ>
    struct TypeNameHelper<typename Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>>
    {
        using Class_T = Knoodle::WideInt<limb_count,Limb_T,Comp_T,signQ>;
        
        static constexpr std::string name()
        {
            return Class_T::ClassName();
        }
    };
    
} // namespace Tools
