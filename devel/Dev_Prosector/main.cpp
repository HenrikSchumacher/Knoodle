//#define TOOLS_NO_RESTRICT

#define TOOLS_NO_INT128

//#define KNOODLE_USE_BOOST_MULTIPRECISION


#include "../../Knoodle.hpp"
//#include "../../experimental/LinkEmbedding_Boost.hpp"
#include "../../experimental/LinkEmbedding3.hpp"

using namespace Knoodle;
using namespace Tools;

using Real  = Real64;
using Int   = Int64;
using IReal = Int64;


//using Link2_T         = LinkEmbedding_Boost<Real,Int>;
using Link3_T         = LinkEmbedding3<Real,Int,IReal>;
using Link4_T         = LinkEmbedding4<Real,Int,IReal>;

//using Prosector_T    = Link_T::Prosector_T;
//using Vector3_T      = Prosector_T::Vector3_T;
//using Intersection_T = Prosector_T::Intersection;
//using Flag_T         = Prosector_T::Flag_T;

using PDC_T          = PlanarDiagramComplex<Int>;



std::string_view f( std::string_view view )
{
    return view;
}

template<typename A_T, typename B_T>
void fun( A_T a, B_T b )
{
    auto c = long_mul(a,b);
    print(std::string(TypeName<decltype(a)>) + " * " + TypeName<decltype(b)> + " -> " + TypeName<decltype(c)>);
    print("a = " + ToString(a) + ", b = " + ToString(b) + ", c = " + ToString(c));
}

int main()
{
    Profiler::Clear();
    Link4_T L = Link4_T::FromFile( HomeDirectory() / "a.txt");
    TOOLS_DUMP(L.EdgeCount());
    auto err = L.RequireIntersections();
    TOOLS_DUMP(err);
    
    PDC_T pdc (L);
    
    print(PDC_T::ClassName());
    print(PDC_T::MethodName(std::string("aaaa")));
    print(PDC_T::MethodName("bbbb"));
    print(PDC_T::MethodName(ct_string{"cccc"}));
    
//    OutString s;
//    
//    s << L.EdgePointers();
//    s << "\n\n";
//    s << L.EdgeCoordinates();
//    s << "\n\n";
//    s << L.BoundingBoxes();
//    
//    print(s);
//    
    {
        Tensor1<Real,Int> a ( 4 );
        a.Randomize();
        TOOLS_DUMP(a);
        
        OutString s_out;
        
        s_out << a;
        
        valprint("s_out",s_out);
        
        InString s_in { s_out.View() };
        
        Tensor1<Real,Int> b ( 4 );
        
        s_in >> b;
        
        TOOLS_DUMP(b);
    }
    
    {
        Tensor2<Real,Int> a ( 4, 4 );
        a.Randomize();
        TOOLS_DUMP(a);
        
        OutString s_out;
        
        s_out << a;
        
        valprint("s_out",s_out);
        
        InString s_in { s_out.View() };
        
        Tensor2<Real,Int> b ( 4, 4 );
        
        s_in >> b;
        
        TOOLS_DUMP(b);
    }
    
    {
        Tensor3<Real,Int> a ( 2, 2, 2 );
        a.Randomize();
        TOOLS_DUMP(a);
        
        OutString s_out;
        
        s_out << a;
        
        valprint("s_out",s_out);
        
        InString s_in { s_out.View() };
        
        Tensor3<Real,Int> b ( 2, 2, 2 );
        
        s_in >> b;
        
        TOOLS_DUMP(b);
    }
    
//    TOOLS_DUMP(CharCount("a","bb"));
//    
//    constexpr auto cts = ct_string("a");
//    
//    constexpr auto cts_1 = "c" + cts + "b" + to_ct_string(1);
//    
//    print(cts);
//    print(cts_1);
//    print("ccc");

    
    constexpr ct_string<8> s;
    
    std::cout << "|" << s << "|" << std::endl;
    
    TOOLS_DUMP(s[0]);
    TOOLS_DUMP(s[1]);
    TOOLS_DUMP(s[2]);
    
    TOOLS_DUMP(s.size());

    constexpr auto a = "\"" + s + "\"";
    valprint("a",std::string_view(a));
    TOOLS_DUMP(a.size());
//    
//    print(IntegerInfo());
//    
//    print(FullTypeName<std::pair<Int,Real>>);
    
//    constexpr auto a = ct_string("abc");
//    TOOLS_DUMP(a.size());
//    TOOLS_DUMP(a.capacity());
    
    print(IntegerInfo());
}
