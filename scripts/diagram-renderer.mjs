import rough from 'roughjs';

// Tokyo Hardware Manual (Light) & PC-9801 CRT (Dark) Color Palette
export const PALETTE = {
  light: {
    ink: '#1d2939',
    inkMuted: '#5a6778',
    canvasBg: '#efe8d8',
    cardBg: '#f6f1e5',
    pillBg: '#e3dccb',
    border: '#1d2939',
    borderSubtle: 'rgba(29, 41, 57, 0.25)',
    // Entry / Input / Primary -> Blueprint Cyan
    entryStroke: '#2563eb',
    entryBg: 'rgba(37, 99, 235, 0.10)',
    // Guard / Decision -> Warm Amber / Gold
    guardStroke: '#b45309',
    guardBg: 'rgba(180, 83, 9, 0.10)',
    // Success / Completed -> Moss / Forest Green
    successStroke: '#15803d',
    successBg: 'rgba(21, 128, 61, 0.10)',
    // Error / Abort -> Muted Vermilion Red
    errorStroke: '#d1493e',
    errorBg: 'rgba(209, 73, 62, 0.12)',
    // Disk / Storage -> Slate Indigo / Purple
    diskStroke: '#6d28d9',
    diskBg: 'rgba(109, 40, 217, 0.10)',
    // Shared / Memory / Cache -> Deep Teal / Cyan
    sharedStroke: '#0e7490',
    sharedBg: 'rgba(14, 116, 144, 0.10)',
    // Subgraphs / Containers
    subgraphStroke: '#5a6778',
    subgraphBg: 'rgba(246, 241, 229, 0.65)'
  },
  dark: {
    ink: '#f4efe6',
    inkMuted: '#8e9bae',
    canvasBg: '#141824',
    cardBg: '#191f2e',
    pillBg: '#21293c',
    border: '#00d2b4',
    borderSubtle: 'rgba(0, 210, 180, 0.30)',
    // Entry / Primary -> Retro Teal
    entryStroke: '#00d2b4',
    entryBg: 'rgba(0, 210, 180, 0.14)',
    // Guard / Decision -> PC-98 Amber
    guardStroke: '#ffb000',
    guardBg: 'rgba(255, 176, 0, 0.14)',
    // Success -> Phosphor Emerald
    successStroke: '#10b981',
    successBg: 'rgba(16, 185, 129, 0.14)',
    // Error -> Neon Coral
    errorStroke: '#f87171',
    errorBg: 'rgba(248, 113, 113, 0.16)',
    // Disk / Storage -> Retro Violet
    diskStroke: '#a78bfa',
    diskBg: 'rgba(167, 139, 250, 0.14)',
    // Shared / Memory -> Sky Cyan
    sharedStroke: '#38bdf8',
    sharedBg: 'rgba(56, 189, 248, 0.14)',
    // Subgraphs / Containers
    subgraphStroke: '#475569',
    subgraphBg: 'rgba(25, 31, 46, 0.60)'
  }
};

// Generates smooth SVG path for hand-drawn rounded rectangle
export function roundedRectPath(x, y, w, h, r = 12) {
  r = Math.min(r, w / 2, h / 2);
  return `M${x + r},${y} h${w - 2 * r} a${r},${r} 0 0 1 ${r},${r} v${h - 2 * r} a${r},${r} 0 0 1 -${r},${r} h-${w - 2 * r} a${r},${r} 0 0 1 -${r},-${r} v-${h - 2 * r} a${r},${r} 0 0 1 ${r},-${r} Z`;
}

let idCounter = 1;
function genId(prefix = 'elem') {
  return `${prefix}_${idCounter++}_${Math.random().toString(36).substring(2, 7)}`;
}

export class DiagramBuilder {
  constructor(name, title, width, height) {
    this.name = name;
    this.title = title;
    this.width = width;
    this.height = height;
    this.elements = [];
    this.rc = rough.generator();
    this.nextSeed = 1000;
  }

  getSeed() {
    return this.nextSeed++;
  }

  // 1. Box / Node with Rounded Edges
  addBox({ id, x, y, w, h, title, subtitle = '', badge = '', type = 'entry', round = 12, fillStyle = 'solid' }) {
    const elId = id || genId('box');
    const colors = this.getStyleColors(type);

    this.elements.push({
      id: elId,
      kind: 'box',
      type: 'rectangle',
      x, y, width: w, height: h,
      radius: round,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle,
      strokeWidth: 1.5,
      strokeStyle: 'solid',
      roughness: 0.9,
      roundness: { type: 3 },
      title,
      subtitle,
      badge,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 2. Decision Diamond
  addDiamond({ id, x, y, w, h, text, type = 'guard' }) {
    const elId = id || genId('diamond');
    const colors = this.getStyleColors(type);

    this.elements.push({
      id: elId,
      kind: 'diamond',
      type: 'diamond',
      x, y, width: w, height: h,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle: 'solid',
      strokeWidth: 1.5,
      strokeStyle: 'solid',
      roughness: 1.0,
      roundness: { type: 2 },
      title: text,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 3. Cylinder / Database Node (Smooth Rounded Capsule)
  addCylinder({ id, x, y, w, h, title, subtitle = '', type = 'disk' }) {
    const elId = id || genId('cyl');
    const colors = this.getStyleColors(type);

    this.elements.push({
      id: elId,
      kind: 'cylinder',
      type: 'rectangle',
      x, y, width: w, height: h,
      radius: 12,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle: 'solid',
      strokeWidth: 1.5,
      strokeStyle: 'solid',
      roughness: 0.9,
      roundness: { type: 3 },
      title,
      subtitle,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 4. Subgraph / Container Box with Rounded Edges
  addContainer({ id, x, y, w, h, title, type = 'subgraph' }) {
    const elId = id || genId('cont');
    const colors = this.getStyleColors(type);

    this.elements.unshift({
      id: elId,
      kind: 'container',
      type: 'rectangle',
      x, y, width: w, height: h,
      radius: 16,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle: 'solid',
      strokeWidth: 1.3,
      strokeStyle: 'dashed',
      roughness: 0.7,
      roundness: { type: 3 },
      title,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 5. Arrow / Connector with Natural Curves
  addArrow({ points, label = '', dashed = false, type = 'default' }) {
    const elId = genId('arrow');
    const colors = this.getStyleColors(type);
    const start = points[0];
    const end = points[points.length - 1];

    this.elements.push({
      id: elId,
      kind: 'arrow',
      type: 'arrow',
      points,
      x: start[0],
      y: start[1],
      width: end[0] - start[0],
      height: end[1] - start[1],
      strokeColor: colors.stroke,
      strokeWidth: 1.5,
      strokeStyle: dashed ? 'dashed' : 'solid',
      roughness: 0.9,
      label,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 6. Sequence Lifeline
  addLifeline({ id, x, topY, height, name, role = '' }) {
    const elId = id || genId('life');
    this.elements.push({
      id: elId,
      kind: 'lifeline',
      x, y: topY, width: 140, height,
      title: name,
      subtitle: role,
      seed: this.getSeed()
    });
    return elId;
  }

  // 7. Sequence Message Arrow
  addSequenceMessage({ fromX, toX, y, num, text, dashed = false, type = 'default' }) {
    const elId = genId('seq_msg');
    this.elements.push({
      id: elId,
      kind: 'seq_message',
      fromX, toX, y,
      num, text,
      dashed,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 8. Sequence Note Box
  addSequenceNote({ x, y, w, h, text, type = 'guard' }) {
    const elId = genId('seq_note');
    this.elements.push({
      id: elId,
      kind: 'seq_note',
      x, y, width: w, height: h,
      title: text,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 9. Entity Card (ERD) with Rounded Edges
  addEntityCard({ id, x, y, w, title, fields = [], type = 'entry' }) {
    const elId = id || genId('entity');
    const h = 42 + fields.length * 24 + 10;
    this.elements.push({
      id: elId,
      kind: 'entity_card',
      x, y, width: w, height: h,
      radius: 12,
      title,
      fields,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  getStyleColors(type) {
    const p = PALETTE.light;
    switch (type) {
      case 'entry':
        return { stroke: p.entryStroke, bg: p.entryBg };
      case 'guard':
        return { stroke: p.guardStroke, bg: p.guardBg };
      case 'success':
        return { stroke: p.successStroke, bg: p.successBg };
      case 'error':
        return { stroke: p.errorStroke, bg: p.errorBg };
      case 'disk':
        return { stroke: p.diskStroke, bg: p.diskBg };
      case 'shared':
        return { stroke: p.sharedStroke, bg: p.sharedBg };
      case 'subgraph':
        return { stroke: p.subgraphStroke, bg: p.subgraphBg };
      default:
        return { stroke: p.ink, bg: 'transparent' };
    }
  }

  // Build standard Excalidraw JSON v2
  toExcalidrawJSON() {
    const exElements = [];
    let zIndex = 1;

    for (const el of this.elements) {
      if (el.kind === 'box' || el.kind === 'container' || el.kind === 'cylinder') {
        exElements.push({
          id: el.id,
          type: 'rectangle',
          x: el.x,
          y: el.y,
          width: el.width,
          height: el.height,
          angle: 0,
          strokeColor: el.strokeColor,
          backgroundColor: el.backgroundColor,
          fillStyle: el.fillStyle,
          strokeWidth: el.strokeWidth,
          strokeStyle: el.strokeStyle,
          roughness: el.roughness,
          opacity: 100,
          groupIds: [],
          roundness: { type: 3 }, // Rounded corners
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });

        if (el.title) {
          const fullText = el.subtitle ? `${el.title}\n${el.subtitle}` : el.title;
          exElements.push({
            id: `${el.id}_txt`,
            type: 'text',
            x: el.x + 10,
            y: el.y + 12,
            width: el.width - 20,
            height: el.height - 24,
            angle: 0,
            strokeColor: '#1d2939',
            backgroundColor: 'transparent',
            fillStyle: 'solid',
            strokeWidth: 1,
            strokeStyle: 'solid',
            roughness: 1,
            opacity: 100,
            groupIds: [],
            roundness: null,
            seed: el.seed + 1,
            version: 1,
            versionNonce: zIndex++,
            isDeleted: false,
            boundElements: null,
            updated: 1,
            link: null,
            locked: false,
            text: fullText,
            fontSize: el.kind === 'container' ? 14 : 13,
            fontFamily: 3,
            textAlign: el.kind === 'container' ? 'left' : 'center',
            verticalAlign: el.kind === 'container' ? 'top' : 'middle',
            baseline: 14,
            containerId: el.id,
            originalText: fullText,
            lineHeight: 1.25
          });
        }
      } else if (el.kind === 'diamond') {
        exElements.push({
          id: el.id,
          type: 'diamond',
          x: el.x,
          y: el.y,
          width: el.width,
          height: el.height,
          angle: 0,
          strokeColor: el.strokeColor,
          backgroundColor: el.backgroundColor,
          fillStyle: el.fillStyle,
          strokeWidth: el.strokeWidth,
          strokeStyle: el.strokeStyle,
          roughness: el.roughness,
          opacity: 100,
          groupIds: [],
          roundness: { type: 2 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });
        if (el.title) {
          exElements.push({
            id: `${el.id}_txt`,
            type: 'text',
            x: el.x + 15,
            y: el.y + 15,
            width: el.width - 30,
            height: el.height - 30,
            angle: 0,
            strokeColor: '#1d2939',
            backgroundColor: 'transparent',
            fillStyle: 'solid',
            strokeWidth: 1,
            strokeStyle: 'solid',
            roughness: 1,
            opacity: 100,
            groupIds: [],
            roundness: null,
            seed: el.seed + 1,
            version: 1,
            versionNonce: zIndex++,
            isDeleted: false,
            boundElements: null,
            updated: 1,
            link: null,
            locked: false,
            text: el.title,
            fontSize: 12,
            fontFamily: 3,
            textAlign: 'center',
            verticalAlign: 'middle',
            baseline: 14,
            containerId: el.id,
            originalText: el.title,
            lineHeight: 1.25
          });
        }
      } else if (el.kind === 'arrow') {
        const start = el.points[0];
        const end = el.points[el.points.length - 1];
        const relPoints = el.points.map(p => [p[0] - start[0], p[1] - start[1]]);
        exElements.push({
          id: el.id,
          type: 'arrow',
          x: start[0],
          y: start[1],
          width: end[0] - start[0],
          height: end[1] - start[1],
          angle: 0,
          strokeColor: el.strokeColor,
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: el.strokeWidth,
          strokeStyle: el.strokeStyle,
          roughness: el.roughness,
          opacity: 100,
          groupIds: [],
          roundness: { type: 2 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          points: relPoints,
          lastCommittedPoint: null,
          startBinding: null,
          endBinding: null,
          startArrowhead: null,
          endArrowhead: 'arrow'
        });
        if (el.label) {
          const midX = (start[0] + end[0]) / 2;
          const midY = (start[1] + end[1]) / 2;
          exElements.push({
            id: `${el.id}_lbl`,
            type: 'text',
            x: midX - 45,
            y: midY - 12,
            width: 90,
            height: 20,
            angle: 0,
            strokeColor: '#5a6778',
            backgroundColor: 'transparent',
            fillStyle: 'solid',
            strokeWidth: 1,
            strokeStyle: 'solid',
            roughness: 1,
            opacity: 100,
            groupIds: [],
            roundness: null,
            seed: el.seed + 1,
            version: 1,
            versionNonce: zIndex++,
            isDeleted: false,
            boundElements: null,
            updated: 1,
            link: null,
            locked: false,
            text: el.label,
            fontSize: 11,
            fontFamily: 3,
            textAlign: 'center',
            verticalAlign: 'middle',
            baseline: 12,
            containerId: null,
            originalText: el.label,
            lineHeight: 1.2
          });
        }
      } else if (el.kind === 'lifeline') {
        exElements.push({
          id: el.id,
          type: 'rectangle',
          x: el.x - 65,
          y: el.y,
          width: 130,
          height: 44,
          angle: 0,
          strokeColor: '#2563eb',
          backgroundColor: '#e7f5ff',
          fillStyle: 'solid',
          strokeWidth: 1.5,
          strokeStyle: 'solid',
          roughness: 0.9,
          opacity: 100,
          groupIds: [],
          roundness: { type: 3 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });
        exElements.push({
          id: `${el.id}_txt`,
          type: 'text',
          x: el.x - 60,
          y: el.y + 12,
          width: 120,
          height: 20,
          angle: 0,
          strokeColor: '#1d2939',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1,
          strokeStyle: 'solid',
          roughness: 1,
          opacity: 100,
          groupIds: [],
          roundness: null,
          seed: el.seed + 1,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          text: el.title,
          fontSize: 13,
          fontFamily: 3,
          textAlign: 'center',
          verticalAlign: 'middle',
          baseline: 14,
          containerId: el.id,
          originalText: el.title,
          lineHeight: 1.25
        });
        exElements.push({
          id: `${el.id}_line`,
          type: 'line',
          x: el.x,
          y: el.y + 44,
          width: 0,
          height: el.height - 44,
          angle: 0,
          strokeColor: '#5a6778',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1.2,
          strokeStyle: 'dashed',
          roughness: 0.8,
          opacity: 100,
          groupIds: [],
          roundness: null,
          seed: el.seed + 2,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          points: [[0, 0], [0, el.height - 44]],
          lastCommittedPoint: null,
          startBinding: null,
          endBinding: null,
          startArrowhead: null,
          endArrowhead: null
        });
      } else if (el.kind === 'seq_message') {
        const dx = el.toX - el.fromX;
        exElements.push({
          id: el.id,
          type: 'arrow',
          x: el.fromX,
          y: el.y,
          width: dx,
          height: 0,
          angle: 0,
          strokeColor: '#1d2939',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1.5,
          strokeStyle: el.dashed ? 'dashed' : 'solid',
          roughness: 0.9,
          opacity: 100,
          groupIds: [],
          roundness: { type: 2 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          points: [[0, 0], [dx, 0]],
          lastCommittedPoint: null,
          startBinding: null,
          endBinding: null,
          startArrowhead: null,
          endArrowhead: 'arrow'
        });
        const midX = (el.fromX + el.toX) / 2;
        const msgText = el.num ? `${el.num}. ${el.text}` : el.text;
        exElements.push({
          id: `${el.id}_txt`,
          type: 'text',
          x: midX - 100,
          y: el.y - 20,
          width: 200,
          height: 18,
          angle: 0,
          strokeColor: '#1d2939',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1,
          strokeStyle: 'solid',
          roughness: 1,
          opacity: 100,
          groupIds: [],
          roundness: null,
          seed: el.seed + 1,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          text: msgText,
          fontSize: 12,
          fontFamily: 3,
          textAlign: 'center',
          verticalAlign: 'middle',
          baseline: 13,
          containerId: null,
          originalText: msgText,
          lineHeight: 1.2
        });
      } else if (el.kind === 'seq_note') {
        exElements.push({
          id: el.id,
          type: 'rectangle',
          x: el.x,
          y: el.y,
          width: el.width,
          height: el.height,
          angle: 0,
          strokeColor: '#b45309',
          backgroundColor: '#fff9db',
          fillStyle: 'solid',
          strokeWidth: 1.2,
          strokeStyle: 'solid',
          roughness: 0.9,
          opacity: 100,
          groupIds: [],
          roundness: { type: 3 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });
        exElements.push({
          id: `${el.id}_txt`,
          type: 'text',
          x: el.x + 10,
          y: el.y + 6,
          width: el.width - 20,
          height: el.height - 12,
          angle: 0,
          strokeColor: '#1d2939',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1,
          strokeStyle: 'solid',
          roughness: 1,
          opacity: 100,
          groupIds: [],
          roundness: null,
          seed: el.seed + 1,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          text: el.title,
          fontSize: 11.5,
          fontFamily: 3,
          textAlign: 'center',
          verticalAlign: 'middle',
          baseline: 12,
          containerId: el.id,
          originalText: el.title,
          lineHeight: 1.2
        });
      } else if (el.kind === 'entity_card') {
        exElements.push({
          id: el.id,
          type: 'rectangle',
          x: el.x,
          y: el.y,
          width: el.width,
          height: el.height,
          angle: 0,
          strokeColor: '#2563eb',
          backgroundColor: '#ffffff',
          fillStyle: 'solid',
          strokeWidth: 1.5,
          strokeStyle: 'solid',
          roughness: 0.9,
          opacity: 100,
          groupIds: [],
          roundness: { type: 3 },
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });
        exElements.push({
          id: `${el.id}_line`,
          type: 'line',
          x: el.x,
          y: el.y + 36,
          width: el.width,
          height: 0,
          angle: 0,
          strokeColor: '#2563eb',
          backgroundColor: 'transparent',
          fillStyle: 'solid',
          strokeWidth: 1.2,
          strokeStyle: 'solid',
          roughness: 0.8,
          opacity: 100,
          groupIds: [],
          roundness: null,
          seed: el.seed + 1,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false,
          points: [[0, 0], [el.width, 0]],
          lastCommittedPoint: null,
          startBinding: null,
          endBinding: null,
          startArrowhead: null,
          endArrowhead: null
        });
      }
    }

    return {
      type: 'excalidraw',
      version: 2,
      source: 'https://excalidraw.com',
      elements: exElements,
      appState: {
        gridSize: null,
        viewBackgroundColor: '#efe8d8'
      },
      files: {}
    };
  }

  // Render authentic Rough.js SVG with Hand-Drawn Rounded Fillets & Dual-Theme CSS
  toSVG() {
    const rc = this.rc;
    let pathsSvg = '';
    let textSvg = '';

    for (const el of this.elements) {
      if (el.kind === 'container') {
        const pathStr = roundedRectPath(el.x, el.y, el.width, el.height, el.radius || 16);
        const box = rc.path(pathStr, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          strokeLineDash: [6, 4],
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed
        });
        for (const p of rc.toPaths(box)) {
          pathsSvg += `<path class="ex-subgraph-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }
        if (el.title) {
          textSvg += `<text class="ex-subgraph-title" x="${el.x + 18}" y="${el.y + 24}" font-size="13" font-weight="700" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;
        }
      } else if (el.kind === 'box') {
        const pathStr = roundedRectPath(el.x, el.y, el.width, el.height, el.radius || 12);
        const box = rc.path(pathStr, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed
        });
        for (const p of rc.toPaths(box)) {
          pathsSvg += `<path class="ex-box-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        const lines = el.title.split('<br/>').join('\n').split('\n');
        const startY = el.y + (el.height / 2) - ((lines.length - 1) * 9);
        textSvg += `<g class="ex-box-text color-${el.colorType}">\n`;
        lines.forEach((line, idx) => {
          const isHeader = idx === 0;
          const fontSize = isHeader ? 13 : 11.5;
          const fontWeight = isHeader ? 'bold' : 'normal';
          const fill = isHeader ? 'var(--ex-ink)' : 'var(--ex-ink-muted)';
          textSvg += `  <text x="${el.x + el.width / 2}" y="${startY + (idx * 18)}" text-anchor="middle" font-size="${fontSize}" font-weight="${fontWeight}" fill="${fill}">${escapeHtml(line)}</text>\n`;
        });
        textSvg += `</g>\n`;
      } else if (el.kind === 'cylinder') {
        // Rounded cylinder
        const pathStr = roundedRectPath(el.x, el.y, el.width, el.height, 14);
        const box = rc.path(pathStr, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed
        });
        // Top ellipse rim
        const topEllipse = rc.ellipse(el.x + el.width / 2, el.y + 14, el.width - 4, 18, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          seed: el.seed + 1
        });

        for (const p of [...rc.toPaths(box), ...rc.toPaths(topEllipse)]) {
          pathsSvg += `<path class="ex-box-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        const lines = el.title.split('<br/>').join('\n').split('\n');
        const startY = el.y + (el.height / 2) - ((lines.length - 1) * 9) + 4;
        textSvg += `<g class="ex-box-text color-${el.colorType}">\n`;
        lines.forEach((line, idx) => {
          const isHeader = idx === 0;
          const fontSize = isHeader ? 13 : 11.5;
          const fontWeight = isHeader ? 'bold' : 'normal';
          const fill = isHeader ? 'var(--ex-ink)' : 'var(--ex-ink-muted)';
          textSvg += `  <text x="${el.x + el.width / 2}" y="${startY + (idx * 18)}" text-anchor="middle" font-size="${fontSize}" font-weight="${fontWeight}" fill="${fill}">${escapeHtml(line)}</text>\n`;
        });
        textSvg += `</g>\n`;
      } else if (el.kind === 'diamond') {
        const cx = el.x + el.width / 2;
        const cy = el.y + el.height / 2;
        const poly = rc.polygon([
          [cx, el.y],
          [el.x + el.width, cy],
          [cx, el.y + el.height],
          [el.x, cy]
        ], {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed
        });
        for (const p of rc.toPaths(poly)) {
          pathsSvg += `<path class="ex-diamond-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        const lines = el.title.split('<br/>').join('\n').split('\n');
        const startY = cy - ((lines.length - 1) * 8);
        textSvg += `<g class="ex-diamond-text color-${el.colorType}">\n`;
        lines.forEach((line, idx) => {
          textSvg += `  <text x="${cx}" y="${startY + (idx * 16)}" text-anchor="middle" font-size="11.5" font-weight="bold" fill="var(--ex-ink)">${escapeHtml(line)}</text>\n`;
        });
        textSvg += `</g>\n`;
      } else if (el.kind === 'arrow') {
        for (let i = 0; i < el.points.length - 1; i++) {
          const p1 = el.points[i];
          const p2 = el.points[i + 1];
          const line = rc.line(p1[0], p1[1], p2[0], p2[1], {
            roughness: el.roughness,
            stroke: el.strokeColor,
            strokeWidth: el.strokeWidth,
            strokeLineDash: el.strokeStyle === 'dashed' ? [6, 4] : undefined,
            seed: el.seed + i
          });
          for (const p of rc.toPaths(line)) {
            pathsSvg += `<path class="ex-arrow-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="none" />\n`;
          }
        }

        const pPenult = el.points[el.points.length - 2];
        const pEnd = el.points[el.points.length - 1];
        const angle = Math.atan2(pEnd[1] - pPenult[1], pEnd[0] - pPenult[0]);
        const headLen = 13;
        const a1 = angle - Math.PI / 6;
        const a2 = angle + Math.PI / 6;

        const tipLine1 = rc.line(pEnd[0], pEnd[1], pEnd[0] - headLen * Math.cos(a1), pEnd[1] - headLen * Math.sin(a1), {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth + 0.3,
          seed: el.seed + 99
        });
        const tipLine2 = rc.line(pEnd[0], pEnd[1], pEnd[0] - headLen * Math.cos(a2), pEnd[1] - headLen * Math.sin(a2), {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth + 0.3,
          seed: el.seed + 100
        });

        for (const p of [...rc.toPaths(tipLine1), ...rc.toPaths(tipLine2)]) {
          pathsSvg += `<path class="ex-arrow-head color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="none" />\n`;
        }

        // Branch label with soft rounded pill
        if (el.label) {
          const midIdx = Math.floor(el.points.length / 2);
          const pA = el.points[midIdx - 1];
          const pB = el.points[midIdx];
          const lx = (pA[0] + pB[0]) / 2;
          const ly = (pA[1] + pB[1]) / 2;
          const textW = Math.max(el.label.length * 7.5 + 16, 44);

          const pillPath = roundedRectPath(lx - textW / 2, ly - 11, textW, 22, 6);
          const pill = rc.path(pillPath, {
            roughness: 0.6,
            stroke: 'var(--ex-border-subtle)',
            strokeWidth: 1,
            fill: 'var(--ex-pill-bg)',
            fillStyle: 'solid',
            seed: el.seed + 150
          });
          for (const p of rc.toPaths(pill)) {
            pathsSvg += `<path class="ex-label-pill" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
          }
          textSvg += `<text class="ex-arrow-label" x="${lx}" y="${ly + 4}" text-anchor="middle" font-size="11" font-weight="600" fill="var(--ex-ink)">${escapeHtml(el.label)}</text>\n`;
        }
      } else if (el.kind === 'lifeline') {
        const pBoxPath = roundedRectPath(el.x - 65, el.y, 130, 44, 10);
        const pBox = rc.path(pBoxPath, {
          roughness: 0.8,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.5,
          fill: 'var(--ex-entry-bg)',
          fillStyle: 'solid',
          seed: el.seed
        });
        const line = rc.line(el.x, el.y + 44, el.x, el.y + el.height, {
          roughness: 0.8,
          stroke: 'var(--ex-ink-muted)',
          strokeWidth: 1.2,
          strokeLineDash: [6, 6],
          seed: el.seed + 1
        });
        for (const p of [...rc.toPaths(pBox), ...rc.toPaths(line)]) {
          pathsSvg += `<path class="ex-lifeline-path" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }
        textSvg += `<text class="ex-participant-title" x="${el.x}" y="${el.y + 24}" text-anchor="middle" font-size="12" font-weight="bold" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;
        if (el.subtitle) {
          textSvg += `<text class="ex-participant-sub" x="${el.x}" y="${el.y + 37}" text-anchor="middle" font-size="10" fill="var(--ex-ink-muted)">${escapeHtml(el.subtitle)}</text>\n`;
        }
      } else if (el.kind === 'seq_message') {
        const leftToRight = el.toX > el.fromX;
        const startX = el.fromX;
        const endX = el.toX;
        const line = rc.line(startX, el.y, endX, el.y, {
          roughness: 0.9,
          stroke: 'var(--ex-ink)',
          strokeWidth: 1.5,
          strokeLineDash: el.dashed ? [6, 4] : undefined,
          seed: el.seed
        });
        const headAngle = leftToRight ? 0 : Math.PI;
        const headLen = 11;
        const a1 = headAngle - Math.PI / 6;
        const a2 = headAngle + Math.PI / 6;

        const tip1 = rc.line(endX, el.y, endX - headLen * Math.cos(a1), el.y - headLen * Math.sin(a1), {
          roughness: 0.9,
          stroke: 'var(--ex-ink)',
          strokeWidth: 1.6,
          seed: el.seed + 2
        });
        const tip2 = rc.line(endX, el.y, endX - headLen * Math.cos(a2), el.y - headLen * Math.sin(a2), {
          roughness: 0.9,
          stroke: 'var(--ex-ink)',
          strokeWidth: 1.6,
          seed: el.seed + 3
        });
        for (const p of [...rc.toPaths(line), ...rc.toPaths(tip1), ...rc.toPaths(tip2)]) {
          pathsSvg += `<path class="ex-seq-path" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="none" />\n`;
        }

        const midX = (startX + endX) / 2;
        const labelText = el.num ? `${el.num}. ${el.text}` : el.text;
        textSvg += `<text class="ex-seq-msg-text" x="${midX}" y="${el.y - 7}" text-anchor="middle" font-size="11.5" font-weight="600" fill="var(--ex-ink)">${escapeHtml(labelText)}</text>\n`;
      } else if (el.kind === 'seq_note') {
        const notePath = roundedRectPath(el.x, el.y, el.width, el.height, 8);
        const noteBox = rc.path(notePath, {
          roughness: 0.9,
          stroke: 'var(--ex-guard-stroke)',
          strokeWidth: 1.2,
          fill: 'var(--ex-guard-bg)',
          fillStyle: 'solid',
          seed: el.seed
        });
        for (const p of rc.toPaths(noteBox)) {
          pathsSvg += `<path class="ex-seq-note" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }
        textSvg += `<text class="ex-seq-note-text" x="${el.x + el.width / 2}" y="${el.y + el.height / 2 + 4}" text-anchor="middle" font-size="11" font-weight="600" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;
      } else if (el.kind === 'entity_card') {
        const cardPath = roundedRectPath(el.x, el.y, el.width, el.height, el.radius || 12);
        const cardBox = rc.path(cardPath, {
          roughness: 0.8,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.5,
          fill: 'var(--ex-card-bg)',
          fillStyle: 'solid',
          seed: el.seed
        });
        const sepLine = rc.line(el.x, el.y + 36, el.x + el.width, el.y + 36, {
          roughness: 0.6,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.2,
          seed: el.seed + 1
        });
        for (const p of [...rc.toPaths(cardBox), ...rc.toPaths(sepLine)]) {
          pathsSvg += `<path class="ex-entity-card-path" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        textSvg += `<text class="ex-entity-title" x="${el.x + el.width / 2}" y="${el.y + 23}" text-anchor="middle" font-size="13" font-weight="bold" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;

        el.fields.forEach((f, idx) => {
          const rowY = el.y + 56 + idx * 24;
          textSvg += `<text class="ex-entity-field" x="${el.x + 16}" y="${rowY}" font-size="11.5" font-family="'JetBrains Mono', monospace" fill="var(--ex-ink)">${escapeHtml(f)}</text>\n`;
        });
      }
    }

    return `<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 ${this.width} ${this.height}" width="100%" height="auto" class="excalidraw-svg">
  <defs>
    <style>
      .excalidraw-svg {
        font-family: 'JetBrains Mono', 'Space Grotesk', -apple-system, BlinkMacSystemFont, sans-serif;
        --ex-ink: #1d2939;
        --ex-ink-muted: #5a6778;
        --ex-card-bg: #f6f1e5;
        --ex-pill-bg: #e3dccb;
        --ex-border-subtle: rgba(29, 41, 57, 0.30);
        --ex-entry-stroke: #2563eb;
        --ex-entry-bg: rgba(37, 99, 235, 0.10);
        --ex-guard-stroke: #b45309;
        --ex-guard-bg: rgba(180, 83, 9, 0.10);
        --ex-success-stroke: #15803d;
        --ex-success-bg: rgba(21, 128, 61, 0.10);
        --ex-error-stroke: #d1493e;
        --ex-error-bg: rgba(209, 73, 62, 0.12);
        --ex-disk-stroke: #6d28d9;
        --ex-disk-bg: rgba(109, 40, 217, 0.10);
        --ex-shared-stroke: #0e7490;
        --ex-shared-bg: rgba(14, 116, 144, 0.10);
        --ex-subgraph-stroke: #5a6778;
        --ex-subgraph-bg: rgba(246, 241, 229, 0.65);
      }

      @media (prefers-color-scheme: dark) {
        .excalidraw-svg {
          --ex-ink: #f4efe6;
          --ex-ink-muted: #8e9bae;
          --ex-card-bg: #191f2e;
          --ex-pill-bg: #21293c;
          --ex-border-subtle: rgba(0, 210, 180, 0.30);
          --ex-entry-stroke: #00d2b4;
          --ex-entry-bg: rgba(0, 210, 180, 0.14);
          --ex-guard-stroke: #ffb000;
          --ex-guard-bg: rgba(255, 176, 0, 0.14);
          --ex-success-stroke: #10b981;
          --ex-success-bg: rgba(16, 185, 129, 0.14);
          --ex-error-stroke: #f87171;
          --ex-error-bg: rgba(248, 113, 113, 0.16);
          --ex-disk-stroke: #a78bfa;
          --ex-disk-bg: rgba(167, 139, 250, 0.14);
          --ex-shared-stroke: #38bdf8;
          --ex-shared-bg: rgba(56, 189, 248, 0.14);
          --ex-subgraph-stroke: #475569;
          --ex-subgraph-bg: rgba(25, 31, 46, 0.60);
        }
      }

      :root.dark .excalidraw-svg,
      .dark .excalidraw-svg {
        --ex-ink: #f4efe6;
        --ex-ink-muted: #8e9bae;
        --ex-card-bg: #191f2e;
        --ex-pill-bg: #21293c;
        --ex-border-subtle: rgba(0, 210, 180, 0.30);
        --ex-entry-stroke: #00d2b4;
        --ex-entry-bg: rgba(0, 210, 180, 0.14);
        --ex-guard-stroke: #ffb000;
        --ex-guard-bg: rgba(255, 176, 0, 0.14);
        --ex-success-stroke: #10b981;
        --ex-success-bg: rgba(16, 185, 129, 0.14);
        --ex-error-stroke: #f87171;
        --ex-error-bg: rgba(248, 113, 113, 0.16);
        --ex-disk-stroke: #a78bfa;
        --ex-disk-bg: rgba(167, 139, 250, 0.14);
        --ex-shared-stroke: #38bdf8;
        --ex-shared-bg: rgba(56, 189, 248, 0.14);
        --ex-subgraph-stroke: #475569;
        --ex-subgraph-bg: rgba(25, 31, 46, 0.60);
      }

      .color-entry { stroke: var(--ex-entry-stroke) !important; fill: var(--ex-entry-bg) !important; }
      .color-guard { stroke: var(--ex-guard-stroke) !important; fill: var(--ex-guard-bg) !important; }
      .color-success { stroke: var(--ex-success-stroke) !important; fill: var(--ex-success-bg) !important; }
      .color-error { stroke: var(--ex-error-stroke) !important; fill: var(--ex-error-bg) !important; }
      .color-disk { stroke: var(--ex-disk-stroke) !important; fill: var(--ex-disk-bg) !important; }
      .color-shared { stroke: var(--ex-shared-stroke) !important; fill: var(--ex-shared-bg) !important; }
      .color-subgraph { stroke: var(--ex-subgraph-stroke) !important; fill: var(--ex-subgraph-bg) !important; }

      .ex-arrow-path { stroke: var(--ex-ink) !important; }
      .ex-arrow-head { stroke: var(--ex-ink) !important; }
    </style>
  </defs>

  <!-- Background blueprint engineering grid -->
  <g class="ex-blueprint-grid" opacity="0.08">
    <pattern id="grid_${this.name}" width="24" height="24" patternUnits="userSpaceOnUse">
      <path d="M 24 0 L 0 0 0 24" fill="none" stroke="var(--ex-ink)" stroke-width="0.5"/>
    </pattern>
    <rect width="${this.width}" height="${this.height}" fill="url(#grid_${this.name})" />
  </g>

  <!-- Paths Layer (Rough.js with Rounded Edges) -->
  <g class="ex-paths-layer">
${pathsSvg}
  </g>

  <!-- Text Layer -->
  <g class="ex-text-layer">
${textSvg}
  </g>
</svg>`;
  }
}

function escapeHtml(str) {
  if (!str) return '';
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}
