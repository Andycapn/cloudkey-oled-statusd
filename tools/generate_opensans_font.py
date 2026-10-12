#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
FONT_PATH = ROOT / "assets" / "fonts" / "OpenSans[wdth,wght].ttf"
OUTPUT_PATH = ROOT / "assets" / "font_opensans.h"
CHARACTERS = "".join(chr(codepoint) for codepoint in range(32, 127))
FONT_SIZES = (("small", 10), ("large", 14))
MAX_GLYPH_WIDTH = 16
MAX_GLYPH_HEIGHT = 24


def visible_bounds(font: ImageFont.FreeTypeFont) -> tuple[int, int, int]:
    boxes = [font.getbbox(character, anchor="ls") for character in CHARACTERS]
    top = min(box[1] for box in boxes)
    bottom = max(box[3] for box in boxes)
    return top, bottom, bottom - top


def rasterize(character: str, font: ImageFont.FreeTypeFont,
              top: int) -> tuple[int, int, int, list[int]]:
    left, _, right, _ = font.getbbox(character, anchor="ls")
    width = max(0, right - left)
    if width > MAX_GLYPH_WIDTH:
        raise ValueError(f"Glyph {character!r} is wider than {MAX_GLYPH_WIDTH} pixels")

    image = Image.new("L", (MAX_GLYPH_WIDTH, MAX_GLYPH_HEIGHT), 0)
    ImageDraw.Draw(image).text((-left, -top), character, font=font, fill=255, anchor="ls")

    rows = []
    for y in range(MAX_GLYPH_HEIGHT):
        bits = 0
        for x in range(MAX_GLYPH_WIDTH):
            coverage = (image.getpixel((x, y)) * 15 + 127) // 255
            bits |= coverage << (x * 4)
        rows.append(bits)

    advance = max(1, round(font.getlength(character)))
    return width, advance, left, rows


def render_font_table(name: str, size: int, font_path: Path) -> list[str]:
    font = ImageFont.truetype(str(font_path), size=size)
    top, _, height = visible_bounds(font)
    if height > MAX_GLYPH_HEIGHT:
        raise ValueError(f"Font visible height {height} exceeds {MAX_GLYPH_HEIGHT} pixels")
    glyphs = [rasterize(character, font, top) for character in CHARACTERS]
    lines = [f"static const struct bitmap_glyph opensans_{name}[95] = {{"]

    for character, (width, advance, x_offset, rows) in zip(CHARACTERS, glyphs):
        row_values = ", ".join(f"0x{bits:016X}ULL" for bits in rows)
        label = "space" if character == " " else character
        lines.append(
            f"    {{ {width}, {advance}, {x_offset}, {{ {row_values} }} }}, /* {label} */"
        )
    lines.extend(("};", ""))
    return lines


def build_header(font_path: Path) -> str:
    lines = [
        "#pragma once",
        "#include <stdint.h>",
        "",
        f"#define OPEN_SANS_MAX_HEIGHT {MAX_GLYPH_HEIGHT}",
        "",
        "struct bitmap_glyph {",
        "    uint8_t width;",
        "    uint8_t advance;",
        "    int8_t x_offset;",
        "    uint64_t rows[OPEN_SANS_MAX_HEIGHT];",
        "};",
        "",
    ]

    for name, size in FONT_SIZES:
        font = ImageFont.truetype(str(font_path), size=size)
        _, _, height = visible_bounds(font)
        lines.append(f"#define OPEN_SANS_{name.upper()}_HEIGHT {height}")
    lines.append("")

    for name, size in FONT_SIZES:
        lines.extend(render_font_table(name, size, font_path))

    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate Open Sans bitmap font tables")
    parser.add_argument("--font", type=Path, default=FONT_PATH)
    parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
    args = parser.parse_args()

    args.output.write_text(build_header(args.font), encoding="utf-8")
    print(f"Wrote {args.output}")


if __name__ == "__main__":
    main()