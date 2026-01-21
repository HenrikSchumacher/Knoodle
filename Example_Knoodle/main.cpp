#include "../Knoodle.hpp"
#include "../src/OrthoDraw.hpp"
#include "../Reapr.hpp"


using Int         = std::int64_t;                 // integer type used, e.g., for indices
using PD_T        = Knoodle::PlanarDiagram<Int>;
using OrthoDraw_T = Knoodle::OrthoDraw<Int>;
using Reapr_T     = Knoodle::Reapr<double,Int>;

int main()
{
    Tools::print( "-=| An example program for Knoodle. |=-" );
    Tools::print( "" );
 
    std::filesystem::path path = std::filesystem::path(__FILE__).parent_path();
  
    Tools::valprint("working directory", path );
    
    // Load a knot from file.
    std::vector<Int> pd_code;
    
    {
        std::string filename ( path / "ExampleKnot.tsv" );
        std::ifstream s ( filename );
        Int number;
        
        if(!s)
        {
            Tools::eprint( "File " + filename + " not found." );
            return EXIT_FAILURE;
        }
        
        while( s )
        {
            if( s >> number )
            {
                pd_code.push_back(number);
            }
        }
    }
    
    Int c_count = static_cast<Int>( pd_code.size()/5 );
    
    // Create an instance of PlanarDiagram.
    PD_T pd_0 = PD_T::FromSignedPDCode(
        &pd_code[0],     // pointer to array of pd code.
        c_count,         // number of crossingss
        Int(0),          // number of unlinks contained in this link (as this is not part of traditional pd codes)
        false, false
    );
    
    // Graphics settings for ASCII art. (Move on, nothing to see here.)
    OrthoDraw_T::Settings_T plot_settings {
        .x_grid_size              = 8,
        .y_grid_size              = 4,
        .x_gap_size               = 1,
        .y_gap_size               = 1
    };

    Tools::valprint( "No. of crossings",  pd_0.CrossingCount() );

    {
        // Print PD code to command line.
        Tools::valprint( "PD code (unsimplified)", pd_0.PDCode() );

        std::string filename ( path / "Diagram_unsimplified.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        // Create an orthogonal layout for the currect knot diagram.
        OrthoDraw_T H ( pd_0, Int(-1), plot_settings );
        // If you are curious: the second argument here specifies which face is to be come the external face. nonnegative number specify the index of the face; -1 means "take a face with maximum number of edges.

        // Write an ASCII art version of the diagram to file.
        std::ofstream( filename ) << H.DiagramString();
    }
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Simplify3 ==" );
    Tools::print( "Simplify3 greedily performs Reidemeister I + II moves and simplifies some further local patterns." );
    {
        PD_T pd ( pd_0 );
        pd.Simplify3(); // Performs Reidemeister I + II moves and simplify some local pattern.
        Tools::valprint( "No. of crossings", pd.CrossingCount() );

        // Print PD code to command line.
        Tools::valprint( "PD code (Simplify3)", pd.PDCode() );
        
        std::string filename ( path / "Diagram_Simplify3.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        // Create an orthogonal layout for the currect knot diagram.
        OrthoDraw_T H ( pd, Int(-1), plot_settings );
        
        // Write an ASCII art version of the diagram to file.
        std::ofstream( filename ) << H.DiagramString();
    }
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Simplify4 ==" );
    Tools::print( "Simplify4 applies Simplify3 and then performs path rerouting (and further runs of Simplify3)." );
    {
        PD_T pd ( pd_0 );
        pd.Simplify4(); // Do Simplify3 and path rerouting.
        Tools::valprint( "No. of crossings", pd.CrossingCount() );
        
        // Print PD code to command line.
        Tools::valprint( "PD code (Simplify4)", pd.PDCode() );
        
        std::string filename ( path / "Diagram_Simplify4.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        // Create an orthogonal layout for the currect knot diagram.
        OrthoDraw_T H ( pd, Int(-1), plot_settings );
        
        // Write an ASCII art version of the diagram to file.
        std::ofstream( filename ) << H.DiagramString();
    }
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Simplify5 ==" );
    Tools::print( "Simplify5 applies Simplify4 and attempts to detect and split off the connected summands." );
    {
        std::string filename ( path / "Diagram_Simplify5.txt" );
        Tools::print( "Writing diagrams to file " + filename + "." );
        
        PD_T pd ( pd_0 );
        std::vector<PD_T> pd_list;
        pd.Simplify5(pd_list); // Do Simplify4 and detect connected summands.
        
        // Simplify5 splits and pushes newly found connected summands to pd_list.
        // It keeps the last summand in pd; we just push it manually to pd_list.
        pd_list.push_back(std::move(pd));
        
        std::ofstream file ( filename );
        
        std::size_t iter = 0;
        for( PD_T p : pd_list )
        {
            Tools::print( "Connected component no. " + Tools::ToString(iter) + ":" );
            Tools::valprint( "No. of crossings", p.CrossingCount() );
            // Print PD code to command line.
            Tools::valprint( "PD code", p.PDCode() );
            
            // Create an orthogonal layout for the currect knot diagram.
            OrthoDraw_T H ( p, Int(-1), plot_settings );

            // Write an ASCII art version of the diagram to file.
            file << H.DiagramString();
            file << "\n";
            
            Tools::print(H.DiagramString());
            Tools::print("");
            
            ++iter;
        }
    }
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Reapr (Rotate-embed and path reroute) ==" );
    Tools::print( "Reapr applies Simplify5 and then make a number of attempts to embed, randomly rotate, project, and run Simplify5 again. Typically, the output of Simplify5 is already as good as it gets. You need a fairly hard knot for Reapr to make a difference. In contrast to the other methods, this routine is _not_ deterministic.." );
    {
        std::string filename ( path / "Diagram_Reapr.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        Reapr_T reapr;

        Int target_iter = 20;
        
        // Make target_iter attempts to embed, rotate, and simplify.
        // The output of Simplify5 is often as good as it gets. You need a fairly hard knot for this to make a difference.
        std::vector<PD_T> pd_list = reapr.Rattle(pd_0,target_iter);
        
        std::ofstream file ( filename );
        
        std::size_t iter = 0;
        for( PD_T p : pd_list )
        {
            Tools::print( "Connected component no. " + Tools::ToString(iter) + ":" );
            
            Tools::valprint( "No. of crossings",  p.CrossingCount() );
            
            // Create an orthogonal layout for the currect knot diagram.
            OrthoDraw_T H ( p, Int(-1), plot_settings );
            
            // Print PD code to command line.
            Tools::valprint( "PD code", p.PDCode() );
            
            // Write an ASCII art version of the diagram to file.
            file << H.DiagramString();
            file << "\n";
            
            Tools::print(H.DiagramString());
            Tools::print("");
            
            ++iter;
        }
    }

    return EXIT_SUCCESS;
}
