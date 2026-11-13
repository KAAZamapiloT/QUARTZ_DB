import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: "QuartzDB",
    description: "Modern C++23 Embedded Relational Database Engine Documentation",
    cleanUrls: true,
    lastUpdated: true,

    head: [
      ['link', { rel: 'icon', href: '/q-logo.svg' }]
    ],

    mermaid: {
      theme: 'base',
      themeVariables: {
        fontFamily: "'JetBrains Mono', monospace",
        primaryColor: '#1e1e24',
        primaryTextColor: '#f7f3e9',
        primaryBorderColor: '#e05a47',
        lineColor: '#94a3b8',
        secondaryColor: '#282830',
        tertiaryColor: '#16161a',
        edgeLabelBackground: '#1e1e24'
      },
      flowchart: {
        htmlLabels: true,
        nodeSpacing: 50,
        rankSpacing: 60,
        padding: 12
      }
    },

    themeConfig: {
      siteTitle: 'QuartzDB',
      logo: '/q-logo.svg',

      // Target ≤ 5 top-level nav items (Docs, Reference, Benchmarks, v1.0)
      nav: [
        {
          text: 'Docs',
          items: [
            { text: 'Database Overview', link: '/overview' },
            { text: 'Getting Started', link: '/getting-started/' },
            { text: 'Architecture & Pipeline', link: '/concepts/architecture' },
            { text: 'Slotted-Page Storage', link: '/concepts/storage' },
            { text: 'Disk-Backed B+ Tree', link: '/concepts/indexing' },
            { text: 'Backup & Recovery Runbooks', link: '/operations/backup-recovery' }
          ]
        },
        {
          text: 'Reference',
          items: [
            { text: 'SQL Syntax Reference', link: '/sql/' },
            { text: 'Configuration Reference', link: '/configuration/' },
            { text: 'Error Code Explorer', link: '/errors/' },
            { text: 'Interactive REPL (quartz.exe)', link: '/tools/cli' },
            { text: 'Global Data Dictionary', link: '/data-dictionary' },
            { text: 'Interactive Schema ERD', link: '/diagrams' }
          ]
        },
        { text: 'Benchmarks', link: '/benchmarks/performance' },
        {
          text: 'v1.0',
          items: [
            { text: 'Business Rules & Constraints', link: '/rules' },
            { text: 'Security, Roles & PII Map', link: '/security' },
            { text: 'All Tables Index', link: '/tables' },
            { text: 'Documentation History', link: '/changelog' },
            { text: 'GitHub Repository', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB' },
            { text: 'Report Issue', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB/issues' }
          ]
        }
      ],

      sidebar: [
        {
          text: 'Getting Started',
          items: [
            { text: 'Database Overview', link: '/overview' },
            { text: 'Quickstart & Building', link: '/getting-started/' },
            { text: 'C++ Embedding Guide', link: '/guide/cpp-integration' },
            { text: 'Test Suite & Verification', link: '/TESTING' }
          ]
        },
        {
          text: 'Engine Concepts',
          items: [
            { text: 'Architecture & Pipeline', link: '/concepts/architecture' },
            { text: 'Slotted-Page Storage', link: '/concepts/storage' },
            { text: 'Disk-Backed B+ Tree', link: '/concepts/indexing' },
            { text: 'Pager & Block Allocation', link: '/architecture/pager' }
          ]
        },
        {
          text: 'SQL Reference',
          items: [
            { text: 'Overview', link: '/sql/' },
            { text: 'SELECT', link: '/sql/statements/select' },
            { text: 'INSERT', link: '/sql/statements/insert' },
            { text: 'UPDATE', link: '/sql/statements/update' },
            { text: 'DELETE', link: '/sql/statements/delete' },
            { text: 'CREATE TABLE', link: '/sql/statements/create-table' },
            { text: 'DROP TABLE', link: '/sql/statements/drop-table' },
            { text: 'Relational JOINs', link: '/sql/joins' },
            { text: 'Aggregate Functions', link: '/sql/aggregations' },
            { text: 'ORDER BY & LIMIT / OFFSET', link: '/sql/order-by-limit' },
            { text: 'Data Types', link: '/sql/data-types' },
            { text: 'Operators', link: '/sql/operators' }
          ]
        },
        {
          text: 'Configuration & Errors',
          items: [
            { text: 'Configuration Reference', link: '/configuration/' },
            { text: 'Error Code Explorer', link: '/errors/' }
          ]
        },
        {
          text: 'Operations & Tooling',
          items: [
            { text: 'Interactive CLI (quartz.exe)', link: '/tools/cli' },
            { text: 'Backup & Recovery Runbooks', link: '/operations/backup-recovery' },
            { text: 'Scale Benchmarks (120k Rows)', link: '/benchmarks/performance' }
          ]
        },
        {
          text: 'Catalog, Schema & Rules',
          collapsed: false,
          items: [
            { text: 'Interactive Relational ERD', link: '/diagrams' },
            { text: 'Global Data Dictionary', link: '/data-dictionary' },
            { text: 'All Tables Index', link: '/tables' },
            { text: 'Business Rules & Constraints', link: '/rules' },
            { text: 'Security, Roles & PII Map', link: '/security' }
          ]
        }
      ],

      search: {
        provider: 'local'
      },

      socialLinks: [
        { icon: 'github', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB' }
      ],

      footer: {
        message: 'A serial publication for systems & database engineers. Released under the MIT License.',
        copyright: 'Copyright © 2026 QuartzDB Engineering Team'
      },

      editLink: {
        pattern: 'https://github.com/KAAZamapiloT/QUARTZ_DB/edit/main/docs/:path',
        text: 'Edit this page on GitHub'
      }
    }
  })
)
