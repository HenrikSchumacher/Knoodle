#pragma once

// The KLUT table set this knoodleidentify is built for: the `set` line of
// data/Klut/Klut_Manifest.txt. Written by scripts/klut-manifest.py; do not edit
// by hand. knoodleidentify refuses tables from another set unless given
// --allow-other-tables, since knot numbering can differ between table sets.
// test/klut_table_check.py checks this matches the manifest.

#include <string_view>

inline constexpr std::string_view kKlutTableSet = "d28a85770f2cab04";
