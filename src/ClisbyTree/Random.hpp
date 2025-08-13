public:

using int_unif  = std::uniform_int_distribution<Int>;
using real_unif = std::uniform_real_distribution<Real>;

using wrapped_gaussian       = WrappedGaussianDistribution<Real>;
using discr_wrapped_gaussian = DiscreteWrappedGaussianDistribution<Int,double>;
using discr_wrapped_laplace  = DiscreteWrappedLaplaceDistribution<Int,double>;

private:

real_unif              prob_unif      { Real(0), Real(1) };

AngleRandomMethod_T    angle_rand_method = AngleRandomMethod_T::Uniform;

real_unif              angle_unif     { Real(0), Scalar::TwoPi<Real> };
wrapped_gaussian       angle_gaussian { Real(0), Real(1), Scalar::TwoPi<Real> };

PivotRandomMethod_T    pivot_rand_method = PivotRandomMethod_T::Uniform;

discr_wrapped_gaussian pivot_gaussian { double(0), double(1), Int(1) };
discr_wrapped_laplace  pivot_laplace  { double(0), double(1), Int(1) };

real_unif              pivot_clisby   { Real(1), Real(2) };

int_unif               coin { Int(0), Int(1) };

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
        case AngleRandomMethod_T::WrappedGaussian:
        {
            return "AngleRandomMethod_T::WrappendGaussian";
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

void UseWrappedGaussianAngles( const Real sigma )
{
    if( sigma <= Real(0) )
    {
        eprint(MethodName("UseWrappedGaussuanAngles") + " argument sigma = " + Tools::ToString(sigma) + " is invalid. Continue to use the present random method \"" + ToString(angle_rand_method) +"\".");
    }
    
    angle_rand_method = AngleRandomMethod_T::WrappedGaussian;
    angle_gaussian = wrapped_gaussian { Real(0), sigma, Scalar::TwoPi<Real> };
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
        case AngleRandomMethod_T::WrappedGaussian:
        {
            return angle_gaussian(random_engine);
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
        case PivotRandomMethod_T::DiscreteWrappedGaussian:
        {
            return "PivotRandomMethod_T::DiscreteWrappendGaussian";
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

void UseDiscreteWrappedGaussianPivots( const double sigma )
{
    if( sigma <= double(0) )
    {
        eprint(MethodName("UseDiscreteWrappedGaussianPivots") + " argument sigma = " + Tools::ToString(sigma) + " must be positive. Using the present random method \"" + ToString(pivot_rand_method) +"\" instead.");
    }
    
    pivot_rand_method = PivotRandomMethod_T::DiscreteWrappedGaussian;
    pivot_gaussian = discr_wrapped_gaussian { double(0), sigma, VertexCount() };
}

void UseDiscreteWrappedLaplacePivots( const double beta )
{
    if( beta <= double(0) )
    {
        eprint(MethodName("UseDiscreteWrappedLaplacePivots") + " argument beta = " + Tools::ToString(beta) + " must be positive. Using the present random method \"" + ToString(pivot_rand_method) +"\" instead.");
    }
    
    pivot_rand_method = PivotRandomMethod_T::DiscreteWrappedLaplace;
    pivot_laplace     = discr_wrapped_laplace { double(0), beta, VertexCount() };
}

void UseClisbyPivots()
{
    pivot_rand_method = PivotRandomMethod_T::Clisby;
    
    // Caution: We really have to _integer_-divide (rounded down) here!
    const Real max_dist = static_cast<Real>(VertexCount()/Int(2));
    
    pivot_clisby  = real_unif {
        Real(1), std::log2(max_dist + Real(1))
    };
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

std::pair<Int,Int> RandomPivots_DiscreteWrappedGaussian()
{
    const Int n = VertexCount();
    Int i;
    Int j;
    do
    {
        i = RandomInteger( Int(0), n - Int(1) );
        j = i + pivot_gaussian(random_engine);
        
        // j now lies in [0,n) + [0,n) = [0,2 * n - 1)
        
        if( j >= n ) { j -= n; }
    }
    while( ModDistance(n,i,j) <= Int(1) );

    return std::pair<Int,Int>( i, j );
}

std::pair<Int,Int> RandomPivots_DiscreteWrappedLaplace()
{
    const Int n = VertexCount();
    Int i;
    Int j;
    do
    {
        i = RandomInteger( Int(0), n - Int(1) );
        
        j = i + pivot_laplace(random_engine);
        
        // j now lies in [0,n) + [0,n) = [0,2 * n - 1)
        
        if( j >= n ) { j -= n; }
    }
    while( ModDistance(n,i,j) <= Int(1) );

    return std::pair<Int,Int>( i, j );
}

std::pair<Int,Int> RandomPivots_Clisby()
{
    const Int n = VertexCount();
    
    Int i = RandomInteger( Int(0), n - Int(1) );
    Int j;
    
    Real d = std::floor(std::exp2(pivot_clisby(random_engine)));
    
    // If n is even and if d is the maximal distance, we reject with 50% and sample again.
    while( (n % Int(2) == Int(0)) && (d == n/Int(2)) && coin(random_engine) )
    {
        
        d = std::floor(std::exp2(pivot_clisby(random_engine)));
    }
        
//    j = i + static_cast<Int>(d);
//    if( j >= n )
//    {
//        j -= n;
//        std::swap(i,j);
//    }
    
    if( coin(random_engine) )
    {
        j = i + static_cast<Int>(d);
        if( j >= n )
        {
            j -= n;
        }
    }
    else
    {
        j = i + n - static_cast<Int>(d);
        if( j >= n )
        {
            j -= n;
        }
    }
    
    // DEBUGGING
    if( ModDistance(n,i,j) <= Int(1) )
    {
        eprint("!!!");
    }

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
        case PivotRandomMethod_T::DiscreteWrappedGaussian:
        {
            return RandomPivots_DiscreteWrappedGaussian();
        }
        case PivotRandomMethod_T::DiscreteWrappedLaplace:
        {
            return RandomPivots_DiscreteWrappedLaplace();
        }
        case PivotRandomMethod_T::Clisby:
        {
            return RandomPivots_Clisby();
        }
        default:
        {
            return RandomPivots_Box(Int(0),VertexCount());
        }
    }
}
