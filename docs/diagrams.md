# Relational Diagrams & Interactive ERD 🗺️

This section provides visual entity-relationship diagrams (ERD) mapping the relational models and foreign key chains in QuartzDB.

> [!NOTE] Example Schema — Not Engine-Specific
> This page describes the **example sample schema** shipped with QuartzDB for demonstration purposes (`users`, `orders`, `products`). QuartzDB itself does not enforce any predefined business schema, roles, or PII policy. These tables exist solely to illustrate engine capabilities.

---

## Interactive Relational ERD

Click on any table or hover to isolate relationships, primary keys 🔑, and foreign key pointers 🔗:

<InteractiveErd />

---

## Physical Page Layout ERD

Below is the low-level physical layout demonstrating how tables link into slotted pages and B+ trees across 4096-byte database blocks:

<ExcalidrawDiagram
  name="erd-physical-page-layout"
  title="Physical Page Layout ERD (Catalog, Slotted Pages & B+ Tree Blocks)"
  alt="Entity relationship diagram of QuartzDB physical page layout: Catalog Page 0, Table Data Page, B+ Meta Page, B+ Root Page, B+ Internal Page, B+ Leaf Page, and Slotted Page Record"
/>
