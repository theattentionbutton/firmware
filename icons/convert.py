from PIL import Image
import os
import sys

# Set the directory path and the output C file
d = os.getcwd()
input_dir = os.path.join(d, "icons")
output_file = sys.argv[1]

if not output_file:
    print("convert: error: no output file supplied", file=sys.stderr)
    exit(1)

icons = dict()
drawables = []
# Open the output C file
for idx, filename in enumerate(os.listdir(input_dir)):
    if not filename.endswith(".png"):
        continue

    cleaned = "".join([c for c in filename])
    if filename.startswith("D_"):
        cleaned = filename.replace("D_", "", 1)
        drawables.append(cleaned)

    img = Image.open(os.path.join(input_dir, filename))
    img = img.convert("1")
    # Resize the image to 8x8 if necessary
    if img.size != (8, 8):
        img = img.resize((8, 8))

    # Convert the image to bytes
    arr = []
    for y in range(8):
        byte = 0
        for x in range(8):
            pixel = img.getpixel((x, y))
            if pixel == 255:  # White pixel
                byte |= 1 << (7 - x)
        arr.append(byte)

    icons[os.path.splitext(cleaned)[0]] = arr

with open(output_file, "w") as f:
    # Write the header
    f.write("#include <pgmspace.h>\n\n")
    f.write("#ifndef __ICON_H\n")
    f.write("#define __ICON_H\n\n")

    f.write("typedef enum icon_id_t {\n")
    for idx, key in enumerate(icons.keys()):
        f.write(f"    {key}")

        if idx == 0:
            f.write(" = 1")

        if not idx == len(icons) - 1:
            f.write(",\n")
        else:
            f.write("\n")

    f.write("} IconId;\n\n")

    f.write("const int DRAWABLE_ICONS[] PROGMEM = {\n")
    for i, icon in enumerate(drawables):
        f.write(f"    {os.path.splitext(icon)[0]}")
        if not idx == len(drawables) - 1:
            f.write(",\n")
        else:
            f.write("\n")
    f.write("};\n\n")

    f.write(
        "#define DRAWABLE_LENGTH"
        " (sizeof(DRAWABLE_ICONS)/sizeof(DRAWABLE_ICONS[0]))\n\n"
    )

    f.write(f"const byte ICONS[][{len(icons)}] PROGMEM = {{\n")
    for idx, (key, value) in enumerate(icons.items()):
        f.write("    " + str(value).replace("[", "{").replace("]", "}"))
        if not idx == len(icons) - 1:
            f.write(",")
        f.write(f" // {key}\n")
    f.write("};\n\n")

    f.write("#define ICONS_LENGTH (sizeof(ICONS)/sizeof(ICONS[0]))\n")

    f.write("#define ICON(id) ICONS[id - 1]\n")
    f.write("\n#endif\n")
