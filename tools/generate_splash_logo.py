#!/usr/bin/env python3
import argparse
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
SOURCE_PATH = ROOT / "assets" / "chaminuka-logo-source.png"
OUTPUT_PATH = ROOT / "assets" / "icons" / "logo_144x50_coverage.bin"
WIDTH = 144
HEIGHT = 50


def build_bitmap(source_path: Path) -> bytes:
    source = Image.open(source_path).convert("RGBA")
    alpha = source.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        raise ValueError("Source logo has no visible pixels")

    alpha = alpha.crop(bounds)
    alpha.thumbnail((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    canvas = Image.new("L", (WIDTH, HEIGHT), 0)
    canvas.paste(alpha, ((WIDTH - alpha.width) // 2, (HEIGHT - alpha.height) // 2))

    coverage = [(value * 15 + 127) // 255 for value in canvas.get_flattened_data()]
    packed = bytearray((len(coverage) + 1) // 2)
    for index in range(0, len(coverage), 2):
        high = coverage[index]
        low = coverage[index + 1] if index + 1 < len(coverage) else 0
        packed[index // 2] = (high << 4) | low
    return bytes(packed)


def main() -> None:
    parser = argparse.ArgumentParser(description="Generate the OLED splash logo coverage bitmap")
    parser.add_argument("--source", type=Path, default=SOURCE_PATH)
    parser.add_argument("--output", type=Path, default=OUTPUT_PATH)
    args = parser.parse_args()
    args.output.write_bytes(build_bitmap(args.source))
    print(f"Wrote {args.output} ({WIDTH}x{HEIGHT}, 4-bit coverage)")


if __name__ == "__main__":
    main()