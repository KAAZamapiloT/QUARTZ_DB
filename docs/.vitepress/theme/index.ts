import DefaultTheme from 'vitepress/theme'
import './custom.css'

import HeroForge from './components/HeroForge.vue'
import SqlPlayground from './components/SqlPlayground.vue'
import CodeDiff from './components/CodeDiff.vue'
import PrerequisiteGraph from './components/PrerequisiteGraph.vue'
import DataDictionary from './components/DataDictionary.vue'
import InteractiveErd from './components/InteractiveErd.vue'
import ConfigTable from './components/ConfigTable.vue'
import ErrorExplorer from './components/ErrorExplorer.vue'
import SyntaxRailroad from './components/SyntaxRailroad.vue'
import MemoryMap from './components/MemoryMap.vue'

export default {
  extends: DefaultTheme,
  enhanceApp({ app }) {
    app.component('HeroForge', HeroForge)
    app.component('SqlPlayground', SqlPlayground)
    app.component('CodeDiff', CodeDiff)
    app.component('PrerequisiteGraph', PrerequisiteGraph)
    app.component('DataDictionary', DataDictionary)
    app.component('InteractiveErd', InteractiveErd)
    app.component('ConfigTable', ConfigTable)
    app.component('ErrorExplorer', ErrorExplorer)
    app.component('SyntaxRailroad', SyntaxRailroad)
    app.component('MemoryMap', MemoryMap)
  }
}
