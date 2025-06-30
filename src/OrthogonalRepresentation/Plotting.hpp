//###########################################################
//##        Plotting
//###########################################################

public:

Int HorizontalGridSize() const
{
    return settings.x_grid_size;
}

void SetHorizontalGridSize( const Int val )
{
    settings.x_grid_size = val;
}

Int VerticalGridSize() const
{
    return settings.y_grid_size;
}

void SetVerticalGridSize( const Int val )
{
    settings.y_grid_size = val;
}



Int HorizontalGapSize() const
{
    return settings.x_gap_size;
}

void SetHorizontalGapSize( const Int val )
{
    settings.x_gap_size = val;
}

Int VerticalGapSize() const
{
    return settings.y_gap_size;
}

void SetVerticalGapSize( const Int val )
{
    settings.y_gap_size = val;
}
