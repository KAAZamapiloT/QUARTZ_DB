import rough from 'roughjs';

const rc = rough.generator();

export function roundedRectPath(x, y, w, h, r = 10) {
  r = Math.min(r, w / 2, h / 2);
  return `M${x + r},${y} h${w - 2 * r} a${r},${r} 0 0 1 ${r},${r} v${h - 2 * r} a${r},${r} 0 0 1 -${r},${r} h-${w - 2 * r} a${r},${r} 0 0 1 -${r},-${r} v-${h - 2 * r} a${r},${r} 0 0 1 ${r},-${r} Z`;
}

const pathStr = roundedRectPath(20, 20, 180, 80, 14);
const drawable = rc.path(pathStr, { roughness: 1.1, stroke: '#1d2939', strokeWidth: 1.5, fill: 'rgba(59, 142, 165, 0.15)', fillStyle: 'solid' });
const paths = rc.toPaths(drawable);

console.log('Success! Paths count:', paths.length);
paths.forEach((p, i) => console.log('Path', i, 'd length:', p.d.length, 'fill:', p.fill, 'stroke:', p.stroke));
