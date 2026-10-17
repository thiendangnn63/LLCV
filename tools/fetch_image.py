"""Download a random photo from picsum.photos and save it as a binary PPM (or PGM).

Usage:
    python tools/fetch_image.py                      # data/random_<id>.ppm, 640x480
    python tools/fetch_image.py -W 1024 -H 768 --gray
    python tools/fetch_image.py --seed 42 -o data/photo.ppm   # same image every time

Requires Pillow:  python -m pip install pillow
"""

import argparse
import io
import sys
import urllib.request
from pathlib import Path

try:
    from PIL import Image
except ImportError:
    sys.exit("Pillow is not installed. Run:  python -m pip install pillow")

DATA_DIR = Path(__file__).resolve().parent.parent / "data"


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("-W", "--width", type=int, default=640)
    p.add_argument("-H", "--height", type=int, default=480)
    p.add_argument("--gray", action="store_true", help="save as 1-channel PGM instead of PPM")
    p.add_argument("--seed", help="fixed seed, so the same image comes back every time")
    p.add_argument("-o", "--output", type=Path, help="output path (default: data/random_<id>.ppm/.pgm)")
    args = p.parse_args()

    url = f"https://picsum.photos/{args.width}/{args.height}"
    if args.seed:
        url = f"https://picsum.photos/seed/{args.seed}/{args.width}/{args.height}"

    print(f"Downloading {url} ...")
    req = urllib.request.Request(url, headers={"User-Agent": "llcv-fetch/1.0"})
    with urllib.request.urlopen(req, timeout=30) as resp:
        image_id = resp.headers.get("picsum-id", "x")
        raw = resp.read()

    img = Image.open(io.BytesIO(raw))
    img = img.convert("L" if args.gray else "RGB")

    ext = ".pgm" if args.gray else ".ppm"
    out = args.output or DATA_DIR / f"random_{args.seed or image_id}{ext}"
    out.parent.mkdir(parents=True, exist_ok=True)
    img.save(out)  # Pillow writes binary P5/P6, which load_pnm reads

    print(f"Saved {out} ({img.width}x{img.height}, {'gray' if args.gray else 'RGB'})")


if __name__ == "__main__":
    main()
