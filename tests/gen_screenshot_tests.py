import argparse
from pathlib import Path

from PIL import Image

FNV_OFFSET_BASIS = 0xCBF29CE484222325
FNV_PRIME = 0x100000001B3
MASK64 = (1 << 64) - 1

MAX_BREAK_FRAMES = 600


def frame_hash(png: Path) -> str:
    image = Image.open(png).convert("RGB")
    if image.size != (160, 144):
        raise SystemExit(f"{png}: expected 160x144, got {image.size}")
    h = FNV_OFFSET_BASIS
    for r, g, b in image.getdata():
        for byte in (b, g, r, 0xFF):
            h = ((h ^ byte) * FNV_PRIME) & MASK64
    return f"{h:016x}"


def first_existing(*paths):
    return next((p for p in paths if p.exists()), None)


def collect(root: Path):
    rows = []

    def add(rom: Path, png, mode, frames):
        if png is not None:
            rows.append((rom.relative_to(root).as_posix(), mode, frames, frame_hash(png)))

    add(root / "dmg-acid2/dmg-acid2.gb", root / "dmg-acid2/dmg-acid2-dmg.png", "frames", 60)
    add(root / "strikethrough/strikethrough.gb", root / "strikethrough/strikethrough-dmg.png", "frames", 60)
    add(root / "little-things-gb/firstwhite.gb", root / "little-things-gb/firstwhite-dmg-cgb.png", "frames", 30)
    add(root / "mooneye-test-suite/manual-only/sprite_priority.gb",
        root / "mooneye-test-suite/manual-only/sprite_priority-dmg.png", "break", MAX_BREAK_FRAMES)

    for rom in sorted((root / "scribbltests").glob("*/*.gb")):
        if rom.stem == "statcount":
            continue
        png = next(iter(sorted(rom.parent.glob("*dmg*.png"))), None)
        add(rom, png, "frames", 600 if rom.stem == "statcount-auto" else 30)

    for rom in sorted((root / "turtle-tests").glob("*/*.gb")):
        add(rom, next(iter(sorted(rom.parent.glob("*.png"))), None), "frames", 30)

    for rom in sorted((root / "mealybug-tearoom-tests").rglob("*.gb")):
        if rom.stem.endswith("-C"):
            continue
        png = first_existing(rom.with_name(f"{rom.stem}_dmg_blob.png"), rom.with_name(f"{rom.stem}_dmg_b.png"))
        add(rom, png, "break", MAX_BREAK_FRAMES)

    for rom in sorted((root / "age-test-roms").rglob("*.gb")):
        add(rom, first_existing(rom.with_name(f"{rom.stem}-dmgC.png")), "break", MAX_BREAK_FRAMES)

    return rows


def main():
    parser = argparse.ArgumentParser(
        description="Write tests/screenshot_tests.txt: expected framebuffer hashes from the ROM bundle's DMG reference images")
    parser.add_argument("roms", type=Path, help="Extracted game-boy-test-roms bundle")
    parser.add_argument("-o", "--output", type=Path, default=Path(__file__).with_name("screenshot_tests.txt"))
    args = parser.parse_args()

    rows = collect(args.roms)
    with open(args.output, "w") as out:
        for rom, mode, frames, digest in rows:
            out.write(f"{rom}\t{mode}\t{frames}\t{digest}\n")
    print(f"wrote {len(rows)} screenshot tests to {args.output}")


if __name__ == "__main__":
    main()
