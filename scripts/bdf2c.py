import sys

def convert_bdf_to_c_256(bdf_path, header_path):
    with open(bdf_path, 'r', encoding='utf-8', errors='ignore') as f:
        lines = f.readlines()

    glyphs = {}
    current_encoding = None
    in_bitmap = False
    current_bitmap = []

    for line in lines:
        line = line.strip()
        
        if line.startswith("ENCODING"):
            parts = line.split()
            if len(parts) > 1:
                current_encoding = int(parts[1])
        elif line.startswith("BITMAP"):
            in_bitmap = True
            current_bitmap = []
        elif line.startswith("ENDCHAR"):
            in_bitmap = False
            if current_encoding is not None and 0 <= current_encoding < 256:
                formatted_rows = []
                for row in current_bitmap:
                    # Take top byte if 16-bit hex strings are emitted by BDF
                    row_hex = row[:2] if len(row) >= 2 else row.zfill(2)
                    formatted_rows.append(row_hex)

                while len(formatted_rows) < 16:
                    formatted_rows.append("00")

                glyphs[current_encoding] = formatted_rows[:16]
            
            current_encoding = None
            
        elif in_bitmap:
            current_bitmap.append(line)

    # Generate full 256-entry array header
    with open(header_path, 'w') as f:
        f.write("#ifndef FONT_H\n#define FONT_H\n\n")
        f.write("#include <stdint.h>\n\n")
        f.write("#define FONT_WIDTH 8\n#define FONT_HEIGHT 16\n\n")
        f.write("/* Full 256-glyph 8x16 Font Bitmap */\n")
        f.write("const uint8_t font_8x16[256][16] = {\n")
        
        for ascii_val in range(256):
            if ascii_val in glyphs:
                hex_rows = [f"0x{row}" for row in glyphs[ascii_val]]
            else:
                hex_rows = ["0x00"] * 16
                
            char_repr = chr(ascii_val) if 32 <= ascii_val <= 126 else f"0x{ascii_val:02X}"
            f.write(f"    {{ {', '.join(hex_rows)} }}, // '{char_repr}'\n")
            
        f.write("};\n\n")
        f.write("#endif // FONT_H\n")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python bdf2c.py input.bdf font.h")
        sys.exit(1)
    convert_bdf_to_c_256(sys.argv[1], sys.argv[2])