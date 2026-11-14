import fs from 'fs';
import path from 'path';
import { buildAllDiagrams } from './diagram-definitions.mjs';

const ROOT_DIR = process.cwd();
const EDITABLE_DIR = path.join(ROOT_DIR, 'docs', 'diagrams', 'editable');
const GENERATED_DIR = path.join(ROOT_DIR, 'docs', 'diagrams', 'generated');
const PUBLIC_DIR = path.join(ROOT_DIR, 'docs', 'public', 'diagrams');

// Ensure output directories exist
[EDITABLE_DIR, GENERATED_DIR, PUBLIC_DIR].forEach(dir => {
  if (!fs.existsSync(dir)) {
    fs.mkdirSync(dir, { recursive: true });
  }
});

console.log('🚀 Starting QuartzDB Excalidraw Migration & Generation Pipeline...');

const builders = buildAllDiagrams();
console.log(`Found ${builders.length} defined diagrams.`);

const results = [];

builders.forEach(builder => {
  const name = builder.name;
  
  // 1. Generate Excalidraw JSON
  const excalidrawJSON = builder.toExcalidrawJSON();
  const editablePath = path.join(EDITABLE_DIR, `${name}.excalidraw`);
  fs.writeFileSync(editablePath, JSON.stringify(excalidrawJSON, null, 2), 'utf8');

  // 2. Generate Hand-Drawn Rough.js SVG
  const svgContent = builder.toSVG();
  const generatedSvgPath = path.join(GENERATED_DIR, `${name}.svg`);
  const publicSvgPath = path.join(PUBLIC_DIR, `${name}.svg`);

  fs.writeFileSync(generatedSvgPath, svgContent, 'utf8');
  fs.writeFileSync(publicSvgPath, svgContent, 'utf8');

  results.push({
    name,
    title: builder.title,
    elements: excalidrawJSON.elements.length,
    svgSize: (svgContent.length / 1024).toFixed(1) + ' KB',
    jsonSize: (JSON.stringify(excalidrawJSON).length / 1024).toFixed(1) + ' KB'
  });
});

console.log('\n📊 Migration & Generation Summary Table:');
console.table(results);
console.log(`\n✨ Successfully generated ${results.length} Excalidraw scenes and SVGs!`);
