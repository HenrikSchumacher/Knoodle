#pragma once

#include <functional>

// |det(K)| mod a prime, straight from a PlanarDiagram.
//
// A pass move is an isotopy, so it must preserve this. The test suite needs
// such an invariant because CheckAll is a weak oracle for move code: it says
// the result is a legal diagram, not that it is the RIGHT one. An earlier
// version of AfterDiagram produced CheckAll-clean output with the correct
// crossing count that had unknotted a trefoil, and only a determinant caught
// it.
//
// The matrix is the Fox-colouring one: at each crossing the under-strand
// enters at X[0] and leaves at X[2], the over-strand is the remaining pair
// (one Wirtinger generator, so its two arcs are identified), and the relation
// is x_in + x_out - 2 x_over = 0. Delete one row and one column and take the
// determinant. Working mod a prime keeps it to machine words; a false match
// has probability about 1/p.

#include <vector>

template<typename PD_T>
typename PD_T::Int DeterminantModP(
    const PD_T & pd, typename PD_T::Int p = typename PD_T::Int(1000003) )
{
    using Int = typename PD_T::Int;

    const Int na = pd.MaxArcCount();
    const Int nc = pd.MaxCrossingCount();

    // Wirtinger generators: arcs, with the two over-arcs at each crossing
    // identified. Union-find.
    std::vector<Int> uf(static_cast<std::size_t>(na));
    for( Int a = 0; a < na; ++a ) { uf[static_cast<std::size_t>(a)] = a; }
    std::function<Int(Int)> find = [&](Int x)
    {
        while( uf[static_cast<std::size_t>(x)] != x )
        {
            uf[static_cast<std::size_t>(x)] = uf[static_cast<std::size_t>(uf[static_cast<std::size_t>(x)])];
            x = uf[static_cast<std::size_t>(x)];
        }
        return x;
    };
    auto unite = [&](Int x, Int y){ x=find(x); y=find(y); if(x!=y) uf[static_cast<std::size_t>(y)]=x; };

    struct Row { Int in, out, over; };
    std::vector<Row> rows;

    for( Int c = 0; c < nc; ++c )
    {
        if( !pd.CrossingActiveQ(c) ) { continue; }

        const bool rightQ =
            (pd.CrossingStates()[c] == Knoodle::CrossingState_T::RightHanded);

        const Int u_in  = pd.Crossings()(c, PD_T::In , rightQ ? PD_T::Right : PD_T::Left );
        const Int u_out = pd.Crossings()(c, PD_T::Out, rightQ ? PD_T::Left  : PD_T::Right);
        const Int o_in  = pd.Crossings()(c, PD_T::In , rightQ ? PD_T::Left  : PD_T::Right);
        const Int o_out = pd.Crossings()(c, PD_T::Out, rightQ ? PD_T::Right : PD_T::Left );

        unite(o_in,o_out);
        rows.push_back(Row{u_in,u_out,o_in});
    }

    if( rows.empty() ) { return Int(1); }   // unknot

    std::vector<Int> label(static_cast<std::size_t>(na), Int(-1));
    Int m = 0;
    for( Int a = 0; a < na; ++a )
    {
        if( !pd.ArcActiveQ(a) ) { continue; }
        const Int r = find(a);
        if( label[static_cast<std::size_t>(r)] < 0 ) { label[static_cast<std::size_t>(r)] = m++; }
    }

    const Int n = static_cast<Int>(rows.size());
    std::vector<Int> M(static_cast<std::size_t>(n*m), Int(0));
    auto at = [&](Int i, Int j) -> Int & { return M[static_cast<std::size_t>(i*m + j)]; };

    for( Int i = 0; i < n; ++i )
    {
        at(i, label[static_cast<std::size_t>(find(rows[static_cast<std::size_t>(i)].in ))]) += Int(1);
        at(i, label[static_cast<std::size_t>(find(rows[static_cast<std::size_t>(i)].out))]) += Int(1);
        at(i, label[static_cast<std::size_t>(find(rows[static_cast<std::size_t>(i)].over))]) -= Int(2);
    }

    // (n-1) x (m-1) minor, Gaussian elimination mod p
    const Int d = (n < m ? n : m) - Int(1);
    Int det = Int(1);
    Int row = 0;

    auto pw = [p](Int b, Int e)
    {
        Int r = 1; b %= p; if (b < 0) b += p;
        while( e > 0 ) { if( e & 1 ) r = (r*b) % p; b = (b*b) % p; e >>= 1; }
        return r;
    };

    for( Int col = 0; col < d; ++col )
    {
        Int piv = -1;
        for( Int i = row; i < d; ++i )
        {
            Int v = at(i,col) % p; if( v < 0 ) v += p;
            if( v != 0 ) { piv = i; break; }
        }
        if( piv < 0 ) { return Int(0); }
        if( piv != row )
        {
            for( Int j = 0; j < m; ++j ) { std::swap(at(row,j), at(piv,j)); }
            det = (p - det % p) % p;
        }
        Int pv = at(row,col) % p; if( pv < 0 ) pv += p;
        det = (det * pv) % p;
        const Int inv = pw(pv, p - Int(2));
        for( Int i = row + 1; i < d; ++i )
        {
            Int f = (at(i,col) % p); if( f < 0 ) f += p;
            if( f == 0 ) { continue; }
            f = (f * inv) % p;
            for( Int j = col; j < m; ++j )
            {
                Int v = (at(i,j) - f * (at(row,j) % p)) % p;
                at(i,j) = (v < 0) ? v + p : v;
            }
        }
        ++row;
    }
    return det % p;
}
