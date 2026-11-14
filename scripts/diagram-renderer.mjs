import rough from 'roughjs';

// Excalidraw Hand-Drawn Color Palette (System Manual Edition)
export const PALETTE = {
  // Light mode colors
  light: {
    ink: '#1d2939',
    inkMuted: '#5a6778',
    canvasBg: '#fbf8f1',
    cardBg: '#ffffff',
    border: '#1d2939',
    entryStroke: '#2563eb', // Royal Blue
    entryBg: 'rgba(219, 234, 254, 0.65)',
    guardStroke: '#d97706', // Warm Amber
    guardBg: 'rgba(254, 243, 199, 0.75)',
    successStroke: '#16a34a', // Forest Green
    successBg: 'rgba(220, 252, 231, 0.70)',
    errorStroke: '#dc2626', // Vermilion Red
    errorBg: 'rgba(254, 226, 226, 0.75)',
    diskStroke: '#7c3aed', // Purple
    diskBg: 'rgba(243, 232, 255, 0.70)',
    sharedStroke: '#0891b2', // Cyan
    sharedBg: 'rgba(207, 250, 254, 0.70)',
    subgraphStroke: '#94a3b8',
    subgraphBg: 'rgba(248, 250, 252, 0.50)'
  },
  // Dark mode colors
  dark: {
    ink: '#f4efe6',
    inkMuted: '#8e9bae',
    canvasBg: '#141824',
    cardBg: '#191f2e',
    border: '#00d2b4',
    entryStroke: '#60a5fa',
    entryBg: 'rgba(30, 58, 138, 0.45)',
    guardStroke: '#fbbf24',
    guardBg: 'rgba(120, 53, 15, 0.45)',
    successStroke: '#4ade80',
    successBg: 'rgba(20, 83, 45, 0.45)',
    errorStroke: '#f87171',
    errorBg: 'rgba(127, 29, 29, 0.45)',
    diskStroke: '#c084fc',
    diskBg: 'rgba(88, 28, 135, 0.45)',
    sharedStroke: '#22d3ee',
    sharedBg: 'rgba(22, 78, 99, 0.45)',
    subgraphStroke: '#475569',
    subgraphBg: 'rgba(15, 23, 42, 0.40)'
  }
};

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

  // 1. Box / Node
  addBox({ id, x, y, w, h, title, subtitle = '', badge = '', type = 'entry', round = 8, fillStyle = 'solid' }) {
    const elId = id || genId('box');
    const colors = this.getStyleColors(type);

    this.elements.push({
      id: elId,
      kind: 'box',
      type: 'rectangle',
      x, y, width: w, height: h,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle,
      strokeWidth: 1.5,
      strokeStyle: 'solid',
      roughness: 1.1,
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
      roughness: 1.2,
      roundness: { type: 2 },
      title: text,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 3. Cylinder / Database
  addCylinder({ id, x, y, w, h, title, subtitle = '', type = 'disk' }) {
    const elId = id || genId('cyl');
    const colors = this.getStyleColors(type);

    this.elements.push({
      id: elId,
      kind: 'cylinder',
      type: 'rectangle', // represented in excalidraw as group or rounded rect
      x, y, width: w, height: h,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle: 'solid',
      strokeWidth: 1.5,
      strokeStyle: 'solid',
      roughness: 1.1,
      roundness: { type: 3 },
      title,
      subtitle,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 4. Subgraph / Container Box
  addContainer({ id, x, y, w, h, title, type = 'subgraph' }) {
    const elId = id || genId('cont');
    const colors = this.getStyleColors(type);

    this.elements.unshift({ // push to back so it renders behind
      id: elId,
      kind: 'container',
      type: 'rectangle',
      x, y, width: w, height: h,
      strokeColor: colors.stroke,
      backgroundColor: colors.bg,
      fillStyle: 'solid',
      strokeWidth: 1.2,
      strokeStyle: 'dashed',
      roughness: 0.8,
      roundness: { type: 3 },
      title,
      colorType: type,
      seed: this.getSeed()
    });
    return elId;
  }

  // 5. Arrow / Connector
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
      roughness: 1.1,
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

  // 9. Entity Card (ERD)
  addEntityCard({ id, x, y, w, title, fields = [], type = 'entry' }) {
    const elId = id || genId('entity');
    const h = 40 + fields.length * 24 + 10;
    this.elements.push({
      id: elId,
      kind: 'entity_card',
      x, y, width: w, height: h,
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

  // Build standard Excalidraw JSON
  toExcalidrawJSON() {
    const exElements = [];
    let zIndex = 1;

    for (const el of this.elements) {
      if (el.kind === 'box' || el.kind === 'container' || el.kind === 'cylinder') {
        // Shape element
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
          roundness: el.roundness,
          seed: el.seed,
          version: 1,
          versionNonce: zIndex++,
          isDeleted: false,
          boundElements: null,
          updated: 1,
          link: null,
          locked: false
        });

        // Associated text element
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
            fontFamily: 3, // Cascadia/Mono
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
          roundness: el.roundness,
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
            x: midX - 40,
            y: midY - 12,
            width: 80,
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
        // Participant Header Box
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
          roughness: 1,
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
        // Vertical dashed line
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
          roughness: 1,
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
          strokeColor: '#d97706',
          backgroundColor: '#fff9db',
          fillStyle: 'solid',
          strokeWidth: 1.2,
          strokeStyle: 'solid',
          roughness: 1.1,
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
        // Entity card background
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
          roughness: 1,
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
        // Header separator line
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

  // Render authentic Rough.js SVG with Dual-Theme CSS variables
  toSVG() {
    const rc = this.rc;
    let pathsSvg = '';
    let textSvg = '';

    for (const el of this.elements) {
      if (el.kind === 'container') {
        const box = rc.rectangle(el.x, el.y, el.width, el.height, {
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
          textSvg += `<text class="ex-subgraph-title" x="${el.x + 14}" y="${el.y + 22}" font-size="13" font-weight="600" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;
        }
      } else if (el.kind === 'box') {
        const box = rc.rectangle(el.x, el.y, el.width, el.height, {
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

        // Format multiline text cleanly
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
        // Draw cylinder: top ellipse, body lines, bottom curve
        const topEllipse = rc.ellipse(el.x + el.width / 2, el.y + 12, el.width, 24, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed
        });
        const bottomBox = rc.rectangle(el.x, el.y + 12, el.width, el.height - 24, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          fill: el.backgroundColor,
          fillStyle: 'solid',
          seed: el.seed + 1
        });
        const bottomArc = rc.arc(el.x + el.width / 2, el.y + el.height - 12, el.width, 24, 0, Math.PI, false, {
          roughness: el.roughness,
          stroke: el.strokeColor,
          strokeWidth: el.strokeWidth,
          seed: el.seed + 2
        });

        for (const p of [...rc.toPaths(bottomBox), ...rc.toPaths(topEllipse), ...rc.toPaths(bottomArc)]) {
          pathsSvg += `<path class="ex-box-path color-${el.colorType}" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        const lines = el.title.split('<br/>').join('\n').split('\n');
        const startY = el.y + (el.height / 2) - ((lines.length - 1) * 9) + 6;
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
        // Draw connector line segments
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

        // Arrowhead at last segment
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

        // Branch label with clean hand-drawn background pill
        if (el.label) {
          const midIdx = Math.floor(el.points.length / 2);
          const pA = el.points[midIdx - 1];
          const pB = el.points[midIdx];
          const lx = (pA[0] + pB[0]) / 2;
          const ly = (pA[1] + pB[1]) / 2;
          const textW = Math.max(el.label.length * 7.5 + 14, 40);

          const pill = rc.rectangle(lx - textW / 2, ly - 10, textW, 20, {
            roughness: 0.8,
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
        // Participant Header Box
        const pBox = rc.rectangle(el.x - 65, el.y, 130, 44, {
          roughness: 1,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.5,
          fill: 'var(--ex-entry-bg)',
          fillStyle: 'solid',
          seed: el.seed
        });
        // Vertical dashed lifeline
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
          roughness: 1,
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
          roughness: 1,
          stroke: 'var(--ex-ink)',
          strokeWidth: 1.6,
          seed: el.seed + 2
        });
        const tip2 = rc.line(endX, el.y, endX - headLen * Math.cos(a2), el.y - headLen * Math.sin(a2), {
          roughness: 1,
          stroke: 'var(--ex-ink)',
          strokeWidth: 1.6,
          seed: el.seed + 3
        });
        for (const p of [...rc.toPaths(line), ...rc.toPaths(tip1), ...rc.toPaths(tip2)]) {
          pathsSvg += `<path class="ex-seq-path" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="none" />\n`;
        }

        const midX = (startX + endX) / 2;
        const labelText = el.num ? `${el.num}. ${el.text}` : el.text;
        textSvg += `<text class="ex-seq-msg-text" x="${midX}" y="${el.y - 7}" text-anchor="middle" font-size="11.5" font-weight="500" fill="var(--ex-ink)">${escapeHtml(labelText)}</text>\n`;
      } else if (el.kind === 'seq_note') {
        const noteBox = rc.rectangle(el.x, el.y, el.width, el.height, {
          roughness: 1.1,
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
      } else if (el.kind === 'lifeline') {
        // Participant Header Box
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
          roughness: 1,
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
        // Vertical dashed line
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
          roughness: 1,
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
          strokeColor: '#d97706',
          backgroundColor: '#fff9db',
          fillStyle: 'solid',
          strokeWidth: 1.2,
          strokeStyle: 'solid',
          roughness: 1.1,
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
        // Main Card
        const cardBox = rc.rectangle(el.x, el.y, el.width, el.height, {
          roughness: 1,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.5,
          fill: 'var(--ex-card-bg)',
          fillStyle: 'solid',
          seed: el.seed
        });
        // Header separator
        const sepLine = rc.line(el.x, el.y + 34, el.x + el.width, el.y + 34, {
          roughness: 0.8,
          stroke: 'var(--ex-entry-stroke)',
          strokeWidth: 1.2,
          seed: el.seed + 1
        });
        for (const p of [...rc.toPaths(cardBox), ...rc.toPaths(sepLine)]) {
          pathsSvg += `<path class="ex-entity-card-path" d="${p.d}" stroke="${p.stroke}" stroke-width="${p.strokeWidth || 1}" fill="${p.fill || 'none'}" />\n`;
        }

        // Entity Title
        textSvg += `<text class="ex-entity-title" x="${el.x + el.width / 2}" y="${el.y + 22}" text-anchor="middle" font-size="13" font-weight="bold" fill="var(--ex-ink)">${escapeHtml(el.title)}</text>\n`;

        // Field items
        el.fields.forEach((f, idx) => {
          const rowY = el.y + 54 + idx * 24;
          const isPK = f.includes('PK') || f.includes('primary');
          const isFK = f.includes('FK') || f.includes('foreign');
          let badgeColor = 'var(--ex-ink-muted)';
          if (isPK) badgeColor = 'var(--ex-guard-stroke)';
          if (isFK) badgeColor = 'var(--ex-shared-stroke)';

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
        --ex-card-bg: #ffffff;
        --ex-pill-bg: #f6f1e5;
        --ex-border-subtle: rgba(29, 41, 57, 0.35);
        --ex-entry-stroke: #2563eb;
        --ex-entry-bg: rgba(219, 234, 254, 0.65);
        --ex-guard-stroke: #d97706;
        --ex-guard-bg: rgba(254, 243, 199, 0.75);
        --ex-success-stroke: #16a34a;
        --ex-success-bg: rgba(220, 252, 231, 0.70);
        --ex-error-stroke: #dc2626;
        --ex-error-bg: rgba(254, 226, 226, 0.75);
        --ex-disk-stroke: #7c3aed;
        --ex-disk-bg: rgba(243, 232, 255, 0.70);
        --ex-shared-stroke: #0891b2;
        --ex-shared-bg: rgba(207, 250, 254, 0.70);
        --ex-subgraph-stroke: #94a3b8;
        --ex-subgraph-bg: rgba(248, 250, 252, 0.50);
      }

      /* Dark mode overrides (works inside VitePress .dark and iframe/embeds) */
      @media (prefers-color-scheme: dark) {
        .excalidraw-svg {
          --ex-ink: #f4efe6;
          --ex-ink-muted: #8e9bae;
          --ex-card-bg: #191f2e;
          --ex-pill-bg: #21293c;
          --ex-border-subtle: rgba(244, 239, 230, 0.35);
          --ex-entry-stroke: #60a5fa;
          --ex-entry-bg: rgba(30, 58, 138, 0.45);
          --ex-guard-stroke: #fbbf24;
          --ex-guard-bg: rgba(120, 53, 15, 0.45);
          --ex-success-stroke: #4ade80;
          --ex-success-bg: rgba(20, 83, 45, 0.45);
          --ex-error-stroke: #f87171;
          --ex-error-bg: rgba(127, 29, 29, 0.45);
          --ex-disk-stroke: #c084fc;
          --ex-disk-bg: rgba(88, 28, 135, 0.45);
          --ex-shared-stroke: #22d3ee;
          --ex-shared-bg: rgba(22, 78, 99, 0.45);
          --ex-subgraph-stroke: #475569;
          --ex-subgraph-bg: rgba(15, 23, 42, 0.40);
        }
      }

      :root.dark .excalidraw-svg,
      .dark .excalidraw-svg {
        --ex-ink: #f4efe6;
        --ex-ink-muted: #8e9bae;
        --ex-card-bg: #191f2e;
        --ex-pill-bg: #21293c;
        --ex-border-subtle: rgba(244, 239, 230, 0.35);
        --ex-entry-stroke: #60a5fa;
        --ex-entry-bg: rgba(30, 58, 138, 0.45);
        --ex-guard-stroke: #fbbf24;
        --ex-guard-bg: rgba(120, 53, 15, 0.45);
        --ex-success-stroke: #4ade80;
        --ex-success-bg: rgba(20, 83, 45, 0.45);
        --ex-error-stroke: #f87171;
        --ex-error-bg: rgba(127, 29, 29, 0.45);
        --ex-disk-stroke: #c084fc;
        --ex-disk-bg: rgba(88, 28, 135, 0.45);
        --ex-shared-stroke: #22d3ee;
        --ex-shared-bg: rgba(22, 78, 99, 0.45);
        --ex-subgraph-stroke: #475569;
        --ex-subgraph-bg: rgba(15, 23, 42, 0.40);
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

  <!-- Background decorative blueprint grid -->
  <g class="ex-blueprint-grid" opacity="0.12">
    <pattern id="grid_${this.name}" width="20" height="20" patternUnits="userSpaceOnUse">
      <path d="M 20 0 L 0 0 0 20" fill="none" stroke="var(--ex-ink)" stroke-width="0.5"/>
    </pattern>
    <rect width="${this.width}" height="${this.height}" fill="url(#grid_${this.name})" />
  </g>

  <!-- Paths Layer (Rough.js) -->
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
