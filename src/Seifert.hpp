#pragma once

#include "MultiGraph.hpp"
#include "../submodules/Tensors/Sparse.hpp"
//#include "../submodules/Tensors/src/Sparse/ApproximateMinimumDegree.hpp"
//#include "../submodules/Tensors/src/Sparse/Metis.hpp"

namespace Knoodle
{
    
    template<typename Scal_, typename Int_>
    class Seifert final
    {
        
    public:
        
        using Scal = Scal_;
        using Real = Scalar::Real<Scal>;
        using Int  = Int_;
//        using LInt = LInt_;
        
        static_assert(SignedIntQ<Int>,"");

                
//        using SparseMatrix_T    = Sparse::MatrixCSR<Scal,Int,LInt>;
//        using BinaryMatrix_T    = Sparse::BinaryMatrixCSR<Int,LInt>;
        
        
//        using Helper_T = Tensor2<Scal,LInt>;
        
//        using Factorization_T   = Sparse::CholeskyDecomposition<Scal,Int,LInt>;
//        using Factorization_Ptr = std::shared_ptr<Factorization_T>;
        using PD_T              = PlanarDiagram<Int>;
//        using Aggregator_T      = TripleAggregator<Int,Int,Scal,LInt>;
        
        using SeifertIncidenceMatrix_T = Sparse::MatrixCSR<Int,Int,Int>;
        
        using Graph_T = MultiGraph<Int>;
    
        static constexpr bool Tail  = PD_T::Tail;
        static constexpr bool Head  = PD_T::Head;
        static constexpr bool Left  = PD_T::Left;
        static constexpr bool Right = PD_T::Right;
        static constexpr bool Out   = PD_T::Out;
        static constexpr bool In    = PD_T::In;
    
        
        Seifert( const Int sparsity_threshold_ )
        :   sparsity_threshold (
                std::min(
                    sparsity_threshold_,
                    static_cast<Int>(std::floor(std::sqrt(std::abs(std::numeric_limits<Int>::max()))))
                )
            )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
      
        // Default constructor
        Seifert()
        :   sparsity_threshold ( 64 )
        ,   LU_buffer ( sparsity_threshold * sparsity_threshold )
        ,   LU_perm   ( sparsity_threshold )
        {}
        
        // Destructor
        ~Seifert() = default;
        // Copy constructor
        Seifert( const Seifert & other ) = default;
        // Copy assignment operator
        Seifert & operator=( const Seifert & other ) = default;
        // Move constructor
        Seifert( Seifert && other ) = default;
        // Move assignment operator
        Seifert & operator=( Seifert && other ) = default;
        
    private:
        
        Int sparsity_threshold;
        
        mutable Tensor1<Scal,Int> LU_buffer;
        
        mutable Tensor1<LAPACK::Int,Int> LU_perm;
        
    private:
  
        void RequireSeifertGraph( cref<PD_T> pd  ) const
        {
            const Int n = pd.CrossingCount();
            const Int m = pd.ArcCount();
            
            auto & C_arcs  = pd.Crossings();
            auto & A_cross = pd.Arcs();
            
            typename Graph_T::EdgeContainer_T C_disks ( n, 2 );
            
            Tensor1<SInt,Int> A_flag  ( m, 0 );
            
            // s_rp also includes a counter to for the number of Seifert disks.
            Aggregator<Int,Int> s_rp ( m    );
            s_rp.Push( 0 );
            
            Aggregator<Int,Int> s_ci ( 2 * n ); // Every crossing appears exactly twice a vertex of a Seifert disk.
            Aggregator<Int,Int> s_val( 2 * n ); // Every crossing appears exactly twice a vertex of a Seifert disk.
            
            Int a_ptr = 0;
            
            // TODO: Use TraverseWithCrossings
            logprint("Start while loop.");
            while( a_ptr < m )
            {
                while( (a_ptr < m) && (A_flag[a_ptr] != 0) )
                {
                    ++a_ptr;
                }
                
                if( a_ptr == m )
                {
                    break;
                }
                
                Int a = a_ptr;
                
                do
                {
                    A_flag[a] = 1;
                    
                    const Int c = A_cross(a,Head);
                    
                    Int C [2][2];
                    copy_buffer<4>( C_arcs.data(c), &C[0][0] );
                    
                    const bool rightQ = (C[In][Right] == a);
                    
                    s_ci.Push(c);
                    s_val.Push(rightQ ? -1 : 1);
                    
                    a = C[0][rightQ];
                    C_disks(c,rightQ) = s_rp.Size()-1;
                    
                }
                while( a != a_ptr );
                
                s_rp.Push( s_ci.Size() );
            }
            
            logprint("Finished while loop.");

            SeifertIncidenceMatrix_T S (
                std::move(s_rp.Get()),
                std::move(s_ci.Get()),
                std::move(s_val.Get()),
                s_rp.Size()-1, n, Int(1)
            );
            
//            S.SortInner();
            
            pd.SetCache( std::string("SeifertIncidenceMatrix"), std::move(S) );
            
            Graph_T G ( s_rp.Size()-1, std::move(C_disks) );
            
            pd.SetCache( std::string("SeifertGraph"), std::move(G) );
                    
        }
        
        
    public:
        
        cref<SeifertIncidenceMatrix_T> SeifertIncidenceMatrix( cref<PD_T> pd  ) const
        {
            std::string tag ("SeifertIncidenceMatrix");
            
            if( !pd.InCacheQ(tag) )
            {
                RequireSeifertGraph(pd);
            }
            
            return pd.template GetCache<SeifertIncidenceMatrix_T>(tag);
        }
        
        cref<Graph_T> SeifertGraph( cref<PD_T> pd ) const
        {
            std::string tag ("SeifertGraph");
            
            if( !pd.InCacheQ(tag) )
            {
                RequireSeifertGraph(pd);
            }
            
            return pd.template GetCache<Graph_T>(tag);
        }
        
    public:
        
        static std::string ClassName()
        {
            return ct_string("Seifert")
                + "<" + TypeName<Scal>
                + "," + TypeName<Int>
                + ">";
        }
        
    }; // class Alexander
    
    
} // namespace Knoodle
