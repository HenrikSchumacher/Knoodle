#pragma  once

namespace KnotTools
{
    
    template<typename Int>
    class alignas( ObjectAlignment ) OrientedLinkDiagram
    {
    public:
        ASSERT_INT(Int)
        
        // Specify local typedefs within this class.

        using CrossingContainer_T      = Tiny::MatrixList<2,2,Int,Int>;
        using CrossingSignContainer_T  = std::vector<bool>;
        
    protected:
        
        // It is my habit to make all data members private/protected and to provided accessor references to only those members that the use is allowed to manipulate.
        
        // We try to make access to stored data as fast as possible by using a Structure of Array (SoA) data layout. That crossing data is stored in the heap-allocated array C_arc_cont of size 2 x 2 x crossing_count. The arc data is stored in the heap-allocated array A_cross_cont of size 2 x arc_count.
        
        CrossingContainer_T     C_cont;
        CrossingSignContainer_T C_sign;

        // The constructor will store pointers to the data in the following pointers. We do so because fixed-size array access is no indirection.
        
        Int * restrict C [2][2] = {{nullptr}};

        // Provide class members for the container sizes as convenience for loops.
        // For example compared to
        //
        //     for( Int i = 0; i < C_arc_cont.Dimension(2); ++i )
        //
        // this
        //
        //     for( Int i = 0; i < crossing_count; ++i )
        //
        // makes it easier for the compiler to figure out that the upper end of the loop does not change.
        
        Int crossing_count = 0;

    public:
        
        OrientedLinkDiagram() = default;
        
        ~OrientedLinkDiagram() = default;
        
        // This constructor is supposed to allocate all relevant buffer.
        // Data has to be filled in manually by using the references provided by Crossings() and Arcs() method.
        OrientedLinkDiagram( const Int crossing_count_ )
        :   C_cont        ( crossing_count_  )
        ,   C_sign        ( crossing_count_  )
        ,   C {
                {C_cont.data(0,0), C_cont.data(0,1)},
                {C_cont.data(1,0), C_cont.data(1,1)}
            }
        ,   crossing_count ( crossing_count_ )
        {}
        
        // Copy constructor
        OrientedLinkDiagram( const OrientedLinkDiagram & other )
        :   C_cont       ( other.C_cont )
        ,   C_sign       ( other.C_sign )
        ,   C {
                {C_cont.data(0,0), C_cont.data(0,1)},
                {C_cont.data(1,0), C_cont.data(1,1)}
            }
        ,   crossing_count ( other.crossing_count )
        {}
        
        friend void swap(OrientedLinkDiagram &A, OrientedLinkDiagram &B) noexcept
        {
            // see https://stackoverflow.com/questions/5695548/public-friend-swap-member-function for details
            using std::swap;
            
            swap( A.C_cont , B.C_cont  );
            swap( A.C_sign , B.C_sign  );
            swap( A.C[0][0], B.C[0][0] );
            swap( A.C[0][1], B.C[0][1] );
            swap( A.C[1][0], B.C[1][0] );
            swap( A.C[1][1], B.C[1][1] );
            
            swap( A.crossing_count, B.crossing_count );

        }
        
        // Move constructor
        OrientedLinkDiagram( OrientedLinkDiagram && other ) noexcept
        :   OrientedLinkDiagram()
        {
            swap(*this, other);
        }

        /* Copy assignment operator */
        OrientedLinkDiagram & operator=( const OrientedLinkDiagram & other )
        {
            // Use the copy constructor.
            swap( *this, OrientedLinkDiagram(other) );
            return *this;
        }

        /* Move assignment operator */
        OrientedLinkDiagram & operator=( OrientedLinkDiagram && other ) noexcept
        {
            swap( *this, other );
            return *this;
        }
        
        
        
        
        
        Int CrossingCount() const
        {
            return crossing_count;
        }
   
        
        CrossingContainer_T & Crossings()
        {
            return C_cont;
        }
        
        const CrossingContainer_T& Crossings() const
        {
            return C_cont;
        }
        
        CrossingSignContainer_T & CrossingSigns()
        {
            return C_sign;
        }
        
        const CrossingSignContainer_T & CrossingSigns() const
        {
            return C_sign;
        }
        
        
        //Modifiers
        
    public:
        
        std::string CrossingString( const Int c )
        {
            return "crossing " +ToString(c) +" = { { " +
               ToString(C[Out][Left ][c])+", "+ToString(C[Out][Right][c])+" }, { "+
               ToString(C[In ][Left ][c])+", "+ToString(C[In ][Right][c])+" } } ";
        }


        
    private:
        
//        Int NextCrossing( const Int c, const Int from_c )
//        {
//            const bool lr = (C[_in][Left][c] == from_c) ? Left : Right;
//
//            return C[Out][lr][c];
//        }
        
        

    
    public:
        
        static std::string ClassName()
        {
            return "OrientedLinkDiagram<"+TypeName<Int>::Get()+">";
        }
        
    };
    
} // namespace KnotTools
