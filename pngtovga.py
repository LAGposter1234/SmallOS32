#!/usr/bin/env python3
import sys
import os
from PIL import Image

W = 320
H = 240

PALETTE = [
    (0, 0, 0),
    (0, 0, 170),
    (0, 170, 0),
    (0, 170, 170),
    (170, 0, 0),
    (170, 0, 170),
    (170, 85, 0),
    (170, 170, 170),
    (85, 85, 85),
    (85, 85, 255),
    (85, 255, 85),
    (85, 255, 255),
    (255, 85, 85),
    (255, 85, 255),
    (255, 255, 85),
    (255, 255, 255)
]

BPP2_PALETTE = [0, 8, 7, 15]

def color_index(rgb, palette):
    best = 0
    best_dist = None

    for index in palette:
        color = PALETTE[index]

        d = (
            (rgb[0] - color[0]) ** 2 +
            (rgb[1] - color[1]) ** 2 +
            (rgb[2] - color[2]) ** 2
        )

        if best_dist is None or d < best_dist:
            best_dist = d
            best = index

    return palette.index(best)

def brightness(rgb):
    return (
        rgb[0] * 299 +
        rgb[1] * 587 +
        rgb[2] * 114
    ) // 1000

def rle(data):
    out = []

    if not data:
        return out

    value = data[0]
    count = 1

    for v in data[1:]:
        if v == value and count < 255:
            count += 1
        else:
            out.extend((count, value))
            value = v
            count = 1

    out.extend((count, value))
    return out

def pack_pixels(pixels, bpp, width, height):
    packed = []
    per_byte = 8 // bpp
    mask = (1 << bpp) - 1

    for y in range(height):
        for x in range(0, width, per_byte):
            value = 0

            for i in range(per_byte):
                px = x + i

                if px < width:
                    pixel = pixels[y * width + px] & mask
                    value |= pixel << (8 - bpp * (i + 1))

            packed.append(value)

    return packed

def convert_image(filename, bpp):
    img = Image.open(filename).convert("RGB")
    img = img.resize((W, H))

    pixels = []

    if bpp == 4:
        palette = list(range(16))

        for y in range(H):
            for x in range(W):
                pixels.append(
                    color_index(img.getpixel((x, y)), palette)
                )

    elif bpp == 2:
        for y in range(H):
            for x in range(W):
                pixels.append(
                    color_index(img.getpixel((x, y)), BPP2_PALETTE)
                )

    else:
        for y in range(H):
            for x in range(W):
                pixels.append(
                    1 if brightness(img.getpixel((x, y))) >= 128 else 0
                )

    if bpp < 4:
        data = pack_pixels(pixels, bpp, W, H)
    else:
        data = pixels

    encoded = rle(data)

    output = [
        ord("K"),
        ord("M"),
        ord("G"),
        W & 0xff,
        (W >> 8) & 0xff,
        H & 0xff,
        (H >> 8) & 0xff,
        bpp
    ]

    output.extend(encoded)
    return output

def make_name(filename):
    name = os.path.splitext(os.path.basename(filename))[0]

    name = "".join(
        c if c.isalnum() or c == "_" else "_"
        for c in name
    )

    if name and name[0].isdigit():
        name = "_" + name

    return name

def main():
    outputfile = None
    bpp = 4
    all_images = False

    for arg in sys.argv[1:]:
        if arg == "--all-images-in-pwd":
            all_images = True

        elif arg.startswith("-out="):
            outputfile = arg[5:]

        elif arg.startswith("-bpp="):
            try:
                bpp = int(arg[5:])
            except ValueError:
                print("error: -bpp must be 4, 2, or 1")
                return 1

            if bpp not in (4, 2, 1):
                print("error: -bpp must be 4, 2, or 1")
                return 1

        elif arg.startswith("-"):
            print("error: unknown argument:", arg)
            return 1

    if not all_images:
        print("error: --all-images-in-pwd is required")
        return 1

    if not outputfile:
        print("error: -out= is required")
        return 1

    files = []

    for filename in os.listdir("."):
        if not os.path.isfile(filename):
            continue

        try:
            with Image.open(filename):
                files.append(filename)
        except (OSError, Image.UnidentifiedImageError):
            pass

    files.sort()

    if not files:
        print("error: no images found in current directory")
        return 1

    with open(outputfile, "w") as f:
        f.write("#include <stdint.h>\n\n")

        for filename in files:
            name = make_name(filename)
            data = convert_image(filename, bpp)

            f.write(
                "#define %s_SIZE %d\n"
                % (name.upper(), len(data))
            )

            f.write(
                "const uint8_t %s[%s_SIZE] = {\n"
                % (name, name.upper())
            )

            for i in range(0, len(data), 12):
                chunk = data[i:i + 12]

                f.write(
                    "    " +
                    ", ".join("0x%02X" % b for b in chunk)
                )

                if i + 12 < len(data):
                    f.write(",")

                f.write("\n")

            f.write("};\n\n")

    return 0

if __name__ == "__main__":
    sys.exit(main())
