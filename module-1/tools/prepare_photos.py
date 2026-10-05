"""Optional: rebuild photos.h from the two PNG assets. Requires Pillow."""
from pathlib import Path
from PIL import Image
root = Path(__file__).resolve().parents[1] / 'migration'
W, H = (96, 112)
images = []
rects = []
for name, size in [('lebanese-home', (90, 76)), ('nyc-skyscraper', (72, 108))]:
    im = Image.open(root / 'assets' / f'{name}.png').convert('RGBA')
    im = im.crop(im.getchannel('A').getbbox())
    im.thumbnail(size, Image.Resampling.LANCZOS)
    canvas = Image.new('RGBA', (W, H))
    canvas.paste(im, ((W - im.width) // 2, H - im.height - 2))
    images.append(canvas)
    boxes = []
    for y in range(0, H, 8):
        for x in range(0, W, 8):
            if canvas.crop((x, y, x + 8, y + 8)).getchannel('A').getextrema()[1] > 16:
                boxes.append((x, y, 8, 8))
    rects.append(boxes)
N = max(map(len, rects))
print('Original image fragments', list(map(len, rects)), 'final', N)
for boxes in rects:
    while len(boxes) < N:
        k = max(range(len(boxes)), key=lambda i: boxes[i][2] * boxes[i][3])
        x, y, w, h = boxes.pop(k)
        if w >= h:
            boxes.extend([(x, y, w // 2, h), (x + w // 2, y, w - w // 2, h)])
        else:
            boxes.extend([(x, y, w, h // 2), (x, y + h // 2, w, h - h // 2)])
text = '// Display-ready RGB565 photographic assets and their non-overlapping fragments.\n#pragma once\n#include <stdint.h>\nstruct Anchor { float x,y; };\nstruct PhotoTile { uint8_t x,y,w,h; };\nconstexpr int PHOTO_W=96, PHOTO_H=112;\nconstexpr int PARTICLES=' + str(N) + ';\n'
for name, im, boxes, left in zip(['HOME', 'CITY'], images, rects, [0, 144]):
    text += 'const PhotoTile ' + name + '_TILES[PARTICLES] = {\n' + ''.join((' {%d,%d,%d,%d},\n' % b for b in boxes)) + '};\n'
    text += 'const Anchor ' + name + '[PARTICLES] = {\n' + ''.join((' {%.1ff,%.1ff},\n' % (left + x + w / 2, 13 + y + h / 2) for x, y, w, h in boxes)) + '};\n'
    pixels = list(im.getdata())
    colors = [r >> 3 << 11 | g >> 2 << 5 | b >> 3 for r, g, b, a in pixels]
    alphas = [a for r, g, b, a in pixels]
    for suffix, typ, vals in [('RGB', 'uint16_t', colors), ('ALPHA', 'uint8_t', alphas)]:
        text += 'const ' + typ + ' ' + name + '_' + suffix + '[PHOTO_W*PHOTO_H] PROGMEM = {\n'
        text += '\n'.join((','.join((str(v) for v in vals[i:i + 24])) + ',' for i in range(0, len(vals), 24))) + '\n};\n'
(root / 'photos.h').write_text(text)
