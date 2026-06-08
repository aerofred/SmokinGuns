#!/usr/bin/env python3
"""Composite the Smokin' Guns icon over a procedural wood texture background."""

import math
import random
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter


def make_wood_texture(size: int, seed: int = 42) -> Image.Image:
    rng = random.Random(seed)
    img = Image.new("RGB", (size, size))
    draw = ImageDraw.Draw(img)

    plank_h = max(48, size // 8)
    colors = [
        (101, 67, 33),
        (120, 78, 42),
        (92, 58, 28),
        (110, 72, 38),
        (88, 54, 26),
    ]

    for y in range(0, size, plank_h):
        base = colors[rng.randrange(len(colors))]
        gap = max(2, size // 256)
        draw.rectangle((0, y, size, min(y + gap, size)), fill=(55, 35, 18))
        plank_end = min(y + plank_h, size)
        for row in range(y + gap, plank_end):
            for x in range(size):
                wave = math.sin(x * 0.018 + row * 0.11 + rng.uniform(-0.2, 0.2)) * 14
                knot = math.exp(-((x - size * 0.35) ** 2 + (row - size * 0.55) ** 2) / (size * 18)) * 28
                knot += math.exp(-((x - size * 0.72) ** 2 + (row - size * 0.28) ** 2) / (size * 22)) * 22
                noise = rng.uniform(-10, 10)
                r = max(0, min(255, int(base[0] + wave + knot + noise)))
                g = max(0, min(255, int(base[1] + wave * 0.7 + knot * 0.8 + noise * 0.8)))
                b = max(0, min(255, int(base[2] + wave * 0.5 + knot * 0.6 + noise * 0.6)))
                img.putpixel((x, row), (r, g, b))

    img = img.filter(ImageFilter.GaussianBlur(radius=max(1, size // 512)))
    return img


def composite_icon(foreground_path: Path, output_path: Path, size: int) -> None:
    wood = make_wood_texture(size)
    fg = Image.open(foreground_path).convert("RGBA")

    scale = 0.88
    target = int(size * scale)
    fg = fg.resize((target, target), Image.Resampling.LANCZOS)

    x = (size - target) // 2
    y = (size - target) // 2
    wood.paste(fg, (x, y), fg)
    wood.save(output_path, "PNG")


def main() -> int:
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} <foreground.png> <output.png> <size>", file=sys.stderr)
        return 1

    foreground = Path(sys.argv[1])
    output = Path(sys.argv[2])
    size = int(sys.argv[3])

    if not foreground.is_file():
        print(f"error: {foreground} not found", file=sys.stderr)
        return 1

    composite_icon(foreground, output, size)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
