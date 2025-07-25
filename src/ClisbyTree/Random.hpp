public:

using int_unif  = std::uniform_int_distribution<Int>;
using real_unif = std::uniform_real_distribution<Real>;

using wrapped_normal       = WrappedNormalDistribution<Real>;
using discr_wrapped_normal = DiscreteWrappedNormalDistribution<Int,double>;

private:

real_unif            prob_unif    { Real(0), Real(1) };

AngleRandomMethod_T  angle_rand_method = AngleRandomMethod_T::Uniform;

real_unif            angle_unif   { Real(0), Scalar::TwoPi<Real> };
wrapped_normal       angle_normal { Real(0), Real(1), Scalar::TwoPi<Real> };

PivotRandomMethod_T  pivot_rand_method = PivotRandomMethod_T::Uniform;

discr_wrapped_normal pivot_normal { double(0), double(1), Int(1) };

public:


bool RandomReflectionFlag( Real P )
{
    if ( P > Real(0) )
    {
        return (prob_unif(random_engine) <= P);
    }
    else
    {
        return false;
    }
}

//###########################################################
//##    Angles
//###########################################################

std::string ToString( const AngleRandomMethod_T method )
{
    switch( method )
    {
//        case AngleRandomMethod_T::Uniform:
//        {
//            return "AngleRandomMethod_T::Uniform";
//        }
        case AngleRandomMethod_T::WrappedNormal:
        {
            return "AngleRandomMethod_T::WrappendNormal";
        }
        default:
        {
            return "AngleRandomMethod_T::Uniform";
        }
    }
}

void UseUniformAngles()
{
    angle_rand_method = AngleRandomMethod_T::Uniform;
}

void UseWrappedNormalAngles( const Real sigma )
{
    if( sigma <= Real(0) )
    {
        eprint(MethodName("UseWrappedNormalAngles") + " argument sigma = " + Tools::ToString(sigma) + " is invalid. Continue to use the present random method \"" + ToString(angle_rand_method) +"\".");
    }
    
    angle_rand_method = AngleRandomMethod_T::WrappedNormal;
    angle_normal = wrapped_normal { Real(0), sigma, Scalar::TwoPi<Real> };
}

AngleRandomMethod_T AngleRandomMethod()
{
    return angle_rand_method;
}

Real RandomAngle()
{
    switch( angle_rand_method )
    {
//        case AngleRandomMethod_T::Uniform:
//        {
//            return angle_unif(random_engine);
//        }
        case AngleRandomMethod_T::WrappedNormal:
        {
            return angle_normal(random_engine);
        }
        default:
        {
            return angle_unif(random_engine);
        }
    }
}

//###########################################################
//##    Pivots
//###########################################################


std::string ToString( const PivotRandomMethod_T method )
{
    switch( method )
    {
//        case PivotRandomMethod_T::Uniform:
//        {
//            return "AngleRandomMethod_T::Uniform";
//        }
        case PivotRandomMethod_T::DiscreteWrappedNormal:
        {
            return "PivotRandomMethod_T::DiscreteWrappendNormal";
        }
        default:
        {
            return "PivotRandomMethod_T::Uniform";
        }
    }
}

void UseUniformPivots()
{
    pivot_rand_method = PivotRandomMethod_T::Uniform;
}

void UseDiscreteWrappedNormalPivots( const double sigma )
{
    if( sigma <= double(0) )
    {
        eprint(MethodName("UseDiscreteWrappedNormalPivots") + " argument sigma = " + Tools::ToString(sigma) + " is invalid. Continue to use the present random method \"" + ToString(pivot_rand_method) +"\".");
    }
    
    pivot_rand_method = PivotRandomMethod_T::DiscreteWrappedNormal;
    pivot_normal = discr_wrapped_normal { double(0), sigma, VertexCount() };
}

PivotRandomMethod_T PivotRandomMethod()
{
    return pivot_rand_method;
}

// Generates a random integer in [a,b[.
Int RandomInteger( const Int a, const Int b )
{
    return int_unif(a,b)(random_engine);
}

// Choose i, j randomly in [begin,end[ so that circular distance of i and j is greater than 1.
// Also, i < j is guaranteed.

std::pair<Int,Int> RandomPivots_Box( Int begin, Int end )
{
    const Int n = VertexCount();

    assert( (begin + Int(0) <  end  ) );
    assert( (begin + Int(1) <  end  ) );
    assert( (begin + Int(2) <  end  ) );
    assert( (Int(0)         <= begin) );
    assert( (end            <= n    ) );

    Int i;
    Int j;
    do
    {
        Int i_ = RandomInteger( begin, end - Int(1) );
        Int j_ = RandomInteger( begin, end - Int(2) );
        
        if( i_ > j_ )
        {
            i = j_;
            j = i_ + Int(1);
        }
        else
        {
            i = i_;
            j = j_ + Int(2);
        }
    }
    while( ModDistance(n,i,j) <= Int(1) );

    return std::pair<Int,Int>( i, j );
}

std::pair<Int,Int> RandomPivots_DiscreteWrappedNormal()
{
    const Int n = VertexCount();
    Int i;
    Int j;
    do
    {
        i = RandomInteger( Int(0), n - Int(1) );
        j = i + pivot_normal(random_engine);
        
        // j now lies in [0,n) + [0,n) = [0,2 * n - 1)
        
        if( j >= n ) { j -= n; }
    }
    while( ModDistance(n,i,j) <= Int(1) );

    return std::pair<Int,Int>( i, j );
}


std::pair<Int,Int> RandomPivots()
{
    switch( pivot_rand_method )
    {
//        case PivotRandomMethod_T::Uniform:
//        {
//            return RandomPivots_Box(Int(0),VertexCount());
//        }
        case PivotRandomMethod_T::DiscreteWrappedNormal:
        {
            return RandomPivots_DiscreteWrappedNormal();
        }
        default:
        {
            return RandomPivots_Box(Int(0),VertexCount());
        }
    }
}
