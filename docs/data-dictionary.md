# Data Dictionary 📖

The **Data Dictionary** provides a centralized, filterable index of all attributes, data types, constraints, and business descriptions across every registered table in QuartzDB.

---

## Global Attribute Directory

Use the search bar and filter dropdowns below to quickly inspect column types, primary keys, foreign key references, and PII sensitivities:

<DataDictionary />

---

## Supported Engine Data Types

| Engine Type | C++ Storage Representation | Serialization Format | Notes |
| :--- | :--- | :--- | :--- |
| **`INT64`** | `std::int64_t` (64-bit signed) | Tag `1` + 8 bytes little-endian | Primary keys must be of type `INT64`. |
| **`TEXT`** | `std::string` | Tag `2` + 4 bytes length + UTF-8 bytes | Dynamically packed in slotted page payloads. |
| **`NULL`** | `std::monostate` | Tag `0` (1 single byte) | Supported for unconstrained columns. |
