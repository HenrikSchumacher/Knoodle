
#pragma once

namespace KnotTools
{
    
    template<typename Real, typename Int>
    struct Collision
    {
        using Vector3_T = Tiny::Vector<3,Real,Int>;
        using Vector2_T = Tiny::Vector<2,Real,Int>;
        
        Real time;
        Vector3_T point;
        Vector2_T z;
        Int  i;
        Int  j;
        Int  flag;
        
        Collision(
            cref<Real> time_,
            const Vector3_T && point_,
            const Vector2_T && z_,
            cref<Int> i_,
            cref<Int> j_,
            cref<Int> flag_
        )
        :   time        ( time_             )
        ,   point       ( std::move(point_) )
        ,   z           ( std::move(z_)     )
        ,   i           ( i_                )
        ,   j           ( j_                )
        ,   flag        ( flag_             )
        {}
    };

} // namespace KnotTools
