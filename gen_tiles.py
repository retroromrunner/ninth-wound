#!/usr/bin/env python3
"""Generate src/tiles.h for AREA QUEST.

Tile layout (all BG tiles, loaded once; per-area palettes switch the look):
  0 T_SKY, 1 T_STAR,
  2-7 floor tiles: dungeon dirt x2, meadow grass x2, crypt stone x2,
  then 7 wall types x 17 tiles each (full + TOP r + BOT r, r=0..7):
    0 slate, 1 cinder, 2 tree, 3 rock, 4 mossy-A, 5 mossy-B, 6 gate door.
  Then font glyphs, then key billboard tiles (small 2x4, large 4x8).

Wall edge tiles: TOP r takes pattern rows r..7 (ceiling above),
BOT r takes pattern rows 0..r then floor colour rows, so brick
courses / foliage run continuously down a wall face.

Palette values in wall patterns: 1 = dark, 2 = light, 3 = floor
colour (used in BOT tiles and, for rock, as the light-rock value --
see the meadow wall palette, where colour 3 is light grey so rock
pattern values 1/3 read as dark/light grey while tree pattern
values 1/2 read as dark/light green under the same palette).
"""
import os

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "src", "tiles.h")

_seven = open(os.path.expanduser("~/workspace/sevenfold/gen_tiles.py")).read()
_ns = {}
exec(_seven[_seven.index("FONT_ORDER"):_seven.index("# ---------------------------------------------------------------- portraits")], _ns)
FONT_ORDER, FONT = _ns["FONT_ORDER"], _ns["FONT"]


def tile_bytes4(rows, name):
    assert len(rows) == 8, f"{name}: need 8 rows"
    out = []
    for r in rows:
        assert len(r) == 8, f"{name}: row {r!r} is not 8 chars"
        lo = hi = 0
        for ch in r:
            v = int(ch)
            lo = (lo << 1) | (v & 1)
            hi = (hi << 1) | ((v >> 1) & 1)
        out.append(lo)
        out.append(hi)
    return out


def tile_bytes_font(rows, name):
    return tile_bytes4(["".join("3" if ch == "#" else "0" for ch in r) for r in rows], name)


SLATE = [
    "22212222", "21212222", "22212222", "11111111",
    "22222212", "21222212", "22222212", "11111111",
]
CINDER = [
    "11110211", "12110111", "11110121", "11110211",
    "12110111", "11110121", "11110211", "00000000",
]
TREE = [
    "12122121", "21211212", "12212112", "21121221",
    "12112212", "21221121", "12211212", "21122121",
]
ROCK = [
    "11113111", "11311131", "11131111", "13111113",
    "11113111", "31111311", "11131131", "11311111",
]
MOSSY_A = [
    "22212232", "21213222", "22212222", "11131111",
    "22223212", "21222212", "22212232", "11111111",
]
MOSSY_B = [
    "22112231", "21122122", "22112222", "13111111",
    "22221132", "21122211", "22112222", "11111111",
]
GATE = [
    "21212121", "21212121", "21212121", "11122111",
    "21212121", "21212121", "21212121", "11111111",
]
WALL_PATTERNS = [SLATE, CINDER, TREE, ROCK, MOSSY_A, MOSSY_B, GATE]

SCENE = []
SCENE.append(("T_SKY", ["00000000"] * 8))
SCENE.append(("T_STAR", ["00000000", "00100000", "00000000", "00000010",
                          "00000000", "00000000", "01000000", "00000000"]))
SCENE.append(("T_FLOOR_DUNGEON", ["12131211", "11121131", "21131111", "11112111",
                                  "13121111", "11131121", "21111131", "11121111"]))
SCENE.append(("T_FLOOR_DUNGEON2", ["11131111", "12111121", "11121131", "31111111",
                                   "11112111", "21131111", "11111121", "13121111"]))
SCENE.append(("T_FLOOR_GRASS", ["12121211", "21112121", "12131121", "21121211",
                                "12112131", "21121211", "12121211", "31112121"]))
SCENE.append(("T_FLOOR_GRASS2", ["21121211", "12112121", "21131121", "12121211",
                                 "21112131", "12121211", "21121211", "12131121"]))
SCENE.append(("T_FLOOR_CRYPT", ["11121111", "12111121", "11112111", "21111121",
                                "11121111", "12111111", "11112121", "11121111"]))
SCENE.append(("T_FLOOR_CRYPT2", ["12111111", "11121121", "11111111", "21112111",
                                 "11121111", "11111121", "12121111", "11111111"]))
for v, pat in enumerate(WALL_PATTERNS):
    SCENE.append((f"T_WALL{v}", pat))
    for r in range(8):
        SCENE.append((f"T_WALL{v}_TOP{r}", ["00000000"] * r + pat[r:]))
        SCENE.append((f"T_WALL{v}_BOT{r}", pat[:r + 1] + ["33333333"] * (7 - r)))


def emit_array(fh, name, data):
    fh.write(f"static const unsigned char {name}[] = {{\n")
    for i in range(0, len(data), 16):
        fh.write("  " + ",".join(f"0x{b:02X}" for b in data[i:i + 16]) + ",\n")
    fh.write("};\n")


def key_master():
    rows = []
    for y in range(32):
        row = ["0"] * 16
        if 2 <= y <= 10:
            for x in range(3, 13):
                if x in (3, 4, 11, 12) or y in (2, 3, 9, 10):
                    row[x] = "1"
                elif x in (5, 6) and y in (4, 5):
                    row[x] = "3"
        if 11 <= y <= 24:
            for x in (7, 8):
                row[x] = "1"
            if y in (20, 21):
                row[10] = "1"; row[11] = "1"
            if y in (23, 24):
                row[10] = "1"
        if 25 <= y <= 31:
            for x in range(2, 14):
                row[x] = "2" if y % 2 == 0 else "1"
        rows.append("".join(row))
    return rows


def split_tiles(art):
    h = len(art); w = len(art[0])
    assert h % 8 == 0 and w % 8 == 0
    out = []; solid = []
    for ty in range(h // 8):
        for tx in range(w // 8):
            sub = [row[tx * 8:(tx + 1) * 8] for row in art[ty * 8:(ty + 1) * 8]]
            solid.append(1 if any(ch != "0" for r in sub for ch in r) else 0)
            out += tile_bytes4(sub, f"key {tx},{ty}")
    return out, solid


def scale_art(art, w, h):
    """Nearest-neighbour downscale of the master key art to w x h pixels."""
    sw, sh = len(art[0]), len(art)
    return ["".join(art[y * sh // h][x * sw // w] for x in range(w))
            for y in range(h)]


# v2 key tile sets, one per distance, so a screen tile NEVER repeats
# a texture tile: the C code places keys at tile coordinates with a
# 1:1 screen-tile -> texture-tile mapping (see draw_key_billboard).
# Sizes (tile-aligned, floor-anchored in tile terms):
#   fwd 1: 16x32 (2 cols x 4 rows), fwd 2: 8x24 (1 x 3),
#   fwd 3: 8x16 (1 x 2), fwd 4: 8x8 (1 x 1).
# This replaces the v1 scheme (small 2x4 sampled at 2+ tiles, large
# 4x8 at 1 tile), whose pixel-centre sampler repeated tiles whenever
# the on-screen tile span exceeded the texture tile count — at
# 2 tiles the key spanned 5 screen rows against a 4-row texture,
# so the key shape duplicated into overlapping clips.
KEY_SIZES = [(16, 32), (8, 24), (8, 16), (8, 8)]


def key_tiles():
    out = []
    solids = []
    for w, h in KEY_SIZES:
        tiles, solid = split_tiles(scale_art(key_master(), w, h))
        out.append(tiles)
        solids.append(solid)
    return out, solids


# v4 Meadow NPC (Area 1 only, tile (2,7)). One standing villager,
# master 32x64, values: 0 background/dark outline, 1 blue tunic,
# 2 skin, 3 brown hair/trousers. Downscaled per distance exactly
# like the v2 keys, so a screen tile never repeats a texture tile:
#   fwd 1: 32x64 (4 cols x 8 rows), fwd 2: 16x32 (2 x 4),
#   fwd 3: 16x24 (2 x 3), fwd 4: 8x16 (1 x 2).
# Heights are about half wall height at 1 tile (64 vs 128) and
# shrink with distance; the C code floor-anchors the figure like
# a key, so its feet sit on the floor line at its distance.
def npc_master():
    rows = []
    cx = 16
    for y in range(64):
        row = ["0"] * 32
        if 6 <= y <= 17:
            hw = 5 if y <= 9 else 6
            for x in range(cx - hw, cx + hw + 1):
                if y <= 8:
                    row[x] = "3"  # hair
                elif y == 12 and x in (13, 18):
                    row[x] = "0"  # eyes (dark dots in the face)
                else:
                    row[x] = "2"  # face
        elif 18 <= y <= 40:
            hw = 9 if y <= 22 else 8
            for x in range(cx - hw, cx + hw + 1):
                row[x] = "3" if y == 33 else "1"  # belt row brown, else tunic
        elif 41 <= y <= 63:
            hw = 6
            for x in range(cx - hw, cx + hw + 1):
                if 45 <= y <= 56 and x in (15, 16):
                    row[x] = "0"  # gap between the legs
                else:
                    row[x] = "3"  # trousers / boots
        rows.append("".join(row))
    return rows


NPC_SIZES = [(32, 64), (16, 32), (16, 24), (8, 16)]


def npc_tiles():
    out = []
    solids = []
    for w, h in NPC_SIZES:
        tiles, solid = split_tiles(scale_art(npc_master(), w, h))
        out.append(tiles)
        solids.append(solid)
    return out, solids



# ---- v3 distinct billboard art (VRAM bank 1) ----
# Every pickup / interactable kind gets its OWN master art, so nothing
# renders as a generic key any more. Masters are 16x32 pixel grids
# (values: 0 transparent, 1 dark, 2 mid, 3 bright, under palette 6).
# Each master is downscaled per distance exactly like the key sets.
def _grid():
    return [["0"] * 16 for _ in range(32)]

def _disc(g, cx, cy, r, v):
    for y in range(32):
        for x in range(16):
            if (x - cx) ** 2 + (y - cy) ** 2 <= r * r:
                g[y][x] = v

def _rect(g, x0, y0, x1, y1, v):
    for y in range(max(0, y0), min(32, y1 + 1)):
        for x in range(max(0, x0), min(16, x1 + 1)):
            g[y][x] = v

def _rows(g):
    return ["".join(r) for r in g]

def _cog():
    g = _grid()
    for a in range(8):  # 8 teeth around the wheel
        import math
        tx = round(8 + 7.5 * math.cos(a * math.pi / 4))
        ty = round(13 + 7.5 * math.sin(a * math.pi / 4))
        _rect(g, tx - 1, ty - 1, tx + 1, ty + 1, "2")
    _disc(g, 8, 13, 6, "2")
    _disc(g, 8, 13, 2, "3")
    g[13][8] = "0"
    return _rows(g)

def _pin():
    g = _grid()
    _disc(g, 8, 4, 3, "3")
    _rect(g, 7, 6, 8, 27, "2")
    _rect(g, 6, 28, 9, 28, "2")
    _rect(g, 7, 29, 8, 30, "2")
    g[31][7] = "2"
    return _rows(g)

def _seal():
    g = _grid()
    _disc(g, 8, 14, 7, "2")
    _disc(g, 8, 14, 4, "3")
    g[14][8] = "1"
    _rect(g, 6, 14, 10, 14, "1")
    return _rows(g)

def _grave():
    g = _grid()
    _disc(g, 8, 14, 7, "2")
    g[12][6] = "0"; g[12][10] = "0"
    _rect(g, 5, 17, 11, 17, "1")
    _rect(g, 6, 19, 10, 19, "1")
    return _rows(g)

def _sigil():
    g = _grid()
    for y in range(32):
        for x in range(16):
            d = abs(x - 8) + abs(y - 14)
            if d <= 7:
                g[y][x] = "2"
            if d <= 3:
                g[y][x] = "3"
    return _rows(g)

def _lantern():
    g = _grid()
    _rect(g, 5, 3, 10, 5, "1")
    _rect(g, 5, 26, 10, 28, "1")
    _rect(g, 5, 6, 5, 25, "1")
    _rect(g, 10, 6, 10, 25, "1")
    _disc(g, 8, 16, 3, "3")
    g[12][8] = "3"; g[11][8] = "3"
    return _rows(g)

def _lever():
    g = _grid()
    _rect(g, 3, 25, 12, 29, "1")
    for i in range(18):
        x = 6 + i // 2
        y = 24 - i
        _rect(g, x, y, x + 1, y + 1, "2")
    _disc(g, 14, 5, 2, "3")
    return _rows(g)

def _bell():
    g = _grid()
    _disc(g, 8, 13, 6, "2")
    _rect(g, 3, 18, 12, 20, "1")
    g[23][8] = "3"
    _rect(g, 0, 19, 15, 19, "0")
    return _rows(g)

def _note():
    g = _grid()
    _rect(g, 3, 5, 12, 27, "2")
    _rect(g, 3, 5, 12, 5, "1")
    _rect(g, 3, 27, 12, 27, "1")
    _rect(g, 3, 6, 3, 26, "1")
    _rect(g, 12, 6, 12, 26, "1")
    for y in (11, 15, 19, 23):
        _rect(g, 5, y, 10, y, "1")
    return _rows(g)

def _epitaph():
    g = _grid()
    _disc(g, 8, 10, 5, "2")
    _rect(g, 4, 10, 11, 28, "2")
    _rect(g, 4, 10, 11, 10, "1")
    _rect(g, 4, 28, 11, 28, "1")
    _rect(g, 7, 13, 7, 19, "3")
    _rect(g, 5, 15, 9, 15, "3")
    return _rows(g)

def _mech():
    g = _grid()
    _rect(g, 3, 8, 12, 24, "1")
    _disc(g, 8, 16, 5, "2")
    _disc(g, 8, 16, 2, "3")
    return _rows(g)

def _tower():
    g = _grid()
    _rect(g, 6, 10, 9, 28, "1")
    _rect(g, 5, 3, 10, 9, "2")
    _disc(g, 8, 6, 2, "3")
    return _rows(g)

def _coffin():
    # v5: upright sarcophagus — a tapered BOX, not a person.
    # No head circle, no arms, no legs: wide shoulders at the top
    # narrowing to the feet, a lid seam down the middle, a head
    # plate disc, and a horizontal lid band. Clearly distinct
    # from the NPC tall figure (head + tunic + legs).
    g = _grid()
    for y in range(2, 28):
        if y <= 10:
            x0, x1 = 4, 11
        elif y <= 20:
            x0, x1 = 5, 10
        else:
            x0, x1 = 6, 9
        for x in range(x0, x1 + 1):
            g[y][x] = "2"
        g[y][x0] = "1"
        g[y][x1] = "1"
    for y in range(5, 25):
        g[y][8] = "1"  # lid seam
    _rect(g, 4, 10, 11, 10, "1")  # lid band
    _disc(g, 8, 6, 2, "3")  # head plate
    return _rows(g)

EXTRA_MASTERS = [
    ("COG", _cog()), ("PIN", _pin()), ("SEAL", _seal()),
    ("GRAVE", _grave()), ("SIGIL", _sigil()), ("LANTERN", _lantern()),
    ("LEVER", _lever()), ("BELL", _bell()), ("NOTE", _note()),
    ("EPITAPH", _epitaph()), ("MECH", _mech()), ("TOWER", _tower()),
    ("COFFIN", _coffin()),
]
ARROW_TILES = {
    "N": ["00033000", "00333300", "03333330", "00033000",
          "00033000", "00033000", "00033000", "00000000"],
    "E": ["00000000", "00003000", "00003300", "00333333",
          "00003300", "00003000", "00000000", "00000000"],
    "S": ["00000000", "00033000", "00033000", "00033000",
          "00033000", "03333330", "00333300", "00033000"],
    "W": ["00000000", "00030000", "00330000", "33333300",
          "00330000", "00030000", "00000000", "00000000"],
}

def extra_tiles():
    """Returns (tile_bytes, [(name, [solid per distance])], base_map)."""
    data = []
    solids = []
    for a in ("N", "E", "S", "W"):
        data += tile_bytes4(ARROW_TILES[a], f"arrow {a}")
    base = {}
    idx = 4
    for name, master in EXTRA_MASTERS:
        base[name] = idx
        ss = []
        for w, h in KEY_SIZES:
            tiles, solid = split_tiles(scale_art(master, w, h))
            data += tiles
            ss.append(solid)
            idx += len(tiles) // 16
        solids.append((name, ss))
    assert idx <= 256, f"bank-1 tiles {idx} exceed 256"
    return data, solids, base

def main():
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    data = []
    with open(OUT, "w") as fh:
        fh.write("/* Generated by gen_tiles.py - do not edit by hand. */\n")
        for i, (name, rows) in enumerate(SCENE):
            fh.write(f"#define {name} {i}\n")
        fh.write(f"#define SCENE_COUNT {len(SCENE)}\n")
        fh.write("#define FLOOR_BASE(area) (2 + ((area) % 3) * 2)\n")
        fh.write("#define WALL_BASE(v) (8 + (v) * 17)\n")
        fh.write("#define WALL_FULL(v) (WALL_BASE(v))\n")
        fh.write("#define WALL_TOP(v, r) (WALL_BASE(v) + 1 + (r) * 2)\n")
        fh.write("#define WALL_BOT(v, r) (WALL_BASE(v) + 2 + (r) * 2)\n")
        for i, (name, rows) in enumerate(SCENE):
            data += tile_bytes4(rows, name)
        fh.write(f"#define FONT_BASE SCENE_COUNT\n")
        for ch in FONT_ORDER:
            assert ch in FONT, f"missing glyph {ch!r}"
            data += tile_bytes_font(FONT[ch], f"font {ch!r}")
        fh.write(f"/* FONT_ORDER = {''.join(FONT_ORDER)} */\n")
        sets, solids = key_tiles()
        counts = [len(s) // 16 for s in sets]  # tiles per set
        assert counts == [8, 3, 2, 1], counts
        nsets, nsolids = npc_tiles()
        ncounts = [len(s) // 16 for s in nsets]
        assert ncounts == [32, 8, 6, 2], ncounts
        total = len(SCENE) + len(FONT_ORDER) + sum(counts) + sum(ncounts)
        assert total <= 256, f"tile count {total} exceeds BG tile space"
        base = len(SCENE) + len(FONT_ORDER)
        for i, ((w, h), n) in enumerate(zip(KEY_SIZES, counts)):
            if i:
                base += counts[i - 1]
            fh.write(f"#define KEY_D{i + 1}_BASE (SCENE_COUNT + {len(FONT_ORDER)} + {base - (len(SCENE) + len(FONT_ORDER))})\n")
            fh.write(f"#define KEY_D{i + 1}_COLS {w // 8}\n#define KEY_D{i + 1}_ROWS {h // 8}\n")
        nbase = base + counts[-1]
        for i, ((w, h), n) in enumerate(zip(NPC_SIZES, ncounts)):
            if i:
                nbase += ncounts[i - 1]
            fh.write(f"#define NPC_D{i + 1}_BASE (SCENE_COUNT + {len(FONT_ORDER)} + {nbase - (len(SCENE) + len(FONT_ORDER))})\n")
            fh.write(f"#define NPC_D{i + 1}_COLS {w // 8}\n#define NPC_D{i + 1}_ROWS {h // 8}\n")
        fh.write(f"#define TILE_COUNT (SCENE_COUNT + {len(FONT_ORDER)} + {sum(counts) + sum(ncounts)})\n")
        for s in sets:
            data += s
        for s in nsets:
            data += s
        emit_array(fh, "tiles", data)
        for i, solid in enumerate(solids):
            fh.write(f"static const unsigned char key_solid_d{i + 1}[] = {{\n  "
                     + ",".join(str(v) for v in solid) + ",\n};\n")
        for i, solid in enumerate(nsolids):
            fh.write(f"static const unsigned char npc_solid_d{i + 1}[] = {{\n  "
                     + ",".join(str(v) for v in solid) + ",\n};\n")

        # ---- v3 bank-1 extra tiles ----
        edata, esolids, ebase = extra_tiles()
        for i, a in enumerate(("N", "E", "S", "W")):
            fh.write(f"#define ARROW_{a} {i}\n")
        for name, _ss in esolids:
            for di, (w, h) in enumerate(KEY_SIZES):
                fh.write(f"#define {name}_D{di + 1}_BASE {ebase[name] + sum(len(split_tiles(scale_art(dict(EXTRA_MASTERS)[name], ww, hh))[0]) // 16 for ww, hh in KEY_SIZES[:di])}\n")
                fh.write(f"#define {name}_D{di + 1}_COLS {w // 8}\n#define {name}_D{di + 1}_ROWS {h // 8}\n")
        fh.write(f"#define EXTRA_TILE_COUNT {len(edata) // 16}\n")
        data += edata
        for name, ss in esolids:
            for di, solid in enumerate(ss):
                fh.write(f"static const unsigned char {name.lower()}_solid_d{di + 1}[] = {{\n  "
                         + ",".join(str(v) for v in solid) + ",\n};\n")
        emit_array(fh, "extra_tiles", edata)
    print(f"wrote {OUT}: {len(SCENE)} scene tiles + {len(FONT_ORDER)} font glyphs "
          f"+ key sets {counts} + npc sets {ncounts} = {total} tiles")


if __name__ == "__main__":
    main()
