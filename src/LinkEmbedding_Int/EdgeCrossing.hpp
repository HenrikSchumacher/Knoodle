#pragma once

namespace Knoodle
{
    /*!@brief An wrapper type for an integral type to store and index of the crossing of an edge-crossing pair along with two further bits of information: the handedness of the crossings and whether that edge goes over.
     *
     * Since bit-twiddling is a bit brittle, we let this class ship its one accesors. 
     */
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
        
        template<SignedIntQ Sign_T = FastInt8>
        Sign_T Handedness() const
        {
            return RightHandedQ() ? Sign_T{1} : Sign_T{-1};
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
        
        friend auto operator<=>( const EdgeCrossing & a, const EdgeCrossing & b )
        {
            return (a.data <=> b.data);
        }
        
        friend bool operator==( const EdgeCrossing & a, const EdgeCrossing & b )
        {
            return (a.data == b.data);
        }
        
    }; // class EdgeCrossing
    
} // namespace Knoodle
