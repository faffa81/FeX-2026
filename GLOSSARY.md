# Glossary

Domain terms used by the FeX client. Only add terms that the code itself uses.

**War list** — the persistent, user-managed list of players and clans, stored in
`fex_wars.cfg`. Every entry belongs to a war type (`none`, `enemy`, `team`,
`helper`). Owned by `CWarList`.

**Temporary list** — session-only war, helper and mute entries added while
playing (`addtempwar`, `addtemphelper`, `addtempmute`). Never written to disk.
Owned by `CPlayerRelations`.

**Player relation** — the merged answer to "how does this client relate to the
local player": war, team, helper, muted, plus the colour and reason to show.
Resolved only by `CPlayerRelations::Get()`. No other module combines the war
list and the temporary list itself.
