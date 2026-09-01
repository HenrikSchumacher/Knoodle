#pragma once

namespace Knoodle
{
    template<IntQ Int>
    class EdgeCrossing
    {
    private:
        
        Int data;
        
    public:
        
        EdgeCrossing( Int raw )
        :   data { raw }
        {}
        
        EdgeCrossing( Int k, bool right_handedQ, bool overQ )
        :   data{ (k << 2) | (Int{right_handedQ} << 1) | Int{overQ} }
        {}
        
        Int Index() const
        {
            return (data >> 2);
        }
        
        bool RightHandedQ() const
        {
            return (data & Int{2});
        }
        
        bool OverQ() const
        {
            return (data & Int{1});
        }
        
        std::tuple<Int,bool,bool> Decompose() const
        {
            return { Index(), RightHandedQ(), OverQ() };
        }
        
        template<IntQ ExtInt = Int>
        inline friend ExtInt ToInt( const EdgeCrossing & c )
        {
            return static_cast<ExtInt>(c.data);
        }
        
    }; // class EdgeCrossing
    
} // namespace Knoodle
