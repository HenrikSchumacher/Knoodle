//###########################################################
//##        Plotting
//###########################################################

public:

Int HorizontalGridSize() const
{
    return x_grid_size;
}

void SetHorizontalGridSize( const Int val )
{
    x_grid_size = val;
}

Int VerticalGridSize() const
{
    return y_grid_size;
}

void SetVerticalGridSize( const Int val )
{
    y_grid_size = val;
}



Int HorizontalGapSize() const
{
    return x_gap_size;
}

void SetHorizontalGapSize( const Int val )
{
    x_gap_size = val;
}

Int VerticalGapSize() const
{
    return y_gap_size;
}

void SetVerticalGapSize( const Int val )
{
    y_gap_size = val;
}
