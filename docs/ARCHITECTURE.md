# MathForces architecture

The project is split into three clear layers:

- `client/` — Qt Widgets desktop client: tabs, dialogs, UI helpers, themes, table search/sort helpers, network error helpers.
- `server/` — Qt HTTP server: API routes, authentication middleware, database access, LLM and SMTP integrations.
- `sql/` — database schema and seed data.
- `assets/brand/` — MathForces brand images.
- `tools/typst/` — local/offline Typst compiler location.

The client follows a small reusable-utility pattern:

- `ui/app_style.*` owns light/dark themes and tab icons.
- `ui/table_utils.*` prepares sortable/searchable tables.
- `ui/toast.*` displays non-blocking success notifications.
- `network/network_utils.*` converts Qt network errors and server JSON errors into readable messages.
- `tabs/*` contains feature screens.
- `dialogs/*` contains modal workflows like authentication, profiles, and archive tasks.

The archive is now a real workflow: task list -> full task dialog -> Typst preview -> submit -> submissions table.
