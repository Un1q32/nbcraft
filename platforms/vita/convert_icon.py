import sys
from PIL import Image

# LiveArea images must be 8-bit indexed PNGs, otherwise VPK install fails with 0x8010113D
src, dst = sys.argv[1], sys.argv[2]
img = Image.open(src).convert("RGBA").resize((128, 128), Image.LANCZOS)
img.quantize(colors=256, method=Image.FASTOCTREE).save(dst, optimize=True)
