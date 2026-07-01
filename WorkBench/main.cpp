#include "../Knoodle.hpp"

using namespace Knoodle;
using namespace Tools;

using Int = Size_T;



int main()
{
    constexpr Int n = 10;
    
    Knoodle::PRNG_T engine = InitializedRandomEngine<Knoodle::PRNG_T>();
    std::uniform_int_distribution<int> coin (0,1) ;
    
    // Create some array of booleans and fill it.
    bool a[n];
    for( Int i = 0; i < n; ++i )
    {
        a[i]  = static_cast<bool>(coin(engine));
    }
    
    // Get a mutable pointer the first element of that array.
    mptr<bool> a_ptr = &a[0];
    
    // This is the supposedly problematic code.
    valprint("a",OutString::FromVector(a_ptr,n));
    
    
    Check_is_pointer();
}
