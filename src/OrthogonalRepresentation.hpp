#pragma once

namespace Knoodle
{
    template<typename Int_>
    class OrthogonalRepresentation
    {
    public:
        
        using Int = Int_;
        
        using EdgeContainer_T = Tensor2<Int,Int>;
        
        
        OrthogonalRepresentation() = default;
        
        
        OrthogonalRepresentation(
            PlanarDiagram<Int> & pd, const Int exterior_face
        )
        {
            // TODO: Compute the bends for each arc.
            Tensor1<Int,Int> bends = BendsByLP(pd);
            
            Int bend_count = bends.Total();
            
            edge_count = pd.ArcCount() + bend_count;
            
            edges = EdgeContainer_T( edge_count );
            
            // TODO: Load arcs from a PlanarDiagram.
            
            // TODO: Split each arc into edges by inserting a vertex for every bend.
            // TODO: Remember which crossings are mapped to which vertices.
            // TODO: Remember which arcs are split into which edges.
            // TODO: Store turn between each edge and its "next left" edge.
            
            // TODO: Store turn between each edge and its "next left" edge.
        }
                                 
        ~OrthogonalRepresentation() = default;
        
    private:
        
        Int edge_count = 0;
        Int edge_count = 0;
        
        Tensor2<Int,Int> V_E;
        EdgeContainer_T E;
        
        Tensor1<Int,Int> E_next_left_E;
        Tensor1<Int,Int> E_turn;
        
        Tensor1<Int,Int> C_to_V;
        Tensor1<Int,Int> E_to_A; // to _oriented_ arcs?
    
    private:
        
#include "OrthogonalRepresentation/BendsByLP.hpp"
        
    public:
        
        OrthogonalRepresentation RegularizeTurnsByKittyCorners()
        {
            // TODO: Compute the faces.
            // TODO: Check faces for turn-regularity/detect kitty-corners.
            // TODO: Resolve kitty-corners.
            //      TODO: Do that horizontally/vertically in alternating fashion.
            //      TODO: Do that randomly.
            // TODO: Update and forward the map from original crossings to vertices.
            // TODO: Update and forward the map from edges to original arcs.
        }
  
//        Not that important:
//        OrthogonalRepresentation RegularizeTurnsByReactangles()
//        {
//            // TODO: Compute the faces.
//            // TODO: Check faces for turn-regularity/detect kitty-corners.
//            // TODO: Resolve by using rectangles.
//            // TODO: Update and forward the map from original crossings to vertices.
//            // TODO: Update and forward the map from edges to original arcs.
//        }
        
    public:
        
        static std::string ClassName()
        {
            return "OrthogonalRepresentation<" + TypeName<Int> + ">";
        }
        
    } // class OrthogonalRepresentation
    
} // namespace Knoodle
