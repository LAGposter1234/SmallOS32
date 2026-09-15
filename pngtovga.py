#!/usr/bin/env python3

from PIL import Image
import sys
import random

W = 320
H = 240

palette = [
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
    (255, 255, 255),
]

def color_index(rgb):
    return min(
        range(16),
        key=lambda i: sum((rgb[j] - palette[i][j]) ** 2 for j in range(3))
    )

def corrupt(img):
    pixels = img.load()

    for _ in range(20):
        x = random.randrange(W)
        y = random.randrange(H)
        r, g, b = pixels[x, y]
        channel = random.randrange(3)
        delta = random.choice([-1, 1])

        if channel == 0:
            r = max(0, min(255, r + delta))
        elif channel == 1:
            g = max(0, min(255, g + delta))
        else:
            b = max(0, min(255, b + delta))

        pixels[x, y] = (r, g, b)

def convert(filename):
    img = Image.open(filename).convert("RGB")
    img = img.resize((W, H), Image.Resampling.NEAREST)

    corrupt(img)

    pixels = []

    for y in range(H):
        for x in range(W):
            pixels.append(color_index(img.getpixel((x, y))))

    data = bytearray()
    current = pixels[0]
    count = 0

    for color in pixels:
        if color == current and count < 255:
            count += 1
        else:
            data.append(count)
            data.append(current)
            current = color
            count = 1

    data.append(count)
    data.append(current)

    return data

def main():
    if len(sys.argv) != 4:
        print(f"usage: {sys.argv[0]} input.png output.h array_name")
        return 1

    input_file = sys.argv[1]
    output_file = sys.argv[2]
    array_name = sys.argv[3]

    data = convert(input_file)
    size_name = f"{array_name.upper()}_SIZE"

    with open(output_file, "w") as f:
        f.write("#include <stdint.h>\n\n")
        f.write(f"#define {size_name} {len(data)}\n\n")
        f.write(f"const uint8_t {array_name}[{size_name}] = {{\n")

        for i in range(0, len(data), 16):
            f.write("    ")
            f.write(", ".join(f"0x{b:02X}" for b in data[i:i + 16]))
            f.write(",\n")

        f.write("};\n")

    print(f"RLE size: {len(data)} bytes")

if __name__ == "__main__":
    main()
