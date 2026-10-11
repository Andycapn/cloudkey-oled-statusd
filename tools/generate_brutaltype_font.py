#!/usr/bin/env python3
import string
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
FONT_PATH = Path('/Users/andrewndhlovu/PhpstormProjects/AxonPayroll/web/public/fonts/BrutalType.otf')
OUTPUT_PATH = ROOT / 'assets' / 'brutaltype_5x7.h'

CHARSET = ' ' + string.punctuation + string.digits + string.ascii_uppercase + string.ascii_lowercase
ALLOWED = ''.join(ch for ch in CHARSET if ch in {' ', '.', ':', '-', '/', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z'})

# Keep the 5x7 raster format used by the framebuffer renderer.
WIDTH = 5
HEIGHT = 7


def rasterize_char(ch: str, font: ImageFont.FreeTypeFont) -> list[int]:
    image = Image.new('L', (24, 24), 0)
    draw = ImageDraw.Draw(image)
    draw.text((2, 2), ch, font=font, fill=255)

    bbox = draw.textbbox((2, 2), ch, font=font)
    if bbox is None:
        bbox = (0, 0, 0, 0)
    left, top, right, bottom = bbox
    crop = image.crop((max(0, left-2), max(0, top-2), min(24, right+2), min(24, bottom+2)))

    resized = crop.resize((WIDTH, HEIGHT), Image.Resampling.NEAREST)
    matrix = []
    for x in range(WIDTH):
        bits = 0
        for y in range(HEIGHT):
            if resized.getpixel((x, y)) > 128:
                bits |= 1 << y
        matrix.append(bits)
    return matrix


def build_font_table() -> str:
    font = ImageFont.truetype(str(FONT_PATH), size=10)
    glyph_map = {
        ' ': rasterize_char(' ', font),
        '.': rasterize_char('.', font),
        ':': rasterize_char(':', font),
        '-': rasterize_char('-', font),
        '/': rasterize_char('/', font),
    }

    digits = []
    for d in '0123456789':
        digits.append(rasterize_char(d, font))
    glyph_map['digits'] = digits

    alpha = []
    for ch in 'ABCDEFGHIJKLMNOPQRSTUVWXYZ':
        alpha.append(rasterize_char(ch, font))
    glyph_map['alpha_upper'] = alpha

    alpha_lower = []
    for ch in 'abcdefghijklmnopqrstuvwxyz':
        alpha_lower.append(rasterize_char(ch, font))
    glyph_map['alpha_lower'] = alpha_lower

    lines = [
        '#pragma once',
        '#include <stdint.h>',
        '',
        '/* BrutalType-derived 5x7 bitmap font for the monochrome OLED display. */',
        '',
        'static const uint8_t brutaltype_space[5] = {',
        '    ' + ','.join(f'0x{b:02X}' for b in glyph_map[' ']) + ',',
        '};',
        '',
        'static const uint8_t brutaltype_dot[5] = {',
        '    ' + ','.join(f'0x{b:02X}' for b in glyph_map['.']) + ',',
        '};',
        '',
        'static const uint8_t brutaltype_colon[5] = {',
        '    ' + ','.join(f'0x{b:02X}' for b in glyph_map[':']) + ',',
        '};',
        '',
        'static const uint8_t brutaltype_dash[5] = {',
        '    ' + ','.join(f'0x{b:02X}' for b in glyph_map['-']) + ',',
        '};',
        '',
        'static const uint8_t brutaltype_slash[5] = {',
        '    ' + ','.join(f'0x{b:02X}' for b in glyph_map['/']) + ',',
        '};',
        '',
        'static const uint8_t brutaltype_digits[10][5] = {',
    ]

    for idx, glyph in enumerate(glyph_map['digits']):
        lines.append(f'    {{ {", ".join(f"0x{b:02X}" for b in glyph)} }}, /* {idx} */')
    lines.append('};')
    lines.append('')
    lines.append('static const uint8_t brutaltype_alpha[26][5] = {')
    for idx, glyph in enumerate(glyph_map['alpha_upper']):
        lines.append(f'    {{ {", ".join(f"0x{b:02X}" for b in glyph)} }}, /* {chr(65 + idx)} */')
    lines.append('};')
    lines.append('')
    lines.append('static const uint8_t brutaltype_alpha_lower[26][5] = {')
    for idx, glyph in enumerate(glyph_map['alpha_lower']):
        lines.append(f'    {{ {", ".join(f"0x{b:02X}" for b in glyph)} }}, /* {chr(97 + idx)} */')
    lines.append('};')
    lines.append('')
    return '\n'.join(lines) + '\n'


if __name__ == '__main__':
    OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT_PATH.write_text(build_font_table(), encoding='utf-8')
    print(f'Wrote {OUTPUT_PATH}')
