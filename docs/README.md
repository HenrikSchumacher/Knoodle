# Knoodle docs

Working notes and discoveries by Jason + Claude. These are *our* documents
(tools/test/docs territory) — they describe Henrik's code but live outside it.

Conventions:
- Each doc states what has been **verified by reading the code/running it**
  versus what is **inferred or reported second-hand** (e.g. by an exploration
  agent). Verify before relying on an unverified claim.
- Cite file paths and line numbers where possible. Paths into Henrik's
  Mathematica research snapshot are under
  `/Users/jasoncantarella/PackageSources_2026-05-03T19-14-31/PackageSources/`
  (read-only; abbreviated below as `$PKG/`).

## Index

- [klut-overview.md](klut-overview.md) — KLUT (Knot LookUp Table): architecture,
  key/value schemes, generation pipeline, data inventory, project status.
- [knoodleidentify-design.md](knoodleidentify-design.md) — design sketch for the
  third pipeline tool: `knoodlesimplify | knoodleidentify` → knot names per
  connect-sum summand.
- [upstream-issues.md](upstream-issues.md) — bugs found in src/ to report to
  Henrik, with evidence.
