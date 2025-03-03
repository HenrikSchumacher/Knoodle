#pragma once

namespace KnotTools
{
    enum class AffineTransformFlag_T : bool
    {
        Id    = false,
        NonId = true
    };

    std::string ToString(
        const AffineTransformFlag_T flag
    )
    {
        if( flag == AffineTransformFlag_T::NonId )
        {
            return "NonId";
        }
        else
        {
            return "Id";
        }
    }
}
