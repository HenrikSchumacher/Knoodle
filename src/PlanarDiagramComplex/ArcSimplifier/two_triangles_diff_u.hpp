// w_0 == e_1 cannot occur because we ran a_is_2loop.
//
// If u_0 && !u_1, only these short-circuits are possible:
//      (w_0 == w_2), (e_1 == s_2), (w_3 == n_3)
//
// We can do interesting things if at least two of them are short-circuited, i.e., if:
//
// (w_0 == w_2) && (e_1 == s_2)
//
// (w_0 == w_2) && (w_3 == n_3)
//
// (w_3 == n_3) && (e_1 == s_2)

/* We could disconnect/identify a trefoil knot, figure-eight knot, or an unlink.
 *
 * u_0 && !u_1 : (w_0 == w_2) && (e_1 == s_2)
 *
 *              #####
 *              #   #
 *          w_3 O   O n_3
 *               \ /
 *                X c_3
 *               / \
 *              O   O
 *             /     \
 *        n_0 /       \ n_1
 *           O         O
 *    w_0    ^    a    | e_1
 *   +----O--X->O-->O--X->O----+
 *   |       |c_0      vc_1    |
 *   |       O         O       |
 *   |    s_0 \       / s_1    |
 *   |         \     /         |
 *   |          O   O          |
 *   |           \ /           |
 *   |            X c_2        |
 *   |           / \           |
 *   |          O   O          |
 *   |      w_2 |   |  s_2     |
 *   +----------+   +----------+
 */

// w_0 == e_1 cannot occur because we ran a_is_2loop.
//
// If u_0 && !u_1, only these short-circuits are possible:
//      (w_0 == w_3), (w_2 == s_2), (e_1 == n_3)
//
// We can do interesting things if at least two of them are short-circuited, i.e., if:
//
// (w_0 == w_3) && (w_2 == s_2)
//
// (w_0 == w_3) && (e_1 == n_3)
//
// (e_1 == n_3) && (e_1 == n_3)

/* We could disconnect/identify a trefoil knot, figure-eight knot, or an unlink.
 *
 *   +----------+   +----------+
 *   |          |   |          |
 *   |      w_3 O   O n_3      |
 *   |           \ /           |
 *   |            X c_3        |
 *   |           / \           |
 *   |          O   O          |
 *   |         /     \         |
 *   |    n_0 /       \ n_1    |
 *   |       O         O       |
 *   |w_0    |    a    ^ e_1   |
 *   +----O--X->O-->O--X->O----+
 *           vc_0      |c_1
 *           O         O
 *        s_0 \       / s_1
 *             \     /
 *              O   O
 *               \ /
 *                X c_2
 *               / \
 *              O   O
 *          w_2 #   #  s_2
 *              #####
 */
