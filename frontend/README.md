# KERNEL — Resource Allocation Simulator Frontend

This is a standalone responsive frontend preview for the Advanced Multithreaded Resource Allocation and Deadlock Management Simulator.

## Run
Open `index.html` in a browser. No build tools or npm install are required.

## Important
This is **DEMO / PREVIEW mode**. Request/release interactions modify browser-local sample state only. They do not call the C backend. Do not present the preview result as a live C-engine result.

## Integration contract
The existing C implementation should remain the source of truth. Add an API bridge and then replace the local state with API calls. Suggested routes:

- `GET /api/state` — customers, resources, total, available, maximum, allocation, need, requests, safe, safeSequence
- `POST /api/request` — `{ customerId, amounts }`; call C `db_request()`
- `POST /api/release` — `{ customerId, amounts }`; call C `db_release()`
- `POST /api/safety` — run C safety check
- `POST /api/deadlocks` — run deadlock detector
- `GET /api/history` — read operation history from SQLite
- `POST /api/reset` — initialize a configuration

Keep one active `DynamicBanker` instance per simulation session, use the existing mutex-protected C APIs, return refreshed state after mutations, and read history from SQLite. The multithreading page should only show real runtime thread information if a real concurrency endpoint is implemented.
