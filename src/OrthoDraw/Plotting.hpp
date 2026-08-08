public:

/*!@brief Return the horizontal grid size.*/
Int HorizontalGridSize() const
{
    return settings.x_grid_size;
}

/*!@brief Set the horizontal grid size.*/
void SetHorizontalGridSize( const Int val )
{
    settings.x_grid_size = val;
}

/*!@brief Return the vertical grid size.*/
Int VerticalGridSize() const
{
    return settings.y_grid_size;
}

/*!@brief Set the vertical grid size.*/
void SetVerticalGridSize( const Int val )
{
    settings.y_grid_size = val;
}


/*!@brief Return the horizontal gap size, i.e., the size of the gap in a horizontal, undergoing strand.*/
Int HorizontalGapSize() const
{
    return settings.x_gap_size;
}

/*!@brief Set the horizontal gap size, i.e., the size of the gap in a horizontal, undergoing strand.*/
void SetHorizontalGapSize( const Int val )
{
    settings.x_gap_size = val;
}

/*!@brief Return the vertical gap size, i.e., the size of the gap in a vertical, undergoing strand.*/
Int VerticalGapSize() const
{
    return settings.y_gap_size;
}

/*!@brief Set the horizontal gap size, i.e., the size of the gap in a vertical, undergoing strand.*/
void SetVerticalGapSize( const Int val )
{
    settings.y_gap_size = val;
}


Int HorizontalRoundingRadius() const
{
    return settings.x_rounding_radius;
}

void SetHorizontalRoundingRadius( const Int val )
{
    settings.x_rounding_radius = val;
}

Int VerticalRoundingRadius() const
{
    return settings.y_rounding_radius;
}

void SetVerticalRoundingRadius( const Int val )
{
    settings.y_rounding_radius = val;
}
