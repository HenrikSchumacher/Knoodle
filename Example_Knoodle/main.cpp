#include "../Knoodle.hpp"

using Int         = std::int64_t;                        // integer type used, e.g., for indices
using PDC_T       = Knoodle::PlanarDiagramComplex<Int>;
using PD_T        = PDC_T::PD_T;
using OrthoDraw_T = Knoodle::OrthoDraw<PD_T>;

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
        c_count          // number of crossingss
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
//        // Print PD code to command line.
//        Tools::valprint( "PD code (unsimplified)", pd.PDCode() );
//
        std::string filename ( path / "Diagram_unsimplified.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        // Create an orthogonal layout for the currect knot diagram.
        OrthoDraw_T H ( pd_0, Int(-1), plot_settings );
        // If you are curious: the second argument here specifies which face is to be come the external face. nonnegative number specify the index of the face; -1 means "take a face with maximum number of edges.

        // Write an ASCII art version of the diagram to file.
        std::ofstream( filename ) << H.DiagramString();
    }
    
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Simplify ==" );
    Tools::print( "Simplify applies pass moves to the ." );
    {
        std::string filename ( path / "Diagram_Simplify5.txt" );
        Tools::print( "Writing diagrams to file " + filename + "." );
        
        /*!For the simplification we have to load the PlanarDiagram into an instance of PlanarDiagramComplex.
         * Tthis is a data struture that can hold the many PlanarDiagrams that can be disconnected (in the sense of connected sum) or split off (in the sense of split link) during simplification.
         * PlanarDiagramComplex also stores the prime link decomposition by using color information for each arc to indicate to which link component of the orginal diagram an arc belongs.
         */
        
        PDC_T pdc { PD_T(pd_0) }; // Creating a copy of pd here because PDC_T wants ownership. (Typically, to save memory.)
        
        pdc.Simplify();
        
        TOOLS_DUMP(pdc.PDCode());
        
        std::ofstream file ( filename );
        
        std::size_t iter = 0;
        for( const PD_T & pd : pdc.Diagrams() )
        {
            Tools::print( "Connected component no. " + Tools::ToString(iter) + ":" );
            Tools::valprint( "No. of crossings", pd.CrossingCount() );
            // Print PD code to command line.
            Tools::valprint( "PD code", pd.PDCode() );
            
            // Create an orthogonal layout for the currect knot diagram.
            OrthoDraw_T H ( pd, Int(-1), plot_settings );

            // Write an ASCII art version of the diagram to file.
            file << H.DiagramString();
            file << "\n";
            
            Tools::print(H.DiagramString());
            Tools::print("");
            
            ++iter;
        }
    }
    
    Tools::print( "" );
    Tools::print( "== Demonstration of Reapr ==" );
    Tools::print( "Reapr applies Simplify and then make a number of attempts to embed, randomly rotate, project, and run Simplify again. Often, the output of Simplify is already as good as it gets. You need a fairly hard knot for Reapr to make a difference. Note that Reapris _not_ deterministic.." );
    {
        std::string filename ( path / "Diagram_Reapr.txt" );
        Tools::print( "Writing diagram to file " + filename + "." );
        
        PDC_T pdc { PD_T(pd_0) }; // Creating a copy of pd here because PDC_T wants ownership. (Typically, to save memory.)
        
        pdc.Simplify({.embedding_trials = 3, .rotation_trials = 20});
        
        std::ofstream file ( filename );
        
        for( Int i = 0; i < pdc.DiagramCount(); ++i )
        {
            const PD_T & pd = pdc.Diagram(i);
            
            Tools::print( "Connected component no. " + Tools::ToString(i) + ":" );
            
            Tools::valprint( "No. of crossings",  pd.CrossingCount() );
            
            // Create an orthogonal layout for the current knot diagram.
            OrthoDraw_T H ( pd, Int(-1), plot_settings );
            
            // Print PD code to command line.
            Tools::valprint( "PD code", pd.PDCode() );
            
            // Write an ASCII art version of the diagram to file.
            file << H.DiagramString();
            file << "\n";
            
            Tools::print(H.DiagramString());
            Tools::print("");
        }
    }

    return EXIT_SUCCESS;
}
