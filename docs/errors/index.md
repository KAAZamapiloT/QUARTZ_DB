# Error Code Explorer 🚨

Search by error code (e.g. `1001`), subsystem, or paste the error string emitted by QuartzDB to find root causes, fixes, and corrected queries.

---

<ErrorExplorer />

---

## Standard Error Response Structure

When an operation fails in the CLI or C++ API, QuartzDB returns a typed `qdb::Status`:

```cpp
struct Status {
    StatusCode  code;       // Numeric error enum (e.g. 1001)
    std::string message;    // Detailed human-readable diagnostics
    std::string subsystem;  // "PrimaryIndex", "Pager", "Lexer", etc.
};
```

In the interactive REPL shell:
```text
quartz> INSERT INTO users VALUES (1, 'Alice');
Error [QERR_1001 / PrimaryIndex]: Duplicate key '1' violates primary key uniqueness constraint.
```
