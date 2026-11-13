-- ===================================================================
-- QuartzDB Demonstration Script: Quickstart & Feature Showcase
-- ===================================================================

-- 1. Create Relational Tables
CREATE TABLE departments (
    dept_id INT PRIMARY KEY,
    name TEXT NOT NULL,
    location TEXT
);

CREATE TABLE employees (
    emp_id INT PRIMARY KEY,
    name TEXT NOT NULL,
    dept_id INT,
    salary INT,
    role TEXT
);

-- 2. Populate Departments
INSERT INTO departments VALUES (101, 'Engineering', 'Building A');
INSERT INTO departments VALUES (102, 'Marketing',   'Building B');
INSERT INTO departments VALUES (103, 'Research',    'Building C');
INSERT INTO departments VALUES (104, 'Operations',  NULL);

-- 3. Populate Employees (Demonstrating varied salaries and NULL values)
INSERT INTO employees VALUES (1, 'Alice Smith',   101, 115000, 'Staff Engineer');
INSERT INTO employees VALUES (2, 'Bob Jones',     102,  78000, 'Marketing Lead');
INSERT INTO employees VALUES (3, 'Charlie Brown', 101,  95000, 'Systems Architect');
INSERT INTO employees VALUES (4, 'Diana Prince',  103, 130000, 'Principal Scientist');
INSERT INTO employees VALUES (5, 'Evan Wright',   101,  88000, 'DevOps Specialist');
INSERT INTO employees VALUES (6, 'Fiona Gallagher', 104, NULL, 'Intern');

-- 4. Basic Projection & Filtering
SELECT name, salary, role FROM employees WHERE salary > 90000;

-- 5. Compound WHERE with Logical Operators (AND, OR, Parentheses)
SELECT name, dept_id, salary FROM employees 
WHERE (dept_id = 101 OR dept_id = 103) AND salary >= 95000;

-- 6. Relational INNER JOIN (Employees joined with Departments)
SELECT employees.name, employees.role, departments.name, departments.location 
FROM employees JOIN departments ON employees.dept_id = departments.dept_id;

-- 7. Relational LEFT JOIN (Preserves rows with missing or outer relations)
SELECT employees.name, employees.salary, departments.name 
FROM employees LEFT JOIN departments ON employees.dept_id = departments.dept_id;

-- 8. Sorting & Pagination (Top-3 Highest Paid Employees)
SELECT name, salary, role FROM employees 
ORDER BY salary DESC LIMIT 3;

-- 9. Pagination with OFFSET (Page 2: 2 rows per page, offset 2)
SELECT name, salary FROM employees 
ORDER BY salary DESC LIMIT 2 OFFSET 2;

-- 10. Aggregate Functions across all Employees
SELECT COUNT(*), COUNT(salary), SUM(salary), AVG(salary), MIN(salary), MAX(salary) 
FROM employees;

-- 11. Filtered Aggregates (Engineering Department only)
SELECT COUNT(*), SUM(salary), AVG(salary) 
FROM employees WHERE dept_id = 101;

-- 12. In-place UPDATE
UPDATE employees SET salary = 120000 WHERE emp_id = 1;

-- 13. Verify Updated Record
SELECT name, salary, role FROM employees WHERE emp_id = 1;

-- 14. DELETE with Filter
DELETE FROM employees WHERE emp_id = 6;

-- 15. Verify Final Record Count
SELECT COUNT(*) FROM employees;
