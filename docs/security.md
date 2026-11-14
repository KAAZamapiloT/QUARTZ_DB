# Security & Threat Model 🔒

QuartzDB is an in-process, embedded relational storage engine. Unlike client-server database systems (such as PostgreSQL or MySQL), QuartzDB does not listen on network sockets, manage user accounts, or execute privileged background daemons.

This page outlines the operational security model, process boundaries, and architectural considerations when embedding QuartzDB into host applications.

---

## 1. The Embedded Execution Model

Because QuartzDB executes strictly in-process within the host application:

```
┌────────────────────────────────────────────────────────┐
│ Host Application Process (e.g. C++ / Python / Go)      │
│                                                        │
│  ┌─────────────────────────┐  ┌──────────────────────┐ │
│  │ Application Auth / RBAC │  │ Business Logic Layer │ │
│  └────────────┬────────────┘  └──────────┬───────────┘ │
│               │ (Validated Queries)      │             │
│               ▼                          ▼             │
│  ┌──────────────────────────────────────────────────┐  │
│  │ QuartzDB Engine (qdb::Database / qdb::Executor)  │  │
│  │ - AST Parser & Semantic Validation               │  │
│  │ - Slotted-Page & B+ Tree Storage Engine          │  │
│  │ - 4KB Block Pager I/O Layer                      │  │
│  └──────────────────────────┬───────────────────────┘  │
└─────────────────────────────┼──────────────────────────┘
                              │ Direct Synchronous File I/O
                              ▼
                 ┌──────────────────────────┐
                 │ Local Database File      │
                 │ production.db (4KB pages)│
                 └──────────────────────────┘
```

### Architectural Guarantees
- **Zero Network Exposure**: QuartzDB opens no TCP/UDP listening sockets and implements no wire protocol. It cannot be targeted by remote port scanning, network packet spoofing, or denial-of-service against a listening port.
- **In-Process Boundary**: Execution is strictly synchronous within the calling thread. The host process controls the execution lifecycle completely.

---

## 2. Access Control & Authorization

> [!IMPORTANT] Host Application Responsibility
> QuartzDB **does not implement native multi-user accounts, passwords, or Role-Based Access Control (RBAC)**. All SQL queries submitted via `qdb::Executor::execute()` or the C API (`qdb_execute()`) execute with the permissions of the calling process.

If your system requires user roles (e.g. read-only auditors, administrators):
1. **Validate permissions in the host application layer** before invoking the QuartzDB API.
2. **Restrict DDL**: Guard `CREATE TABLE` and `DROP TABLE` queries in application routing logic.
3. **Prepared Query Routing**: Keep SQL construction inside parameterized application services rather than concatenating untrusted user strings into query text.

---

## 3. Storage Security & File Permissions

Access control to QuartzDB data files is enforced directly by the host operating system's filesystem Discretionary Access Control (DAC):

### Recommended File Permissions

On Linux / macOS:
```bash
# Ensure only the application service user can read and write the database
chmod 0600 /var/data/quartz/production.db
chown quartz_app:quartz_app /var/data/quartz/production.db
```

On Windows (PowerShell):
```powershell
# Remove inherited permissions and grant FullControl exclusively to the service account
icacls "C:\data\quartz\production.db" /inheritance:r /grant:r "QuartzServiceUser:(R,W)"
```

---

## 4. Unsupported Security Features

To maintain a lean, zero-dependency engine footprint, the following features are intentionally **not implemented** in QuartzDB:

| Feature | Status | Architectural Alternative |
| :--- | :---: | :--- |
| **Native Multi-User Authentication** | ❌ Not Implemented | Enforce authentication in the host application prior to API execution. |
| **Role-Based Access Control (RBAC)** | ❌ Not Implemented | Filter statements (e.g., block `DROP`/`DELETE`) in application controllers. |
| **Transparent Data Encryption (TDE)** | ❌ Not Implemented | Use filesystem-level encryption (dm-crypt/LUKS, BitLocker, ZFS native encryption) or cloud volume encryption. |
| **Network TLS / Wire Encryption** | ❌ Not Implemented | N/A (QuartzDB is strictly in-process with zero network sockets). |
| **Audit Log Journals** | ❌ Not Implemented | Log executed SQL queries in host application logging frameworks. |
