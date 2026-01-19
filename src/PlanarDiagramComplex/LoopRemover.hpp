#pragma once

namespace Knoodle
{
    template<typename Int_>
    class LoopRemover
    {
    public:
        
        using Int  = Int_;
        
        using PDC_T = PlanarDiagramComplex<Int>;
        using PD_T  = PDC_T::PD_T;
    
        static constexpr Int  Uninitialized = PD_T::Uninitialized;
        static constexpr bool Tail = PD_T::Tail;
        static constexpr bool Head = PD_T::Head;
        
    private:
        
        PDC_T & pdc;
        PD_T  & pd;

        Int  a      = Uninitialized;
        bool d      = Head;

        Int  c_0    = Uninitialized;
        Int  c_1    = Uninitialized;
        Int  a_prev = Uninitialized;
        Int  a_next = Uninitialized;
        
    public:
        
        // Constructor from arc and direction.
        LoopRemover( PDC_T & pdc_, PD_T & pd_, const Int arc, const bool direction )
        :   pdc       { pdc_      }
        ,   pd        { pd_       }
        ,   a         { arc       }
        ,   d         { direction }
        {}
        
        // Constructor from darc and direction.
        LoopRemover( PDC_T & pdc_, PD_T & pd_, const Int da )
        :   pdc       { pdc_      }
        ,   pd        { pd_       }
        {
            std::tie(a,d) = PD_T::FromDarc(da);
        }
        
        bool Direction() const
        {
            return d;
        }
        
        Int DeactivatedCrossing() const
        {
            return c_0;
        }
        
        Int DeactivatedArc0() const
        {
            return a_prev;
        }
        Int DeactivatedArc1() const
        {
            return a_next;
        }
        
        Int CurrentArc() const
        {
            return a;
        }
        
        void LoadArc( const Int arc, const bool direction )
        {
            a = arc;
            d = direction;
        }
        
        bool Step()
        {
            if( a == Uninitialized )
            {
                // Signal major halt.
                return false;
            }
            
            if( !pd.ArcActiveQ(a) )
            {
                // Signal major halt.
                a = Uninitialized;
                return false;
            }
            
            c_0 = pd.A_cross(a,!d);
            c_1 = pd.A_cross(a, d);
            
            if( c_0 != c_1 )
            {
                // Signal major halt.
                a = Uninitialized;
                return false;
            }
            
//            nprint(MethodName("Step")+": Removing a loop arc at " + pd.ArcString(a) + ".");
            
            a_prev = pd.NextArc(a,!d,c_0);
            a_next = pd.NextArc(a, d,c_1);
            
            if( a_prev == a_next )
            {
                // We found an 8-shaped unlink.
                pd.DeactivateArc(a_prev);
                pd.DeactivateArc(a);
                pd.DeactivateCrossing(c_0);
                pdc.CreateUnlinkFromArc(pd,a);
                a_next = a;          // Store a in a_next so that it can be looked up later.
                a = Uninitialized;   // Signal to stop iterating.
                return true;         // Signal that something has changed.
            }
            else
            {
                // We found an ordinary loop that can be removed.
                pd.Reconnect(a_next,!d,a_prev); // Keep a_next alive.
                pd.DeactivateArc(a);
                pd.DeactivateCrossing(c_0);
                std::swap(a,a_next); // Signal to continue iterating by setting a to a_next.
                                     // Store a in a_next so that it can be looked up later.
                return true;
            }
        }
        
    public:
        
    /*!@brief Returns a string that identifies a class method specified by `tag`. Mostly used for logging and in error messages.
     */
        
        static std::string MethodName( const std::string & tag )
        {
            return ClassName() + "::" + tag;
        }
        
    /*!@brief Returns a string that identifies this class with type information. Mostly used for logging and in error messages.
    */
        
        static std::string ClassName()
        {
            return ct_string("LoopRemover")
                + "<" + TypeName<Int>
                + ">";
        }
        
    }; // LoopRemover
}

