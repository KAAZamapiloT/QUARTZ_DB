import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: "QuartzDB",
    description: "Modern C++23 Embedded Relational Database Engine Documentation",
    cleanUrls: true,
    lastUpdated: true,

    head: [
      ['link', { rel: 'icon', href: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><text y=".9em" font-size="90">💎</text></svg>' }]
    ],

    mermaid: {
      theme: 'base',
      themeVariables: {
        primaryColor: '#9e2a2b',
        primaryTextColor: '#ffffff',
        primaryBorderColor: '#7d2021',
        lineColor: '#64748b',
        secondaryColor: '#f4f0e6',
        tertiaryColor: '#e3ded2',
        fontFamily: 'Inter, system-ui, sans-serif'
      }
    },

    themeConfig: {
      siteTitle: 'QuartzDB 💎',
      logo: {
        text: '💎'
      },

      nav: [
        { text: 'Getting Started', link: '/getting-started/' },
        { text: 'Concepts', link: '/concepts/architecture' },
        { text: 'SQL Reference', link: '/sql/' },
        { text: 'Configuration', link: '/configuration/' },
        { text: 'Error Explorer', link: '/errors/' },
        { text: 'Operations', link: '/operations/backup-recovery' },
        { text: 'Benchmarks', link: '/benchmarks/performance' },
        { text: 'v1.0', items: [
          { text: 'CLI Reference', link: '/tools/cli' },
          { text: 'Interactive ERD', link: '/diagrams' },
          { text: 'Data Dictionary', link: '/data-dictionary' },
          { text: 'Changelog', link: '/changelog' },
          { text: 'GitHub Repository', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB' }
        ]}
      ],

      sidebar: [
        {
          text: 'Getting Started',
          items: [
            { text: 'Quickstart & Building', link: '/getting-started/' }
          ]
        },
        {
          text: 'Engine Concepts',
          items: [
            { text: 'Architecture & Pipeline', link: '/concepts/architecture' },
            { text: 'Slotted-Page Storage', link: '/concepts/storage' },
            { text: 'Disk-Backed B+ Tree', link: '/concepts/indexing' }
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
          text: 'Catalog & Schema',
          collapsed: true,
          items: [
            { text: 'Interactive Relational ERD', link: '/diagrams' },
            { text: 'Global Data Dictionary', link: '/data-dictionary' },
            { text: 'All Tables Index', link: '/tables' }
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
        message: 'Released under the MIT License. Modern C++23 Embedded Relational Engine.',
        copyright: 'Copyright © 2026 QuartzDB Engineering Team'
      },

      editLink: {
        pattern: 'https://github.com/KAAZamapiloT/QUARTZ_DB/edit/main/docs/:path',
        text: 'Edit this page on GitHub'
      }
    }
  })
)
