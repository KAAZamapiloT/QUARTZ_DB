import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: "QuartzDB",
    description: "A serial publication & comprehensive database documentation portal for QuartzDB C++23.",
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
        { text: 'Overview', link: '/overview' },
        { text: 'Tables', link: '/tables' },
        { text: 'Diagrams', link: '/diagrams' },
        { text: 'Dictionary', link: '/data-dictionary' },
        { text: 'Governance', items: [
          { text: 'Business Rules & Constraints', link: '/rules' },
          { text: 'Security, Roles & PII Map', link: '/security' },
          { text: 'Operations & Compaction Runbooks', link: '/operations' }
        ]},
        { text: 'Architecture', items: [
          { text: 'System Architecture', link: '/architecture/overview' },
          { text: 'Block Pager (4KB)', link: '/architecture/pager' },
          { text: 'Slotted-Page Layout', link: '/architecture/slotted-pages' },
          { text: 'Disk-Backed B+ Tree', link: '/architecture/bplus-tree' },
          { text: 'SQL Syntax Reference', link: '/sql/reference' },
          { text: 'Scale Benchmarks', link: '/benchmarks/performance' }
        ]},
        { text: 'Changelog', link: '/changelog' }
      ],

      sidebar: [
        {
          text: 'Database Catalog',
          collapsed: false,
          items: [
            { text: 'Database Overview', link: '/overview' },
            { text: 'All Tables Index', link: '/tables' },
            { text: '▸ Table: orders', link: '/tables/orders' },
            { text: '▸ Table: users', link: '/tables/users' },
            { text: '▸ Table: products', link: '/tables/products' },
            { text: 'Interactive ERD', link: '/diagrams' },
            { text: 'Data Dictionary', link: '/data-dictionary' }
          ]
        },
        {
          text: 'Governance & Operations',
          collapsed: false,
          items: [
            { text: 'Business Rules & Constraints', link: '/rules' },
            { text: 'Security, Roles & PII Map', link: '/security' },
            { text: 'Operations & Compaction Runbooks', link: '/operations' }
          ]
        },
        {
          text: 'Engine Internals (Series)',
          collapsed: false,
          items: [
            { text: 'Architecture Overview', link: '/architecture/overview' },
            { text: 'Pager & 4KB Boundaries', link: '/architecture/pager' },
            { text: 'Slotted-Page Storage', link: '/architecture/slotted-pages' },
            { text: 'Disk-Backed B+ Tree', link: '/architecture/bplus-tree' },
            { text: 'SQL Syntax Reference', link: '/sql/reference' },
            { text: 'Interactive REPL CLI', link: '/sql/interactive-cli' },
            { text: 'Scale Benchmarks (120k Rows)', link: '/benchmarks/performance' }
          ]
        },
        {
          text: 'Meta',
          collapsed: true,
          items: [
            { text: 'Documentation History', link: '/changelog' }
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
