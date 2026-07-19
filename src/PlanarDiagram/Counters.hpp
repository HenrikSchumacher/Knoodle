private:

//// Counters for Reidemeister moves.
//Int R_I_counter   = 0;
//Int R_Ia_counter  = 0;
//Int R_II_counter  = 0;
//Int R_IIa_counter = 0;
//Int twist_counter = 0;
//Int four_counter  = 0;

public:

/*!
 * @brief Returns how many Reidemeister I moves have been performed so far.
 */

Int Reidemeister_I_Counter() const
{
    return R_I_counter;
}

/*!
 * @brief Returns how many Reidemeister Ia moves have been performed so far.
 */

Int Reidemeister_Ia_Counter() const
{
    return R_Ia_counter;
}

/*!
 * @brief Returns how many Reidemeister II moves have been performed so far.
 */

Int Reidemeister_II_Counter() const
{
    return R_II_counter;
}

/*!
 * @brief Returns how many Reidemeister IIa moves have been performed so far.
 */

Int Reidemeister_IIa_Counter() const
{
    return R_IIa_counter;
}

/*!
 * @brief Returns how many twist moves have been performed so far.
 *
 * See TwistMove.hpp for details.
 */

Int TwistMove_Counter() const
{
    return twist_counter;
}

/*!
 * @brief Returns how many moves that remove 4 crossings at once have been performed so far.
 */

Int FourMove_Counter() const
{
    return four_counter;
}
