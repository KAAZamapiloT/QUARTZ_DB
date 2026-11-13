import { defineConfig } from 'vitepress'
import { withMermaid } from 'vitepress-plugin-mermaid'

export default withMermaid(
  defineConfig({
    title: "QuartzDB",
    description: "A lightweight, embedded relational database engine written in modern C++23.",
    cleanUrls: true,
    lastUpdated: true,

    head: [
      ['link', { rel: 'icon', href: 'data:image/svg+xml,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 100 100"><text y=".9em" font-size="90">💎</text></svg>' }]
    ],

    mermaid: {
      // Mermaid configuration options
      theme: 'base',
      themeVariables: {
        primaryColor: '#0284c7',
        primaryTextColor: '#ffffff',
        primaryBorderColor: '#0369a1',
        lineColor: '#64748b',
        secondaryColor: '#f1f5f9',
        tertiaryColor: '#e2e8f0',
        fontFamily: 'system-ui, -apple-system, sans-serif'
      }
    },

    themeConfig: {
      siteTitle: 'QuartzDB 💎',
      logo: {
        text: '💎'
      },

      nav: [
        { text: 'Guide', link: '/guide/what-is-quartz' },
        { text: 'Architecture', link: '/architecture/overview' },
        { text: 'SQL & CLI', link: '/sql/reference' },
        { text: 'Benchmarks', link: '/benchmarks/performance' },
        { text: 'C++ API', link: '/guide/cpp-integration' },
        { text: 'v1.0', items: [
          { text: 'GitHub Repository', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB' },
          { text: 'Issue Tracker', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB/issues' }
        ]}
      ],

      sidebar: {
        '/guide/': [
          {
            text: 'Introduction',
            items: [
              { text: 'What is QuartzDB?', link: '/guide/what-is-quartz' },
              { text: 'Getting Started & Building', link: '/guide/getting-started' },
              { text: 'C++ Embedding API', link: '/guide/cpp-integration' }
            ]
          }
        ],
        '/architecture/': [
          {
            text: 'Engine Internals',
            items: [
              { text: 'Architecture Overview', link: '/architecture/overview' },
              { text: 'Pager & 4KB Boundaries', link: '/architecture/pager' },
              { text: 'Slotted-Page Storage', link: '/architecture/slotted-pages' },
              { text: 'Disk-Backed B+ Tree', link: '/architecture/bplus-tree' }
            ]
          }
        ],
        '/sql/': [
          {
            text: 'SQL & Terminal',
            items: [
              { text: 'SQL Syntax Reference', link: '/sql/reference' },
              { text: 'Interactive REPL CLI', link: '/sql/interactive-cli' }
            ]
          }
        ],
        '/benchmarks/': [
          {
            text: 'Benchmarking',
            items: [
              { text: 'Scale & Limits (120k Rows)', link: '/benchmarks/performance' }
            ]
          }
        ]
      },

      search: {
        provider: 'local'
      },

      socialLinks: [
        { icon: 'github', link: 'https://github.com/KAAZamapiloT/QUARTZ_DB' }
      ],

      footer: {
        message: 'Released under the MIT License. Designed for educational depth and high performance.',
        copyright: 'Copyright © 2026 QuartzDB Contributors'
      },

      editLink: {
        pattern: 'https://github.com/KAAZamapiloT/QUARTZ_DB/edit/main/docs/:path',
        text: 'Edit this page on GitHub'
      }
    }
  })
)
