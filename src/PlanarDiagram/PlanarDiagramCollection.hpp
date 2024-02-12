#pragma  once

//https://larsgeb.github.io/2022/04/20/m1-gpu.html

namespace KnotTools
{
    
    template<typename Int>
    class PlanarDiagramCollection
    {
    public:

        ASSERT_INT(Int)
        
        // Specify local typedefs within this class.

        using PlanarDiagram_T = PlanarDiagram<Int>;
        
    protected:
        
        Int diagram_count           = 0;
        Int unlink_count            = 0;
        Int component_count         = 0;
        Int connected_sum_count     = 0;
        
        std::vector<PlanarDiagram_T> diagrams;
    
    public:
        
        static std::string ClassName()
        {
            return "PlanarDiagramCollection<"+TypeName<Int>+">";
        }
        
    };
    
} // namespace KnotTools
