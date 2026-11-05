/* THE NINTH WOUND v8 - Areas 1-12 (complete).
 * v2 adds title music (title screen only, stopped on entering the game)
 * and event SFX; in-game there is no background music.
 * Built on Area Quest v5 (instant tile-step raycaster, facing-preserving
 * gates, floor-anchored billboards, bottom-half dialog, Start inventory,
 * Select minimap with fog of war). No combat, no leveling, no smooth
 * movement. Area 1 The Gatehouse: 3 component pickups (Cog/Pin/Seal),
 * 3 statues rotated to face east (no levers in Area 1 in v4),
 * gate mechanism grants Bronze Sigil, which opens the gate to Area 2.
 * Area 2 Ash Village: 4 NPCs (bell order west-east-north), 3 bells,
 * lantern tower grants Ash Lantern, which opens the gate to Area 3.
 * Area 3 The Burial Maze: 3 Grave Seals, 2 coffins rotated to face
 * south (the pulse), epitaph readable only with the Ash Lantern;
 * all three together open the crypt and grant the Ancient Key
 * Fragment, ending the slice. Proof block in WRAM: 'N','W',state,
 * area,ptx,pty,ang,items_lo,items_hi,flags,checksum. */
#include <gb/gb.h>
#include <gb/cgb.h>
#include <stdint.h>
#include "tiles.h"

static const uint8_t AREA_W[12] = {16, 12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20};
static const uint8_t AREA_H[12] = {16, 12, 12, 12, 12, 12, 12, 20, 20, 20, 20, 20};
/* Cell codes: '0' open, '1' stone/mossy wall, 'T' tree, 'R' rock,
 * 'G' gate to next area, 'B' return gate to previous area
 * (always open, v3), '3' final exit. */
static const char *MAPS[3][20] = {
    {   /* Area 0 v3: 16x16 Gatehouse. Spawn (1,1); spawn note (2,1);
         * Cog (13,2), Pin (3,13), Seal (12,13); statues (6,5)(10,5)
         * (8,7); statue note (7,4); levers west (2,10) middle (8,11)
         * east (14,3); lever note (13,10); mechanism (13,8);
         * gate G (15,8), alcove arrivals (15,7) (15,9) (14,8). */
        "1111111111111111",
        "1000001000000001",
        "1011101011111001",
        "1010001010000101",
        "1000101000100101",
        "1110100010000101",
        "1000101000100001",
        "1011101100111100",
        "101000000000000G",
        "1011101110111100",
        "1000001000000001",
        "1111101001111101",
        "1000101000100001",
        "1010101010100101",
        "1000001000000101",
        "1111111111111111",
    },
    {   /* Area 2: Mossy Crypt, final exit (10,10), no key.
         * v3: (1,1) is the return gate 'B' (gold, always open);
         * entry is (2,1) facing east after the Meadow gate.
         * v4: (1,0) and (1,2) opened so direction-preserving
         * gate travel has an open arrival cell for every
         * reachable approach: (1,2) is the arrival for a
         * southward Meadow-gate crossing and the south
         * approach to the Crypt return gate; (1,0) is the
         * arrival for a northward Meadow-gate crossing
         * (a border alcove: north of it is out of bounds,
         * so forward from there bumps and Down returns). */
        "101111111111",
        "1B0000000111",
        "101111010001",
        "100000010101",
        "101110110101",
        "100010000101",
        "111010111101",
        "100010100001",
        "101110101111",
        "100000101001",
        "111011000031",
        "111111111111",
    },
    {   /* Area 6: Flesh Market. Return B (1,1); merchants (3,5)
         * (9,5) (5,8); relic keeper (6,9); v7 Blood Amber pickup
         * (10,10), southeast back corner. v11: a small maze in
         * the southeast quadrant guards the Amber — walls at
         * (8,7)(9,7), (7,8)(10,8), (8,10) force the player
         * around the south edge ((7,10)->(7,9)->(8,9)->(9,9)
         * ->(9,10)->(10,10)) past dead-end pockets at (7,7),
         * (10,7) and (8,8)(9,8); the gate (10,9) stays
         * reachable from (9,9). */
        "111111111111",
        "1B0000000001",
        "100000100001",
        "100000000001",
        "100100000001",
        "100000000001",
        "100000100001",
        "100000001101",
        "100100010011",
        "1000000000G1",
        "100000001001",
        "111111111111",
    },
};
/* Items (bitmask g_items): 0 Cog, 1 Pin, 2 Gate Seal, 3 Bronze Sigil,
 * 4 Ash Lantern, 5-7 Grave Seals 1-3, 8 Ancient Key Fragment,
 * 9 Blood Sigil, 10 Bone Tuning Fork, 11 River Seal, 12 Eye Lens,
 * 13 Blood Amber, 14 Pale Idol (v7 Flesh Market trade chain).
 * v8: 15 Lung Bellows, 16 Silver Sigil, 17 Heart Seal, 18 Heart Key,
 * 19 Ninth Key, 20 Vein Organ (Forest backtrack), 21 Gold Ingot
 * (Bone Labyrinth treasure), 22 Market Key (Market backtrack),
 * 23 Binding Thread (Cathedral backtrack), 24-27 Bone Seals 1-4. */
#define IT_COG 0
#define IT_PIN 1
#define IT_SEAL 2
#define IT_SIGIL 3
#define IT_LANTERN 4
#define IT_GRAVE1 5
#define IT_GRAVE2 6
#define IT_GRAVE3 7
#define IT_FRAGMENT 8
#define IT_BLOOD 9
#define IT_FORK 10
#define IT_RIVER 11
#define IT_LENS 12
#define IT_AMBER 13
#define IT_IDOL 14
#define IT_LUNG 15
#define IT_SILVER 16
#define IT_HEARTSEAL 17
#define IT_HEARTKEY 18
#define IT_NINTH 19
#define IT_ORGAN 20
#define IT_GOLD 21
#define IT_MARKETKEY 22
#define IT_THREAD 23
#define IT_BONE1 24
#define IT_BONE2 25
#define IT_BONE3 26
#define IT_BONE4 27
static const char *ITEM_NAMES[28] = {"RUSTED COG", "IRON PIN", "GATE SEAL",
    "BRONZE SIGIL", "ASH LANTERN", "GRAVE SEAL 1", "GRAVE SEAL 2",
    "GRAVE SEAL 3", "ANCIENT KEY FRAG", "BLOOD SIGIL", "BONE FORK",
    "RIVER SEAL", "EYE LENS", "BLOOD AMBER", "PALE IDOL",
    "LUNG", "SILVER", "HEART SEAL", "HEART KEY",
    "NINTH KEY", "VEIN", "GOLD", "MKT KEY",
    "BINDING", "BONE 1", "BONE 2", "BONE 3",
    "BONE 4"};
/* Entity kinds. SMALL = gold key tileset, TALL = NPC tileset (statues,
 * coffins, mechanisms, villagers all read as tall figures/pillars). */
#define ENT_PICKUP 0
#define ENT_STATUE 1
#define ENT_LEVER 2
#define ENT_NOTE 3
#define ENT_MECH 4
#define ENT_NPC 5
#define ENT_BELL 6
#define ENT_TOWER 7
#define ENT_COFFIN 8
#define ENT_EPITAPH 9
#define ENT_COLUMN 10
#define ENT_RVALVE 11
#define ENT_WIND 12
#define ENT_SPINE 13
#define ENT_RUNE 14
#define ENT_ORGAN 15
#define ENT_THRONE 16
#define ENT_CHAMBER 17
typedef struct { uint8_t x, y, kind, aux; } Ent;
/* Area 0 Gatehouse: pickups Cog(5,3) Pin(8,9) Seal(3,9); statues
 * (4,3)(6,7)(8,7) -> east; levers west(2,6) middle(7,9) east(9,3),
 * order middle-west-east; notes (3,3) statue hint (9,7) lever hint;
 * mechanism (9,9). */
static const Ent ENTS0[9] = {
    {13, 2, ENT_PICKUP, IT_COG}, {3, 13, ENT_PICKUP, IT_PIN},
    {12, 13, ENT_PICKUP, IT_SEAL}, {6, 5, ENT_STATUE, 0},
    {10, 5, ENT_STATUE, 1}, {8, 7, ENT_STATUE, 2},
    {5, 7, ENT_NOTE, 24}, {7, 4, ENT_NOTE, 4},
    {13, 8, ENT_MECH, 0},
};
/* Area 1 Ash Village: NPCs Elder(2,7) Merchant(5,2) Child(8,5)
 * Monk(4,8); bells west(3,5) east(9,5) north(6,2), order west-east-
 * north; lantern tower (6,9). */
static const Ent ENTS1[8] = {
    {2, 7, ENT_NPC, 0}, {5, 2, ENT_NPC, 1}, {8, 5, ENT_NPC, 2},
    {4, 8, ENT_NPC, 3}, {3, 5, ENT_BELL, 0}, {9, 5, ENT_BELL, 1},
    {6, 2, ENT_BELL, 2}, {6, 9, ENT_TOWER, 0},
};
/* Area 2 Burial Maze: Grave Seals (3,3)(7,5)(9,7); coffins
 * (5,1)(3,9) -> south; epitaph (8,3); v5 pulse-hint note (5,5),
 * central corridor, reachable before either coffin or the crypt,
 * so the "face away from the pulse" message is never a dead end. */
static const Ent ENTS2[7] = {
    {3, 3, ENT_PICKUP, IT_GRAVE1}, {7, 5, ENT_PICKUP, IT_GRAVE2},
    {9, 7, ENT_PICKUP, IT_GRAVE3}, {5, 1, ENT_COFFIN, 0},
    {3, 9, ENT_COFFIN, 1}, {8, 3, ENT_EPITAPH, 0},
    {5, 5, ENT_NOTE, 38},
};
/* Area 3 Forest: valves west aux0 / north aux1 / east aux2 (order
 * north, west, east = LEVER_ORDER), note, NPC, barrier mechanism.
 * Area 4 Cathedral: bone columns west aux0 / east aux1 / north
 * aux2 (order north, west, east), note, NPC, mechanism.
 * Area 5 River: river valves west aux0 / north aux1 / east aux2
 * (order west, east, north), note, NPC, dock mechanism.
 * Area 6 Market: merchants are NPCs with dialog indices 69-71,
 * keeper is a mechanism aux4; v7 trade chain: Blood Amber pickup
 * (10,10), east merchant (9,5) trades Amber for Pale Idol. */
static const Ent ENTS3[7] = {
    {3, 5, ENT_LEVER, 0}, {6, 2, ENT_LEVER, 1}, {9, 5, ENT_LEVER, 2},
    {5, 5, ENT_NOTE, 39}, {4, 8, ENT_NPC, 40}, {6, 9, ENT_MECH, 1},
    {9, 2, ENT_CHAMBER, 0},
};
static const Ent ENTS4[7] = {
    {3, 5, ENT_COLUMN, 0}, {9, 5, ENT_COLUMN, 1}, {6, 2, ENT_COLUMN, 2},
    {5, 5, ENT_NOTE, 44}, {4, 8, ENT_NPC, 45}, {6, 9, ENT_MECH, 2},
    {9, 2, ENT_CHAMBER, 1},
};
static const Ent ENTS5[6] = {
    {3, 5, ENT_RVALVE, 0}, {6, 2, ENT_RVALVE, 1}, {9, 5, ENT_RVALVE, 2},
    {5, 5, ENT_NOTE, 52}, {4, 8, ENT_NPC, 53}, {6, 9, ENT_MECH, 3},
};
static const Ent ENTS6[6] = {
    {3, 5, ENT_NPC, 60}, {9, 5, ENT_NPC, 61}, {5, 8, ENT_NPC, 62},
    {6, 9, ENT_MECH, 4}, {10, 10, ENT_PICKUP, IT_AMBER},
    {2, 8, ENT_NPC, 74},
};
static const Ent ENTS7[6] = {
    {4, 4, ENT_WIND, 0}, {15, 4, ENT_WIND, 1}, {10, 16, ENT_WIND, 2},
    {10, 10, ENT_NOTE, 79}, {4, 14, ENT_NPC, 80}, {10, 9, ENT_MECH, 5},
};
static const Ent ENTS8[11] = {
    {3, 3, ENT_PICKUP, IT_BONE1}, {17, 3, ENT_PICKUP, IT_BONE2},
    {3, 17, ENT_PICKUP, IT_BONE3}, {17, 17, ENT_PICKUP, IT_BONE4},
    {10, 10, ENT_PICKUP, IT_GOLD},
    {6, 10, ENT_SPINE, 0}, {10, 6, ENT_SPINE, 1}, {14, 10, ENT_SPINE, 2},
    {10, 14, ENT_NOTE, 84}, {10, 2, ENT_NPC, 85}, {10, 9, ENT_MECH, 6},
};
/* v11: runes moved into the central plaza (7..12, 7..12) around
 * the mechanism (10,9) — Birth (7,7), Peace (12,7), Dream (7,12),
 * War (12,12). Order unchanged: Dream, Birth, War, Peace. */
static const Ent ENTS9[8] = {
    {7, 7, ENT_RUNE, 0}, {12, 7, ENT_RUNE, 1}, {7, 12, ENT_RUNE, 2},
    {12, 12, ENT_RUNE, 3}, {10, 10, ENT_NOTE, 89},
    {4, 14, ENT_NPC, 90}, {15, 10, ENT_NPC, 91}, {10, 9, ENT_MECH, 7},
};
static const Ent ENTS10[7] = {
    {4, 4, ENT_ORGAN, 0}, {15, 4, ENT_ORGAN, 1}, {10, 16, ENT_ORGAN, 2},
    {10, 10, ENT_NOTE, 94}, {4, 14, ENT_NPC, 95}, {10, 9, ENT_MECH, 8},
    /* v10: Vein Organ hint note, right of the mechanism, so a
     * player arriving in Heart Chamber without the organ can
     * read where it is (Forest of Veins chamber, Bone Fork
     * lock) without leaving the area. */
    {12, 9, ENT_NOTE, 102},
};
static const Ent ENTS11[4] = {
    {10, 5, ENT_THRONE, 0}, {10, 9, ENT_MECH, 9},
    {4, 14, ENT_NPC, 107}, {10, 14, ENT_NOTE, 108},
};
static const Ent *ENTS[12] = {ENTS0, ENTS1, ENTS2, ENTS3, ENTS4, ENTS5, ENTS6, ENTS7, ENTS8, ENTS9, ENTS10, ENTS11};
static const uint8_t ENT_COUNT[12] = {9, 8, 7, 7, 7, 6, 6, 6, 11, 8, 7, 4};
static const int8_t GATE_POS[11][2] = {{15, 8}, {10, 9}, {10, 10}, {10, 9}, {10, 9}, {10, 9}, {10, 9}, {18, 9}, {18, 9}, {18, 9}, {18, 9}};
static const int8_t RET_POS[11][2] = {{1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}, {1, 1}};
static const uint8_t LEVER_ORDER[3] = {1, 0, 2};
static const uint8_t WIND_ORDER[3] = {1, 2, 0};
static const uint8_t MEM_ORDER[4] = {2, 0, 3, 1};
static const uint8_t ORGAN_ORDER[3] = {0, 2, 1};
static const uint8_t BELL_ORDER[3] = {0, 1, 2};
static const uint8_t BONE_ORDER[3] = {2, 0, 1};
static const uint8_t FLOW_ORDER[3] = {0, 2, 1};
static const char *AREA_NAMES[12] = {"THE GATEHOUSE", "ASH VILLAGE", "THE BURIAL MAZE",
    "FOREST OF VEINS", "CATHEDRAL OF TEETH", "BLOOD RIVER", "FLESH MARKET",
    "LUNG GARDENS", "BONE LABYRINTH", "MEMORY VAULT", "HEART CHAMBER",
    "THE THRONE"};
static const uint8_t AREA_NAME_X[12] = {3, 4, 2, 2, 1, 4, 4, 4, 3, 4, 3, 3};

/* v11: Areas 1, 3-5 (12x12) and 7-10 (20x20) are bit-packed
 * wall maps — one bit per cell, 1 = wall — generated from the
 * verified design grids (/tmp/gen_bits.py, which imports
 * /tmp/design3.py and /tmp/design4.py). This replaced both the
 * stored 12x12 tables and the run-table interpreter, which
 * together overflowed plain 32KB: the bit decoder is one short
 * function shared by all eight areas. B/G cells are open bits
 * and are returned by map_char. Wall bits in areas 1 and 3 are
 * trees ('T'); area 1 has a second plane marking its rocks
 * ('R'); all other wall bits are stone ('1').
 *  Lung (7): concentric rings + spokes spiralling to the
 *   central plaza (mechanism (10,9), note (10,10)); vanes
 *   (4,4)(15,4)(10,16) and NPC (4,14) sit on spokes.
 *  Bone (8): horizontal/vertical wall segments with single
 *   gaps — a classic branching maze; all seals, Gold, spines,
 *   note, NPC, mechanism and gate stay reachable.
 *  Memory (9): one long winding corridor (top, east, bottom,
 *   west, mid, east-mid) into the central plaza (7..12, 7..12)
 *   holding the four runes around the mechanism (10,9).
 *  Heart (10): one long narrow serpentine corridor, ~110
 *   steps from the return gate to the Throne gate, with the
 *   note (10,10) and NPC (4,14) as hints along the way. */
/* generated by /tmp/gen_bits.py — v11 bit-packed wall maps */
static const uint8_t BITS_MID[4][18] = {
    {0xFF, 0x1F, 0x80, 0x89, 0x18, 0x80, 0x05, 0x19, 0x80, 0x11, 0x1A, 0x80, 0x89, 0x18, 0x80, 0x01, 0xF8, 0xFF},  /* area 1 */
    {0xFF, 0x1F, 0x80, 0x11, 0x98, 0x88, 0x01, 0x18, 0x80, 0x11, 0x19, 0x80, 0x09, 0x18, 0x80, 0x01, 0xFC, 0xFF},  /* area 3 */
    {0xFF, 0x1F, 0x80, 0x01, 0x98, 0x84, 0x01, 0x18, 0x80, 0x09, 0x18, 0x80, 0x21, 0x18, 0x80, 0x01, 0xFC, 0xFF},  /* area 4 */
    {0xFF, 0x1F, 0x80, 0x01, 0x98, 0x84, 0x01, 0x18, 0x80, 0x41, 0x18, 0x80, 0x09, 0x18, 0x80, 0x01, 0xFC, 0xFF},  /* area 5 */
};
static const uint8_t BITS_ROCK[18] = {
    0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x02, 0x00,
    0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
};
static const uint8_t BITS_BIG[5][50] = {
    {0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x01, 0x00, 0xBC, 0xFE, 0xD7, 0xEB, 0x7F, 0xBD, 0x01, 0xD8, 0xDB, 0xBE, 0xBD, 0xED, 0xDB, 0xDB, 0xB0, 0x3D, 0x0C, 0x9B, 0xDB, 0xB0, 0xBD, 0x0D, 0xDB, 0xDB, 0xBF, 0xBD, 0xFD, 0xDB, 0x0B, 0x80, 0xBD, 0xFE, 0xDF, 0xEB, 0xFB, 0x3D, 0x00, 0xC0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* area 7 */
    {0xFF, 0xFF, 0x1F, 0x00, 0x80, 0x01, 0x40, 0x18, 0x00, 0x84, 0xFD, 0xFD, 0x19, 0x04, 0x80, 0x41, 0x00, 0x18, 0x04, 0x80, 0xD9, 0xFF, 0x1B, 0x00, 0x81, 0x01, 0x10, 0x18, 0x00, 0x81, 0xFD, 0xEB, 0x18, 0x21, 0x88, 0x11, 0x82, 0x18, 0x21, 0x88, 0x71, 0xFF, 0x1B, 0x00, 0x80, 0x01, 0x00, 0xF8, 0xFF, 0xFF},  /* area 8 */
    {0xFF, 0xFF, 0x9F, 0xFF, 0xFF, 0x03, 0x00, 0xFC, 0xFF, 0xDF, 0xFF, 0xFF, 0x7D, 0x00, 0xD8, 0xF7, 0xBF, 0x7D, 0x07, 0xDA, 0x77, 0xA0, 0x7D, 0x07, 0x98, 0x77, 0x20, 0x7C, 0x07, 0xDA, 0x77, 0xA0, 0x7D, 0xFF, 0xDF, 0xE7, 0xFF, 0x7D, 0xFF, 0xDF, 0x07, 0x00, 0xFC, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* area 9 */
    {0xFF, 0xFF, 0x1F, 0x00, 0x80, 0xFF, 0xFF, 0xFB, 0xFF, 0xBF, 0x03, 0x00, 0xB8, 0xFF, 0xFF, 0xFB, 0xFF, 0x3F, 0x00, 0xC0, 0xFF, 0xFF, 0xFD, 0x3F, 0x80, 0xFF, 0xFB, 0xFF, 0xBF, 0xFF, 0x07, 0xF8, 0x7F, 0xFF, 0xFF, 0xE7, 0xFF, 0x7F, 0x00, 0xE0, 0x07, 0x00, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* area 10 */
    {0xFF, 0xFF, 0x1F, 0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x80, 0x41, 0x20, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x04, 0x82, 0x01, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0x18, 0x00, 0x80, 0x01, 0x00, 0xF8, 0xFF, 0xFF},  /* area 11 */
};

static char map_char(uint8_t area, uint8_t x, uint8_t y) {
    const uint8_t *b;
    uint16_t i;
    if (area == 0) return MAPS[0][y][x];
    if (area == 2) return MAPS[1][y][x];
    if (area == 6) return MAPS[2][y][x];
    if (x == 1 && y == 1) return 'B';
    if (area < 7) {
        if (x == 10 && y == 9) return 'G';
        i = (uint16_t)y * 12 + x;
        if (area == 1) {
            if ((BITS_ROCK[i >> 3] >> (i & 7)) & 1) return 'R';
            b = BITS_MID[0];
        } else b = BITS_MID[area - 2];
        if ((b[i >> 3] >> (i & 7)) & 1) return (area == 3 || area == 1) ? 'T' : '1';
        return '0';
    }
    if (area < 11 && x == 18 && y == 9) return 'G';
    i = (uint16_t)y * 20 + x;
    b = BITS_BIG[area - 7];
    if ((b[i >> 3] >> (i & 7)) & 1) return '1';
    return '0';
}
static uint8_t cell_at(uint8_t area, int8_t x, int8_t y) {
    char c;
    if (area > 11 || x < 0 || y < 0 || x >= AREA_W[area] || y >= AREA_H[area]) return 1;
    c = map_char(area, (uint8_t)x, (uint8_t)y);
    if (c == '0') return 0;
    if (c == 'T') return 'T';
    if (c == 'R') return 'R';
    if (c == 'G') return 'G';
    if (c == 'B') return 'B';
    if (c == '3') return 3;
    return 1;
}

/* Per-area BG palettes (8 each, loaded on area entry; CGB holds
 * only 8 at once, so they are switched, never held together).
 * Layout in every area: 0 ceiling/sky, 1 floor, 2 wall near,
 * 3 wall near side, 4 wall far, 5 locked gate, 6 key, 7 gold.
 * Meadow wall palette doubles for trees and rocks: tree pattern
 * uses values 1/2 (dark/light green), rock pattern uses values
 * 1/3 (dark/light grey), and colour 3 is light grey in the meadow
 * wall palettes, so both read correctly under one palette. */
static const uint8_t PAL_IDX[12] = {0, 1, 2, 3, 4, 5, 6, 4, 7, 5, 3, 8};
/* Lung Gardens reuses the Cathedral ivory palette (white
 * branching structures) and Memory Vault reuses the Blood
 * River deep-blue palette (dream depths) and Heart
 * Chamber reuses the Forest red palette: sharing saves
 * 192 ROM bytes, which is what keeps v8 in plain 32KB. */
static const palette_color_t AREA_PALS[9][8][4] = {
    {   /* Area 0: Stone Dungeon */
        {RGB(10, 10, 14), RGB(30, 22, 8), RGB(16, 16, 20), RGB(24, 20, 14)},
        {RGB(10, 10, 14), RGB(12, 7, 3), RGB(20, 12, 5), RGB(28, 18, 9)},
        {RGB(10, 10, 14), RGB(14, 14, 16), RGB(26, 26, 28), RGB(20, 12, 5)},
        {RGB(10, 10, 14), RGB(10, 10, 12), RGB(18, 18, 20), RGB(20, 12, 5)},
        {RGB(10, 10, 14), RGB(8, 8, 10), RGB(14, 14, 16), RGB(20, 12, 5)},
        {RGB(10, 10, 14), RGB(8, 6, 6), RGB(24, 8, 6), RGB(20, 12, 5)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(10, 10, 14), RGB(14, 10, 2), RGB(26, 20, 6), RGB(20, 12, 5)},
    },
    {   /* Area 1: The Meadow. Palette 3 is the v4 NPC palette:
         * compose_raycaster never uses palette 3 for Meadow
         * walls (Meadow walls use 2 near / 4 far regardless of
         * side, and palette 3 here was identical to palette 2),
         * so repurposing it costs nothing. Values: 0 dark
         * outline, 1 blue tunic, 2 skin, 3 brown hair/trousers. */
        {RGB(16, 6, 6), RGB(31, 31, 31), RGB(16, 22, 30), RGB(24, 28, 30)},
        {RGB(16, 6, 6), RGB(12, 10, 8), RGB(18, 16, 12), RGB(24, 20, 16)},
        {RGB(16, 6, 6), RGB(8, 6, 4), RGB(20, 14, 8), RGB(22, 22, 24)},
        {RGB(8, 8, 10), RGB(10, 14, 26), RGB(30, 22, 14), RGB(20, 12, 6)},
        {RGB(16, 6, 6), RGB(6, 6, 6), RGB(12, 12, 12), RGB(15, 15, 17)},
        {RGB(10, 18, 30), RGB(8, 6, 6), RGB(24, 8, 6), RGB(12, 22, 6)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(10, 18, 30), RGB(14, 10, 2), RGB(26, 20, 6), RGB(12, 22, 6)},
    },
    {   /* Area 2: Mossy Crypt */
        {RGB(4, 6, 5), RGB(20, 18, 8), RGB(8, 10, 8), RGB(14, 14, 12)},
        {RGB(4, 6, 5), RGB(6, 6, 7), RGB(12, 12, 13), RGB(18, 18, 19)},
        {RGB(4, 6, 5), RGB(8, 12, 7), RGB(16, 20, 12), RGB(12, 12, 13)},
        {RGB(4, 6, 5), RGB(6, 9, 5), RGB(12, 15, 9), RGB(12, 12, 13)},
        {RGB(4, 6, 5), RGB(4, 7, 4), RGB(9, 12, 7), RGB(12, 12, 13)},
        {RGB(4, 6, 5), RGB(8, 6, 6), RGB(24, 8, 6), RGB(12, 12, 13)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(4, 6, 5), RGB(14, 10, 2), RGB(26, 20, 6), RGB(12, 12, 13)},
    },
    {   /* Area 3: Forest of Veins - dark red / root brown */
        {RGB(12, 4, 4), RGB(30, 20, 10), RGB(14, 8, 6), RGB(22, 14, 10)},
        {RGB(12, 4, 4), RGB(10, 6, 4), RGB(18, 10, 6), RGB(26, 14, 8)},
        {RGB(12, 4, 4), RGB(16, 6, 6), RGB(26, 10, 8), RGB(18, 10, 6)},
        {RGB(12, 4, 4), RGB(12, 5, 5), RGB(20, 9, 7), RGB(18, 10, 6)},
        {RGB(12, 4, 4), RGB(9, 4, 4), RGB(15, 7, 6), RGB(18, 10, 6)},
        {RGB(12, 4, 4), RGB(8, 6, 6), RGB(24, 8, 6), RGB(18, 10, 6)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(12, 4, 4), RGB(14, 10, 2), RGB(26, 20, 6), RGB(18, 10, 6)},
    },
    {   /* Area 4: Cathedral of Teeth - pale ivory / bone */
        {RGB(6, 6, 8), RGB(28, 26, 20), RGB(12, 12, 14), RGB(20, 18, 14)},
        {RGB(6, 6, 8), RGB(14, 13, 11), RGB(22, 20, 16), RGB(28, 25, 20)},
        {RGB(6, 6, 8), RGB(18, 17, 14), RGB(28, 26, 22), RGB(22, 20, 16)},
        {RGB(6, 6, 8), RGB(14, 13, 11), RGB(22, 21, 18), RGB(22, 20, 16)},
        {RGB(6, 6, 8), RGB(10, 10, 9), RGB(17, 16, 14), RGB(22, 20, 16)},
        {RGB(6, 6, 8), RGB(8, 6, 6), RGB(24, 8, 6), RGB(22, 20, 16)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(6, 6, 8), RGB(14, 10, 2), RGB(26, 20, 6), RGB(22, 20, 16)},
    },
    {   /* Area 5: Blood River - dark blue / pulsing red */
        {RGB(4, 4, 12), RGB(28, 18, 12), RGB(10, 10, 16), RGB(20, 12, 12)},
        {RGB(4, 4, 12), RGB(8, 6, 8), RGB(16, 10, 12), RGB(24, 12, 12)},
        {RGB(4, 4, 12), RGB(10, 8, 16), RGB(20, 10, 14), RGB(16, 10, 12)},
        {RGB(4, 4, 12), RGB(8, 7, 13), RGB(16, 9, 12), RGB(16, 10, 12)},
        {RGB(4, 4, 12), RGB(6, 5, 10), RGB(12, 7, 10), RGB(16, 10, 12)},
        {RGB(4, 4, 12), RGB(8, 6, 6), RGB(24, 8, 6), RGB(16, 10, 12)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(4, 4, 12), RGB(14, 10, 2), RGB(26, 20, 6), RGB(16, 10, 12)},
    },
    {   /* Area 6: Flesh Market - scar pink / flesh brown */
        {RGB(10, 4, 6), RGB(28, 22, 18), RGB(14, 10, 10), RGB(22, 16, 14)},
        {RGB(10, 4, 6), RGB(12, 8, 7), RGB(20, 14, 12), RGB(26, 18, 15)},
        {RGB(10, 4, 6), RGB(18, 10, 10), RGB(28, 16, 14), RGB(20, 14, 12)},
        {RGB(10, 4, 6), RGB(14, 8, 8), RGB(22, 13, 12), RGB(20, 14, 12)},
        {RGB(10, 4, 6), RGB(10, 6, 6), RGB(17, 10, 10), RGB(20, 14, 12)},
        {RGB(10, 4, 6), RGB(8, 6, 6), RGB(24, 8, 6), RGB(20, 14, 12)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(10, 4, 6), RGB(14, 10, 2), RGB(26, 20, 6), RGB(20, 14, 12)},
    },
    {   /* Area 7: Lung Gardens - pale white / air green */
        {RGB(18, 22, 18), RGB(30, 30, 28), RGB(16, 20, 16), RGB(24, 26, 22)},
        {RGB(18, 22, 18), RGB(14, 16, 12), RGB(22, 24, 20), RGB(28, 30, 26)},
        {RGB(18, 22, 18), RGB(16, 20, 14), RGB(26, 30, 24), RGB(22, 24, 20)},
        {RGB(18, 22, 18), RGB(12, 16, 11), RGB(20, 24, 18), RGB(22, 24, 20)},
        {RGB(18, 22, 18), RGB(10, 13, 9), RGB(16, 19, 14), RGB(22, 24, 20)},
        {RGB(18, 22, 18), RGB(8, 6, 6), RGB(24, 8, 6), RGB(22, 24, 20)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(18, 22, 18), RGB(14, 10, 2), RGB(26, 20, 6), RGB(22, 24, 20)},
    },
    {   /* Area 8: Bone Labyrinth - ivory / shadow grey */
        {RGB(6, 6, 8), RGB(28, 28, 26), RGB(12, 12, 14), RGB(20, 20, 20)},
        {RGB(6, 6, 8), RGB(14, 14, 13), RGB(22, 22, 21), RGB(28, 27, 24)},
        {RGB(6, 6, 8), RGB(18, 18, 17), RGB(28, 28, 26), RGB(22, 22, 21)},
        {RGB(6, 6, 8), RGB(14, 14, 13), RGB(22, 22, 21), RGB(22, 22, 21)},
        {RGB(6, 6, 8), RGB(10, 10, 10), RGB(17, 17, 16), RGB(22, 22, 21)},
        {RGB(6, 6, 8), RGB(8, 6, 6), RGB(24, 8, 6), RGB(22, 22, 21)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(6, 6, 8), RGB(14, 10, 2), RGB(26, 20, 6), RGB(22, 22, 21)},
    },
    {   /* Area 9: Memory Vault - deep violet / dream blue */
        {RGB(8, 4, 16), RGB(26, 20, 30), RGB(12, 10, 20), RGB(20, 16, 26)},
        {RGB(8, 4, 16), RGB(10, 8, 14), RGB(18, 14, 24), RGB(24, 20, 30)},
        {RGB(8, 4, 16), RGB(14, 10, 22), RGB(24, 18, 30), RGB(18, 14, 24)},
        {RGB(8, 4, 16), RGB(11, 8, 18), RGB(19, 15, 26), RGB(18, 14, 24)},
        {RGB(8, 4, 16), RGB(8, 6, 14), RGB(15, 12, 21), RGB(18, 14, 24)},
        {RGB(8, 4, 16), RGB(8, 6, 6), RGB(24, 8, 6), RGB(18, 14, 24)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(8, 4, 16), RGB(14, 10, 2), RGB(26, 20, 6), RGB(18, 14, 24)},
    },
    {   /* Area 10: Heart Chamber - living red / muscle brown */
        {RGB(12, 3, 5), RGB(30, 22, 18), RGB(14, 8, 8), RGB(22, 14, 12)},
        {RGB(12, 3, 5), RGB(12, 7, 6), RGB(20, 12, 10), RGB(26, 16, 13)},
        {RGB(12, 3, 5), RGB(18, 8, 8), RGB(28, 14, 12), RGB(20, 12, 10)},
        {RGB(12, 3, 5), RGB(14, 6, 6), RGB(22, 11, 10), RGB(20, 12, 10)},
        {RGB(12, 3, 5), RGB(10, 5, 5), RGB(17, 9, 8), RGB(20, 12, 10)},
        {RGB(12, 3, 5), RGB(8, 6, 6), RGB(24, 8, 6), RGB(20, 12, 10)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(12, 3, 5), RGB(14, 10, 2), RGB(26, 20, 6), RGB(20, 12, 10)},
    },
    {   /* Area 11: Throne of the Sleeper - dark crystal / gold */
        {RGB(4, 4, 10), RGB(24, 20, 28), RGB(10, 10, 18), RGB(18, 16, 24)},
        {RGB(4, 4, 10), RGB(8, 7, 12), RGB(16, 13, 20), RGB(22, 18, 26)},
        {RGB(4, 4, 10), RGB(12, 10, 18), RGB(22, 17, 26), RGB(16, 13, 20)},
        {RGB(4, 4, 10), RGB(9, 8, 14), RGB(17, 14, 22), RGB(16, 13, 20)},
        {RGB(4, 4, 10), RGB(7, 6, 11), RGB(13, 11, 18), RGB(16, 13, 20)},
        {RGB(4, 4, 10), RGB(8, 6, 6), RGB(24, 8, 6), RGB(16, 13, 20)},
        {RGB(8, 8, 10), RGB(26, 20, 6), RGB(16, 12, 2), RGB(31, 28, 10)},
        {RGB(4, 4, 10), RGB(14, 10, 2), RGB(26, 20, 6), RGB(16, 13, 20)},
    },
};

static uint8_t cur_tiles[360], cur_attrs[360];
static uint8_t prev_tiles[360], prev_attrs[360];
static uint8_t ch_tile[360], ch_attr[360];
static uint16_t ch_addr[360], ch_idx[360];
static uint16_t ch_count = 0, wr = 0;

static void buf_cell(int8_t x, int8_t y, uint8_t tile, uint8_t pal) {
    uint16_t i;
    if (x < 0 || x >= 20 || y < 0 || y >= 18) return;
    i = (uint16_t)y * 20 + (uint8_t)x;
    cur_tiles[i] = tile;
    cur_attrs[i] = pal;
}

static void scan_diff(void) {
    uint8_t *tp = cur_tiles, *ap = cur_attrs;
    uint8_t *pp = prev_tiles, *qp = prev_attrs;
    uint8_t x, y;
    uint16_t n = 0;
    for (y = 0; y < 18; y++) {
        for (x = 0; x < 20; x++) {
            uint8_t t = *tp, a = *ap;
            if (t != *pp || a != *qp) {
                ch_tile[n] = t;
                ch_attr[n] = a;
                ch_addr[n] = (uint16_t)(0x9800u + ((uint16_t)y << 5) + x);
                ch_idx[n] = (uint16_t)((uint16_t)y * 20 + x);
                n++;
            }
            pp++; qp++; tp++; ap++;
        }
    }
    ch_count = n;
    wr = 0;
}

static void write_list_chunked(void) {
    while (wr < ch_count) {
        uint8_t ly = LY_REG;
        uint16_t a, ci;
        if (ly < 144 || ly > 152) break;
        a = ch_addr[wr];
        ci = ch_idx[wr];
        VBK_REG = 0;
        *(uint8_t *)a = ch_tile[wr];
        VBK_REG = 1;
        *(uint8_t *)a = ch_attr[wr];
        prev_tiles[ci] = ch_tile[wr];
        prev_attrs[ci] = ch_attr[wr];
        wr++;
    }
    VBK_REG = 0;
}

static void write_list_all(void) {
    uint16_t n;
    VBK_REG = 0;
    for (n = 0; n < ch_count; n++) *(uint8_t *)ch_addr[n] = ch_tile[n];
    VBK_REG = 1;
    for (n = 0; n < ch_count; n++) *(uint8_t *)ch_addr[n] = ch_attr[n];
    VBK_REG = 0;
}

static void sync_prev_cur(void) {
    uint16_t i;
    for (i = 0; i < 360; i++) {
        prev_tiles[i] = cur_tiles[i];
        prev_attrs[i] = cur_attrs[i];
    }
}

static void invalidate_prev(void) {
    uint16_t i;
    for (i = 0; i < 360; i++) prev_tiles[i] = prev_attrs[i] = 0xFF;
}

static int16_t cam_x[20];
static int16_t zbuf[20];
static uint8_t g_area = 0;
static uint32_t g_items = 0;
static uint8_t statue_face[3] = {192, 128, 64};
static uint8_t coffin_face[2] = {0, 192};
static uint8_t lever_idx = 0, lever_done = 0;
static uint8_t lever_pulled[3] = {0, 0, 0};
static uint8_t intro_idx = 0;
static uint8_t bell_idx = 0, bell_done = 0;
static uint8_t epitaph_read = 0, slice_done = 0;
static uint8_t bone_idx = 0, bone_done = 0;
static uint8_t flow_idx = 0, flow_done = 0;
static uint8_t river_pulled[3] = {0, 0, 0};
static uint8_t merchant_clues = 0;
static uint8_t trader_done = 0; /* v7: east merchant Amber->Idol trade done */
static uint8_t wind_idx = 0, wind_done = 0, wind_pulled[3] = {0, 0, 0};
static uint8_t mem_idx = 0, mem_done = 0;
static uint8_t organ_idx = 0, organ_done = 0, organ_pulled[3] = {0, 0, 0};
static uint8_t spine_face[3] = {0, 64, 128};
static uint8_t choice_idx = 0, ending_id = 0;
static uint8_t gate_jingled[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static uint8_t audio_music_track = 0;
static uint8_t title_proof[4];
static uint8_t ending_proof[4];
static uint8_t keeper_proof[4];
static uint8_t choice_proof[4];
static uint8_t crypt_open = 0;
static uint8_t inv_page = 0;
static uint8_t code_digit[3] = {0, 0, 0};
static uint8_t code_cursor = 0, code_wrong = 0;
static uint8_t g_state = 0;

/* v4 Meadow NPC: Area 1 only, tile (2,7), west meadow, off the
 * key/gate route, all four neighbours open, walk-through. */
/* NPCs are ENTS1 entries (Elder at (2,7) kept from Area Quest). */

/* v4 minimap fog of war: step-on visited only, per area.
 * visited[area][y*12+x] = 1 once the player has stepped onto
 * that tile (entry tile on area load and gate-arrival tile on
 * any transition count). Persists across returns; cleared only
 * on a new game from the title screen. */
/* v8: visited is a BITMASK (1 bit per tile) instead of a byte
 * per tile: 12 areas x 50 bytes = 600 bytes, not 4800. The
 * byte-per-tile version pushed total RAM past the 0xE000
 * stack (DATA ended at 0xE211) and the game never reached
 * the main loop. Bit index = y*W+x, byte = idx>>3, bit = idx&7. */
static uint8_t visited_bits[12][50];

static void mark_visited(uint8_t area, int8_t x, int8_t y) {
    uint16_t idx;
    if (area > 11 || x < 0 || y < 0 || x >= AREA_W[area] || y >= AREA_H[area]) return;
    idx = (uint16_t)((uint8_t)y * AREA_W[area] + (uint8_t)x);
    visited_bits[area][idx >> 3] = (uint8_t)(visited_bits[area][idx >> 3] | (1 << (idx & 7)));
}
static uint8_t is_visited(uint8_t area, uint8_t mx, uint8_t my) {
    uint16_t idx = (uint16_t)(my * AREA_W[area] + mx);
    return (uint8_t)((visited_bits[area][idx >> 3] >> (idx & 7)) & 1);
}

static void clear_visited(void) {
    uint8_t i;
    uint16_t j;
    for (i = 0; i < 12; i++)
        for (j = 0; j < 50; j++) visited_bits[i][j] = 0;
}

static uint8_t item_held(uint8_t it) {
    return (uint8_t)((g_items >> it) & 1);
}
static uint8_t gate_held(uint8_t area) {
    if (area == 0) return item_held(IT_SIGIL);
    if (area == 1) return item_held(IT_LANTERN);
    if (area == 3) return item_held(IT_BLOOD);
    if (area == 4) return item_held(IT_FORK);
    if (area == 5) return item_held(IT_RIVER);
    if (area == 6) return item_held(IT_LENS);
    if (area == 7) return item_held(IT_LUNG);
    if (area == 8) return item_held(IT_SILVER);
    if (area == 9) return item_held(IT_HEARTSEAL);
    if (area == 10) return item_held(IT_HEARTKEY) && item_held(IT_MARKETKEY);
    return 1;
}
static uint8_t spine_solved(void);
static int8_t find_ent(uint8_t area, int8_t x, int8_t y) {
    uint8_t i;
    for (i = 0; i < ENT_COUNT[area]; i++)
        if (ENTS[area][i].x == (uint8_t)x && ENTS[area][i].y == (uint8_t)y) return (int8_t)i;
    return -1;
}
static uint8_t statues_solved(void) {
    return statue_face[0] == 0 && statue_face[1] == 0 && statue_face[2] == 0;
}
static uint8_t spine_solved(void) {
    return spine_face[0] == 192 && spine_face[1] == 192 && spine_face[2] == 192;
}
static uint8_t bone_seals_all(void) {
    return item_held(IT_BONE1) && item_held(IT_BONE2) && item_held(IT_BONE3) && item_held(IT_BONE4);
}
static uint8_t coffins_solved(void) {
    return coffin_face[0] == 64 && coffin_face[1] == 64;
}
static uint8_t uncollected_pickup_at(uint8_t area, uint8_t mx, uint8_t my) {
    uint8_t i;
    for (i = 0; i < ENT_COUNT[area]; i++) {
        const Ent *e = &ENTS[area][i];
        if (e->kind == ENT_PICKUP && e->x == mx && e->y == my && !item_held(e->aux)) return 1;
    }
    return 0;
}

/* v10 ROM fit: the DDA used 65536L/|ray| — a 32-bit long
 * division — which dragged SDCC's __divslon/__divulon
 * (~750 ROM bytes) into the build and pushed v10 382 bytes
 * over plain 32KB. Exact 16-bit replacement: for r >= 5,
 * 65536/r = 2*(32768/r) + (2*(32768%r))/r (exact identity,
 * since 65536 = 2*32768); for r <= 4 the result exceeds the
 * 16000 clamp anyway, so it is 16000 directly. 32768 and all
 * intermediates fit in uint16_t, so only __divuint/__moduint
 * (already linked) are used and the long-division routines
 * drop out. Rendered output is bit-identical — verified by
 * pixel-diffing identical v9/v10 frames in QA. */
static int16_t delta_dist(int16_t ray) {
    uint16_t ar, q, rem;
    if (ray == 0) return 16000;
    ar = (uint16_t)(ray < 0 ? -ray : ray);
    if (ar < 5) return 16000;
    q = (uint16_t)(32768U / ar);
    rem = (uint16_t)(32768U % ar);
    return (int16_t)((uint16_t)(2 * q) + (uint16_t)((uint16_t)(2 * rem) / ar));
}
static void compose_raycaster(int16_t px, int16_t py, uint8_t ang) {
    /* v8: only the four cardinal facings are ever used
     * (ang = 0/64/128/192), so the 256-entry trig tables
     * (1024 ROM bytes) are replaced by direct values:
     * cos: east 256, south 0, west -256, north 0;
     * sin: east 0, south 256, west 0, north -256. */
    int16_t dir_x = ang == 0 ? 256 : (ang == 128 ? -256 : 0);
    int16_t dir_y = ang == 64 ? 256 : (ang == 192 ? -256 : 0);
    int16_t plane_x = (int16_t)(-(int32_t)dir_y * 169 / 256);
    int16_t plane_y = (int16_t)((int32_t)dir_x * 169 / 256);
    uint8_t col;
    for (col = 0; col < 20; col++) {
        int16_t ray_x = (int16_t)(dir_x + (int32_t)plane_x * cam_x[col] / 256);
        int16_t ray_y = (int16_t)(dir_y + (int32_t)plane_y * cam_x[col] / 256);
        int8_t map_x = (int8_t)(px >> 8), map_y = (int8_t)(py >> 8);
        int16_t ddx, ddy, sdx, sdy;
        int8_t step_x, step_y, side = 0, hit = 0;
        uint8_t cell = 1;
        int16_t perp, line_h, top, bottom;
        int8_t top_tile, bot_tile, top_edge, bot_edge;
        uint8_t variant, pal;
        int8_t row;

        ddx = delta_dist(ray_x);
        ddy = delta_dist(ray_y);
        if (ray_x < 0) {
            step_x = -1;
            sdx = (int16_t)(((int32_t)(px & 255) * ddx) >> 8);
        } else {
            step_x = 1;
            sdx = (int16_t)(((int32_t)(256 - (px & 255)) * ddx) >> 8);
        }
        if (ray_y < 0) {
            step_y = -1;
            sdy = (int16_t)(((int32_t)(py & 255) * ddy) >> 8);
        } else {
            step_y = 1;
            sdy = (int16_t)(((int32_t)(256 - (py & 255)) * ddy) >> 8);
        }
        {
            uint8_t steps = 0;
            while (!hit && steps < 32) {
                if (sdx < sdy) {
                    sdx += ddx; map_x += step_x; side = 0;
                } else {
                    sdy += ddy; map_y += step_y; side = 1;
                }
                cell = cell_at(g_area, map_x, map_y);
                if (cell != 0) hit = 1;
                steps++;
            }
        }
        if (side == 0) perp = (int16_t)(sdx - ddx); else perp = (int16_t)(sdy - ddy);
        if (perp < 16) perp = 16;
        zbuf[col] = perp;
        /* v10: 36864 fits in uint16_t, so this is an exact
         * 16-bit division too (was a long division, same
         * oversized routine as the DDA above). */
        line_h = (int16_t)(36864U / (uint16_t)perp);
        if (line_h > 128) line_h = 128;
        top = (144 - line_h) / 2;
        bottom = top + line_h - 1;
        if (top < 0) top = 0;
        if (bottom > 143) bottom = 143;
        top_tile = (int8_t)(top / 8);
        bot_tile = (int8_t)(bottom / 8);
        top_edge = (int8_t)(top % 8);
        bot_edge = (int8_t)(bottom % 8);
        if (cell == 'G') variant = 6;
        else if (cell == 'B') variant = 6;
        else if (cell == 'T') variant = 2;
        else if (cell == 'R') variant = 3;
        else if (g_area == 2) variant = (uint8_t)(4 + (((uint8_t)(map_x + map_y)) & 1));
        else variant = (uint8_t)(((uint8_t)(map_x + map_y)) & 1);
        if (cell == 3) pal = 7;
        else if (cell == 'B') pal = 7;
        else if (cell == 'G') pal = gate_held(g_area) ? 7 : 5;
        else if (g_area == 1) pal = (perp < 512) ? 2 : 4;
        else if (perp < 512) pal = (uint8_t)(2 + (side ? 1 : 0));
        else pal = 4;

        for (row = 0; row < 18; row++) {
            if (row < top_tile) {
                buf_cell((int8_t)col, row,
                         (((col * 3 + row * 5) & 7) == 0) ? T_STAR : T_SKY, 0);
            } else if (row == top_tile && row == bot_tile) {
                buf_cell((int8_t)col, row, (uint8_t)WALL_FULL(variant), pal);
            } else if (row == top_tile) {
                buf_cell((int8_t)col, row, (uint8_t)WALL_TOP(variant, top_edge), pal);
            } else if (row < bot_tile) {
                buf_cell((int8_t)col, row, (uint8_t)WALL_FULL(variant), pal);
            } else if (row == bot_tile) {
                buf_cell((int8_t)col, row, (uint8_t)WALL_BOT(variant, bot_edge), pal);
            } else {
                buf_cell((int8_t)col, row,
                         (uint8_t)(FLOOR_BASE(g_area) + (((col + row) & 1) ? 1 : 0)), 1);
            }
        }
    }
}

static const char FONT_ORDER[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 .,!?'-:;()/&><+=\"";

static void set_cell(int8_t x, int8_t y, uint8_t tile, uint8_t pal) {
    if (x < 0 || x >= 20 || y < 0 || y >= 18) return;
    VBK_REG = 0;
    set_bkg_tile_xy((uint8_t)x, (uint8_t)y, tile);
    VBK_REG = 1;
    set_bkg_tile_xy((uint8_t)x, (uint8_t)y, pal);
    VBK_REG = 0;
}

static void put_text(uint8_t x, uint8_t y, const char *str, uint8_t pal) {
    uint8_t i = 0;
    while (str[i]) {
        const char *p = FONT_ORDER;
        uint8_t idx = 0;
        while (*p && *p != str[i]) { p++; idx++; }
        if (*p) set_cell((int8_t)(x + i), (int8_t)y, (uint8_t)(FONT_BASE + idx), pal);
        i++;
    }
}

static void fill_all(uint8_t tile, uint8_t pal) {
    uint8_t x, y;
    for (y = 0; y < 18; y++)
        for (x = 0; x < 20; x++)
            set_cell((int8_t)x, (int8_t)y, tile, pal);
}

static void hide_all_sprites(void) {
    uint8_t i;
    for (i = 0; i < 40; i++) move_sprite(i, 0, 200);
}

static void load_area_palettes(uint8_t area) {
    set_bkg_palette(0, 8, (palette_color_t *)AREA_PALS[PAL_IDX[area]]);
}

/* Text screens: background = the area's ceiling/sky, glyphs white,
 * glyph background = same ceiling colour so text has no dark box. */
static void text_palette_for(uint8_t area) {
    palette_color_t tp[4];
    tp[0] = AREA_PALS[PAL_IDX[area]][0][0];
    tp[1] = RGB(14, 10, 2);
    tp[2] = RGB(26, 20, 6);
    tp[3] = RGB(31, 31, 31);
    set_bkg_palette(7, 1, tp);
}

/* v2 audio globals, defined with the SFX block below. */
static uint8_t audio_music_on, audio_music_pos, audio_music_tick;

static void draw_title(void) {
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    load_area_palettes(0);
    text_palette_for(0);
    fill_all(T_SKY, 0);
    /* v8 title graphic: a wound/eye diamond (gate-door tiles in
     * gold around a central eye) above the title text. */
    set_cell(9, 1, (uint8_t)WALL_FULL(6), 6);
    set_cell(8, 2, (uint8_t)WALL_FULL(6), 6);
    set_cell(10, 2, (uint8_t)WALL_FULL(6), 6);
    set_cell(9, 3, (uint8_t)WALL_FULL(6), 6);
    put_text(9, 2, "O", 7);
    put_text(2, 4, "THE NINTH WOUND", 7);
    put_text(2, 6, "AREAS 1-12 - V9", 7);
    title_proof[0] = 'T'; title_proof[1] = 'G'; title_proof[2] = 1;
    title_proof[3] = (uint8_t)('T' + 'G' + 1);
    put_text(4, 11, "PRESS START", 7);
    audio_music_on = 1;
    audio_music_track = 0;
    audio_music_pos = 0;
    audio_music_tick = 0;
    put_text(3, 13, "UP/DOWN: STEP", 7);
    put_text(1, 14, "LEFT/RIGHT: TURN", 7);
    put_text(4, 15, "A: USE / TALK", 7);
    put_text(2, 16, "ITEMS OPEN GATES", 7);
    DISPLAY_ON;
}

/* v8 final-choice screen (ST_CHOICE). Left/Right cycle the three
 * endings, A confirms, Start cancels back to play. The selected
 * option draws in gold (palette 6), the others in white (7). */
static void draw_choice(void) {
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    load_area_palettes(11);
    text_palette_for(11);
    fill_all(T_SKY, 0);
    put_text(4, 2, "FINAL CHOICE", 7);
    put_text(5, 5, "THE WARDEN", choice_idx == 0 ? 6 : 7);
    put_text(4, 6, "KEEP IT LOCKED", 7);
    put_text(3, 8, "THE AWAKENING", choice_idx == 1 ? 6 : 7);
    put_text(7, 9, "FREE IT", 7);
    put_text(7, 11, "ESCAPE", choice_idx == 2 ? 6 : 7);
    put_text(8, 12, "LEAVE", 7);
    put_text(1, 14, "LEFT/RIGHT: CHOOSE", 7);
    put_text(5, 15, "A: CONFIRM", 7);
    put_text(3, 16, "START: CANCEL", 7);
    DISPLAY_ON;
}

/* v8 ending screen (ST_END). A drawn emblem (throne arch + eye
 * diamond, BG tiles) plus ending text, never plain text alone.
 * Each ending starts its own music track (see audio_music_step):
 * 0 Warden = slow low track, 1 Awakening = faster rising track,
 * 2 Escape = mid-tempo major-ish track. */
static void draw_ending(uint8_t id) {
    uint8_t pal = id == 0 ? 5 : (id == 1 ? 6 : 2);
    uint8_t y;
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    load_area_palettes(11);
    text_palette_for(11);
    fill_all(T_SKY, 0);
    for (y = 2; y < 8; y++) {
        set_cell(6, (int8_t)y, (uint8_t)WALL_FULL(6), pal);
        set_cell(13, (int8_t)y, (uint8_t)WALL_FULL(6), pal);
    }
    set_cell(9, 2, (uint8_t)WALL_FULL(6), pal);
    set_cell(8, 3, (uint8_t)WALL_FULL(6), pal);
    set_cell(10, 3, (uint8_t)WALL_FULL(6), pal);
    set_cell(9, 4, (uint8_t)WALL_FULL(6), pal);
    put_text(9, 3, "O", 7);
    if (id == 0) {
        put_text(5, 8, "THE WARDEN", 7);
        put_text(2, 10, "SLEEPER STAYS", 7);
        put_text(3, 11, "LOCKED. YOU KEEP", 7);
        put_text(5, 12, "THE KEY.", 7);
    } else if (id == 1) {
        put_text(3, 8, "THE AWAKENING", 7);
        put_text(2, 10, "SLEEPER WAKES.", 7);
        put_text(3, 11, "WORLD CHANGES.", 7);
        put_text(4, 12, "YOU LET IT OUT.", 7);
    } else {
        put_text(7, 8, "ESCAPE", 7);
        put_text(1, 10, "GATES RESTORE.", 7);
        put_text(2, 11, "YOU WALK OUT.", 7);
        put_text(5, 12, "THE LIGHT.", 7);
    }
    put_text(4, 14, "PRESS START", 7);
    ending_proof[0] = 'E'; ending_proof[1] = 'N';
    ending_proof[2] = id;
    ending_proof[3] = (uint8_t)('E' + 'N' + id);
    ending_id = id;
    audio_music_track = (uint8_t)(id + 1);
    audio_music_on = 1;
    audio_music_pos = 0;
    audio_music_tick = 0;
    DISPLAY_ON;
}

/* Forward declarations: proof globals defined below. */
static uint32_t g_items;
static int8_t g_ptx;
static int8_t g_pty;

/* v6 inventory screen: paged, 8 items per page (v7: 15 items total:
 * page 1 = items 0-7, page 2 = items 8-14). Inventory always opens
 * on page 1; Left/Right page (wraps at both ends), Start closes.
 * The page indicator and
 * the paging control are printed on the screen itself, and the
 * current page is in the inventory proof block for QA. */
static void draw_inventory(void) {
    uint8_t slot, idx;
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    load_area_palettes(g_area);
    text_palette_for(g_area);
    fill_all(T_SKY, 0);
    put_text(5, 2, "INVENTORY", 7);
    put_text(AREA_NAME_X[g_area], 4, AREA_NAMES[g_area], 7);
    put_text(6, 5, inv_page == 0 ? "PAGE 1/4" : (inv_page == 1 ? "PAGE 2/4" : (inv_page == 2 ? "PAGE 3/4" : "PAGE 4/4")), 7);
    for (slot = 0; slot < 8; slot++) {
        idx = (uint8_t)(inv_page * 8 + slot);
        if (idx >= 28) break;
        put_text(2, (uint8_t)(6 + slot), ITEM_NAMES[idx], 7);
        put_text(17, (uint8_t)(6 + slot), item_held(idx) ? "YES" : "--", 7);
    }
    put_text(2, 15, "LEFT/RIGHT: PAGE", 7);
    put_text(4, 16, "START: CLOSE", 7);
    DISPLAY_ON;
}

/* v6 crypt code entry screen (state ST_CODE). Controls are
 * printed on the screen: Left/Right select a digit, Up/Down
 * change it (wraps 0-9), A confirms (advances, submits on the
 * third digit), Start cancels back to play. The selected digit
 * draws in gold (palette 6), the others in white (palette 7),
 * and a DIGIT n line names the selected digit. A wrong code
 * shows WRONG CODE - TRY AGAIN, resets the digits, and stays
 * on this screen for a retry; nothing is lost. */
static void draw_code(void) {
    uint8_t i;
    char one[2];
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    load_area_palettes(2);
    text_palette_for(2);
    fill_all(T_SKY, 0);
    put_text(5, 2, "CRYPT CODE", 7);
    put_text(3, 4, "ENTER THE CODE", 7);
    for (i = 0; i < 3; i++) {
        one[0] = (char)('0' + code_digit[i]);
        one[1] = 0;
        put_text((uint8_t)(8 + i * 2), 6, one, i == code_cursor ? 6 : 7);
    }
    one[0] = (char)('1' + code_cursor);
    one[1] = 0;
    put_text(7, 8, "DIGIT", 7);
    put_text(13, 8, one, 7);
    if (code_wrong) {
        put_text(5, 10, "WRONG CODE", 7);
        put_text(5, 11, "TRY AGAIN", 7);
    }
    put_text(1, 13, "LEFT/RIGHT: DIGIT", 7);
    put_text(2, 14, "UP/DOWN: CHANGE", 7);
    put_text(5, 15, "A: CONFIRM", 7);
    put_text(3, 16, "START: CANCEL", 7);
    DISPLAY_ON;
}

/* v3 minimap palettes (dedicated, restored on return to play by
 * load_area_palettes + render_one_frame). Cells are solid T_SKY
 * tiles, so only colour 0 of each palette shows:
 *  0 wall/tree/rock + background (dark), 1 open (light),
 *  2 player (bright yellow), 3 uncollected key (gold),
 *  4 forward gate locked (red), 5 forward gate unlocked /
 *  Area 3 exit / return gate (light gold -- a gate is a gate,
 *  return gates are NOT a different colour, corrected v3),
 *  6 spare (same light gold, unused),
 *  7 text (dark bg, white glyph value 3). */
static const palette_color_t MINIMAP_PALS[8][4] = {
    {RGB(8, 8, 12), RGB(8, 8, 12), RGB(8, 8, 12), RGB(8, 8, 12)},
    {RGB(24, 24, 24), RGB(24, 24, 24), RGB(24, 24, 24), RGB(24, 24, 24)},
    {RGB(31, 28, 10), RGB(31, 28, 10), RGB(31, 28, 10), RGB(31, 28, 10)},
    {RGB(24, 18, 4), RGB(24, 18, 4), RGB(24, 18, 4), RGB(24, 18, 4)},
    {RGB(24, 6, 6), RGB(24, 6, 6), RGB(24, 6, 6), RGB(24, 6, 6)},
    {RGB(28, 22, 8), RGB(28, 22, 8), RGB(28, 22, 8), RGB(28, 22, 8)},
    {RGB(28, 22, 8), RGB(28, 22, 8), RGB(28, 22, 8), RGB(28, 22, 8)},
    {RGB(4, 4, 6), RGB(4, 4, 6), RGB(4, 4, 6), RGB(31, 31, 31)},
};

/* v3 minimap screen (Select in play; Select or Start to return).
 * Current area only, 12x12 grid of one BG cell per map cell,
 * centred: x 4..15, y 3..14, area name at the top. Uses only
 * existing tiles (T_SKY + font); no new tile system.
 * v4 FOG OF WAR: only step-on visited tiles are shown, per
 * area, persisting across returns. Unvisited tiles draw as
 * blank dark cells (palette 0, same as a wall) with NO wall/
 * open/key/gate/exit information -- strict step-on only, the
 * raycast view itself is NOT fogged. The player's tile is
 * always visited, so it always shows. Consequence, correct
 * fog behaviour: a key tile only becomes visited by stepping
 * onto it, which collects it, so an uncollected key never
 * appears gold on the minimap. */
static void draw_minimap(void) {
    uint8_t mx, my;
    int8_t ox = (int8_t)((20 - (int16_t)AREA_W[g_area]) / 2);
    int8_t oy = AREA_H[g_area] > 18 ? -1 : (int8_t)((18 - (int16_t)AREA_H[g_area]) / 2);
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    set_bkg_palette(0, 8, (palette_color_t *)MINIMAP_PALS);
    fill_all(T_SKY, 0);
    /* v9: name is drawn AFTER the grid loop below (a
     * pre-loop name is overwritten by the grid itself:
     * 16x16 grids start at row 1, 20x20 grids at row 0). */
    for (my = 0; my < AREA_H[g_area]; my++) {
        for (mx = 0; mx < AREA_W[g_area]; mx++) {
            char c = map_char(g_area, mx, my);
            uint8_t pal;
            if (mx == (uint8_t)g_ptx && my == (uint8_t)g_pty) pal = 2;
            else if (!is_visited(g_area, mx, my)) pal = 0;
            else if (c == '1' || c == 'T' || c == 'R') pal = 0;
            else if (c == 'G') pal = gate_held(g_area) ? 5 : 4;
            else if (c == 'B') pal = 5;
            else if (c == '3') pal = 5;
            else if (uncollected_pickup_at(g_area, mx, my)) pal = 3;
            else pal = 1;
            set_cell((int8_t)(ox + mx), (int8_t)(oy + my), T_SKY, pal);
        }
    }
    if (AREA_H[g_area] <= 16) {
        put_text(2, 16, "SELECT TO CLOSE", 7);
        put_text(6, 17, "OR START", 7);
    }
    /* v9: name on the minimap for ALL areas, drawn last so
     * the grid cannot cover it. 12x12: row 1, above the
     * grid. 16x16: row 1, over the border row (no info
     * lost). 20x20: row 0 = map row 1, starting at x>=3
     * so the return gate at (1,1) stays visible. */
    put_text(AREA_NAME_X[g_area], AREA_H[g_area] <= 16 ? 1 : 0, AREA_NAMES[g_area], 7);
    DISPLAY_ON;
}

/* v4 dialog box (A when adjacent to and facing the Meadow NPC).
 * Covers ONLY the bottom half of the screen (rows 9-17, 72px),
 * leaving the top half raycast view visible and untouched:
 * it writes dialog background + text cells directly to the
 * bottom rows in VRAM (DISPLAY_OFF, write, DISPLAY_ON) and
 * never clears the screen, unlike Start/Select which take the
 * full screen. Text uses palette 6 (key gold: glyph colour 3
 * is light gold on dark colour 0) and NO palette is changed,
 * so the top half's palettes are untouched too. Every line
 * satisfies x + length <= 20. Closing restores the full view
 * with one render_one_frame() call. */
/* Dialog lines: 32 dialogs x 4 lines, each line x+len<=20 (x=1). */
static const char *DIALOGS[109][4] = {
    {"THE BELLS SING:", "WEST, THEN EAST,", "THEN NORTH.", ""},
    {"THE TOWER SLEEPS", "UNTIL THE BELLS", "ARE RUNG TRUE.", ""},
    {"GRANDFATHER SAYS", "THE SKY WAS NOT", "ALWAYS RED.", ""},
    {"THE BREATHING IS", "LOUDER THIS YEAR.", "NOBODY ELSE SEEMS", "CONCERNED."},
    {"THE DEAD FACE", "THE GATE.", "", ""},
    {"THE STATUES TURN", "TO FACE THE GATE.", "", ""},
    {"THE LEVERS GROAN.", "THE ORDER RESETS.", "", ""},
    {"THE GATE NEEDS:", "COG, PIN, SEAL.", "", ""},
    {"THE STATUES STILL", "TURN AWAY.", "", ""},
    {"THE BRONZE SIGIL", "CLICKS INTO PLACE.", "", ""},
    {"THE BELL RINGS.", "", "", ""},
    {"THE BELLS FALL", "SILENT, OUT OF", "ORDER.", ""},
    {"THE TOWER HUMS", "AWAKE.", "", ""},
    {"THE BELLS ARE", "ALREADY RUNG TRUE.", "", ""},
    {"THE TOWER IS DARK.", "THE BELLS MUST", "SING FIRST.", ""},
    {"THE ASH LANTERN", "BURNS BRIGHT.", "HIDDEN WRITING", "CAN NOW BE READ."},
    {"THE LANTERN TOWER", "ALREADY BURNS.", "", ""},
    {"FAINT SCRATCHES,", "UNREADABLE IN", "THE DARK.", ""},
    {"THE EPITAPH READS:", "SEVEN, THREE, NINE.", "", ""},
    {"THE COFFINS FACE", "THE PULSE.", "", ""},
    {"THE CRYPT IS SEALED.", "THREE GRAVE SEALS", "ARE NEEDED.", ""},
    {"THE COFFINS STILL", "FACE AWAY FROM", "THE PULSE.", ""},
    {"THE FINAL DOOR", "NEEDS THE EPITAPH", "CODE.", ""},
    {"THE GATE MECHANISM", "ALREADY RUNS.", "", ""},
    {"THE GATE IS BROKEN.", "FIND COG, PIN, SEAL.", "TURN THE STATUES.", ""},
    {"THE STATUE TURNS", "TO FACE EAST.", "", ""},
    {"THE STATUE TURNS", "TO FACE SOUTH.", "", ""},
    {"THE STATUE TURNS", "TO FACE WEST.", "", ""},
    {"THE STATUE TURNS", "TO FACE NORTH.", "", ""},
    {"THE COFFIN TURNS", "TO FACE EAST.", "", ""},
    {"THE COFFIN TURNS", "TO FACE SOUTH.", "", ""},
    {"THE COFFIN TURNS", "TO FACE WEST.", "", ""},
    {"THE COFFIN TURNS", "TO FACE NORTH.", "", ""},
    {"YOU WAKE IN THE", "DARK, BENEATH", "THE WORLD.", ""},
    {"YOU DO NOT", "REMEMBER HOW YOU", "GOT HERE.", ""},
    {"THE PEOPLE HERE", "CALL THIS PLACE", "THE NINE HELLS.", ""},
    {"WHERE ARE YOU?", "HOW DID YOU", "GET HERE?", ""},
    {"EXPLORE. FIND", "THE GATES.", "GO DEEPER.", ""},
    {"THE PULSE FLOWS", "SOUTH THROUGH", "THE BODY.", "FACE THE PULSE."},
    {"VALVES SING:", "NORTH, THEN WEST,", "THEN EAST.", ""},
    {"THE ROOTS DRINK", "WHERE THE VALVES", "POINT THEM.", ""},
    {"THE BARRIER", "HOLDS. SET THE", "VALVES FIRST.", ""},
    {"THE BARRIER", "ALREADY OPEN.", "", ""},
    {"THE BLOOD SIGIL", "PULSES. LIVING", "WOOD OBEYS YOU.", ""},
    {"THE BONES SING:", "HIGH, LOW,", "THEN MIDDLE.", ""},
    {"THE CATHEDRAL", "WAS BUILT FROM", "THOSE WHO SANG.", ""},
    {"THE COLUMN", "SOUNDS.", "", ""},
    {"THE TONES CLASH.", "THE SEQUENCE", "RESETS.", ""},
    {"THE BONES HUM", "IN TUNE.", "", ""},
    {"THE SKELETAL", "MECHANISM SLEEPS.", "TUNE THE BONES.", ""},
    {"THE BONE FORK", "VIBRATES. BONE", "MECHANISMS WAKE.", ""},
    {"THE MECHANISM", "ALREADY HUMS.", "", ""},
    {"OPEN WEST, THEN", "EAST, THEN", "NORTH.", ""},
    {"THE RIVER CARRIES", "ALL WHO PAY", "THE FLOW.", ""},
    {"THE VALVE GROANS", "OPEN.", "", ""},
    {"THE FLOW CHOKES.", "THE VALVES RESET.", "", ""},
    {"THE CHANNELS", "RUN TRUE.", "", ""},
    {"THE DOCK GATE", "NEEDS THE FLOW", "SET TRUE.", ""},
    {"THE RIVER SEAL", "FLOWS TO YOU.", "FAR SHORES OPEN.", ""},
    {"THE DOCK ALREADY", "STANDS OPEN.", "", ""},
    {"THE EAST MERCHANT", "TRADES FOR RARE", "GOODS. SEEK HIM.", ""},
    {"THE THIEF WEARS", "A BONE RING.", "", ""},
    {"BRING BLOOD AMBER", "TO THE EAST", "MERCHANT.", ""},
    {"THE TRADE IS MADE.", "THE EYE LENS", "SEES TRUE.", ""},
    {"THE LENS IS", "YOURS.", "", ""},
    {"SEVEN, THREE, NINE.", "THE CRYPT OPENS.", "ANCIENT KEY FRAG", "MENT FOUND."},
    {"THE VALVES ARE", "ALREADY SET.", "", ""},
    {"THE VALVES HUM.", "THE BARRIER", "SHUDDERS.", ""},
    {"I TRADE ONLY FOR", "BLOOD AMBER.", "BRING ME SOME.", ""},
    {"THE AMBER IS", "MINE. TAKE THIS", "PALE IDOL.", ""},
    {"I WAIT FOR THE", "PALE IDOL.", "BRING IT TO ME.", ""},
    {"THE IDOL IS", "YOURS NOW.", "TRADE COMPLETE.", ""},
    {"NEEDS BONE", "TUNING FORK.", "", ""},
    {"VEIN ORGAN", "FOUND.", "", ""},
    {"I SELL THE", "MKT KEY", "FOR GOLD.", ""},
    {"GOLD FOR KEY.", "THE MARKET KEY", "IS YOURS.", ""},
    {"TRADE COMPLETE.", "", "", ""},
    {"NEEDS LUNG", "BELLOWS.", "", ""},
    {"BINDING THREAD", "FOUND.", "", ""},
    {"WINDS BLOW:", "EAST, NORTH,", "THEN WEST.", ""},
    {"FOLLOW THE WIND.", "", "", ""},
    {"SET THE WINDS", "FIRST.", "", ""},
    {"ALREADY BREATHES.", "", "", ""},
    {"LUNG BELLOWS", "FOUND.", "", ""},
    {"SPINE POINTS", "NORTH.", "", ""},
    {"FOUR SEALS,", "TRUE SPINE.", "", ""},
    {"SEALS AND SPINE", "MUST BE TRUE.", "", ""},
    {"ALREADY HAVE IT.", "", "", ""},
    {"SILVER SIGIL", "FOUND.", "", ""},
    {"MEMORIES IN", "ORDER: DREAM,", "BIRTH, WAR,", "PEACE."},
    {"RECORDS LIE.", "", "", ""},
    {"TRUST RECORDS.", "", "", ""},
    {"SET MEMORIES", "FIRST.", "", ""},
    {"HEART SEAL", "FOUND.", "", ""},
    {"ORGANS BEAT:", "WEST, NORTH,", "THEN EAST.", ""},
    {"NEEDS HEART", "AND MARKET KEY.", "", ""},
    {"SET ORGANS", "FIRST.", "", ""},
    {"NEEDS VEIN", "ORGAN. FOREST", "OF VEINS.", "BONE TUNING FORK."},
    {"HEART KEY", "FOUND.", "", ""},
    {"STILL MISSING.", "", "", ""},
    {"NINTH KEY FOUND.", "", "", ""},
    {"BRING NINTH KEY.", "", "", ""},
    {"VEIN ORGAN:", "FOREST OF VEINS", "CHAMBER.", "BONE TUNING FORK."},
    {"IT SHIFTS.", "", "", ""},
    {"THEY ALIGN.", "", "", ""},
    {"THE ORDER RESETS.", "", "", ""},
    {"ALREADY SET.", "", "", ""},
    {"IT DREAMS.", "", "", ""},
    {"IT DREAMS HERE.", "", "", ""},
};
static void draw_intro_lines(uint8_t d) {
    uint8_t x, y;
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    for (y = 9; y < 18; y++)
        for (x = 0; x < 20; x++) set_cell((int8_t)x, (int8_t)y, T_SKY, 6);
    if (DIALOGS[d][0][0]) put_text(1, 10, DIALOGS[d][0], 6);
    if (DIALOGS[d][1][0]) put_text(1, 11, DIALOGS[d][1], 6);
    if (DIALOGS[d][2][0]) put_text(1, 12, DIALOGS[d][2], 6);
    if (DIALOGS[d][3][0]) put_text(1, 13, DIALOGS[d][3], 6);
    if (d < 44) put_text(2, 16, "PRESS A TO CONTINUE", 6);
    else put_text(2, 16, "PRESS A TO BEGIN", 6);
    DISPLAY_ON;
}

/* v10 dialog proof: 'D','L',last dialog id, checksum — lets
 * QA prove which dialog a talk/mechanism actually showed
 * (e.g. Forest chamber refusal = 96, Heart note = 128). */
static uint8_t dlg_proof[4];
/* v10: one shared dialog-box clear, used by both dialog
 * drawers — deduplicating this loop is what keeps v10
 * inside plain 32KB. */
static void clear_dialog_box(void) {
    uint8_t x, y;
    for (y = 9; y < 18; y++)
        for (x = 0; x < 20; x++) set_cell((int8_t)x, (int8_t)y, T_SKY, 6);
}
static void draw_dialog_lines(uint8_t d) {
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    dlg_proof[0] = 'D'; dlg_proof[1] = 'L'; dlg_proof[2] = d;
    dlg_proof[3] = (uint8_t)('D' + 'L' + d);
    clear_dialog_box();
    if (DIALOGS[d][0][0]) put_text(1, 10, DIALOGS[d][0], 6);
    if (DIALOGS[d][1][0]) put_text(1, 11, DIALOGS[d][1], 6);
    if (DIALOGS[d][2][0]) put_text(1, 12, DIALOGS[d][2], 6);
    if (DIALOGS[d][3][0]) put_text(1, 13, DIALOGS[d][3], 6);
    put_text(2, 16, "PRESS A TO CLOSE", 6);
    DISPLAY_ON;
}

/* v10 Memory Vault rune labels. Runes share one billboard
 * set, so their identity was invisible: the generic sequence
 * dialogs (103-106 after the v11 compaction) never named the
 * rune. Now every rune interaction draws "<NAME> RUNE" + its
 * status, reusing the generic status strings (103 IT SHIFTS /
 * 104 THEY ALIGN / 105 THE ORDER RESETS / 106 ALREADY SET) so
 * no new dialog strings are needed. Rune proof 'R','N',aux,
 * status,checksum lets QA prove the label matched the rune
 * toggled.
 * aux->name: 0 Birth (7,7), 1 Peace (12,7), 2 Dream (7,12),
 * 3 War (12,12) — the order hint (Dream, Birth, War, Peace)
 * is still solved from these labels + the NPC hint. */
static const char *RUNE_NAMES[4] = {"BIRTH", "PEACE", "DREAM", "WAR"};
static uint8_t rune_proof[5];
static void draw_rune_dialog(uint8_t aux, uint8_t status) {
    uint8_t i = 0;
    char line[12];
    const char *n = RUNE_NAMES[aux];
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    rune_proof[0] = 'R'; rune_proof[1] = 'N'; rune_proof[2] = aux;
    rune_proof[3] = status; rune_proof[4] = (uint8_t)('R' + 'N' + aux + status);
    clear_dialog_box();
    while (n[i]) { line[i] = n[i]; i++; }
    line[i++] = ' '; line[i++] = 'R'; line[i++] = 'U'; line[i++] = 'N'; line[i++] = 'E'; line[i] = 0;
    put_text(1, 10, line, 6);
    put_text(1, 11, DIALOGS[103 + status][0], 6);
    put_text(2, 16, "PRESS A TO CLOSE", 6);
    DISPLAY_ON;
}

/* v9 area-title banner: bottom-half box over the rendered
 * frame, shown only on FIRST entry through a newly unlocked
 * gate (never on returns, never on game start). Area 11's
 * banner name is the full "THRONE OF THE SLEEPER"; other
 * areas use AREA_NAMES. Timer is main()'s bump_timer (180
 * vblanks ~= 3s, decremented at the loop top); A dismisses
 * early. Closing re-renders the frame, like dialog close. */
static void draw_banner(uint8_t area) {
    /* No box fill: each text cell is itself a dark square
     * with gold glyphs (palette 6), so the name reads as a
     * banner over the raycast view without hiding it. This
     * also keeps v9 inside plain 32KB (a full box fill
     * pushed the ROM 29 bytes over). */
    DISPLAY_OFF;
    SCX_REG = 0;
    hide_all_sprites();
    /* "THRONE OF THE SLEEPER" is 21 chars, one more than
     * the 20-tile screen, so it wraps over two lines
     * instead of clipping to "THRONE OF THE SLEEPE". */
    if (area == 11) {
        put_text(3, 13, "THRONE OF THE", 6);
        put_text(6, 14, "SLEEPER", 6);
    }
    else put_text(AREA_NAME_X[area], 14, AREA_NAMES[area], 6);
    put_text(2, 16, "PRESS A", 6);
    DISPLAY_ON;
}

/* ---- v2 audio ----
 * Title music: a non-blocking 64-note D HARMONIC MINOR track (4 phrases,
 * slow: one note per 30 vblanks, with rests, low D drone)
 * channel 1, with a low D drone on channel 2 every 4th note.
 * It runs ONLY on the title screen (audio_music_on = 1); leaving
 * the title stops it, and in-game there is NO background music,
 * only event SFX. SFX ids (audio_last_sfx, also in the audio
 * proof block for PyBoy QA): 1 step, 2 bump, 3 pickup,
 * 4 talk, 5 lever, 6 bell west, 7 bell east, 8 bell north,
 * 9 statue rotate, 10 coffin rotate, 11 gate unlock,
 * 12 gate transition (RETIRED in v10 — repeat crossings
 * are silent, audio_last_sfx = 0), 13 reward, 14 puzzle solved,
 * 15 slice complete. */
static uint8_t audio_music_on = 0;
static uint8_t audio_music_pos = 0;
static uint8_t audio_music_tick = 0;
static uint8_t audio_last_sfx = 0;
static uint8_t audio_proof[8];
/* Melody freq bytes: D4 F4 A4 Bb4 A4 G4 F4 E4 D4 F4 A4 C5 Bb4 A4 G4 F4. */
static const uint8_t TITLE_LO[64] = {0x42, 0xFF, 0x89, 0x72, 0x42, 0xFF, 0x27, 0x42, 0x89, 0xB2, 0x89, 0x72, 0x42, 0x27, 0x42, 0xFF, 0xAC, 0xFF, 0xCE, 0xAC, 0x89, 0x72, 0x42, 0x27, 0x42, 0xFF, 0x89, 0xB2, 0xD6, 0xE7, 0xD6, 0xB2, 0x89, 0x72, 0x42, 0xFF, 0x27, 0x42, 0x72, 0x89, 0xB2, 0x89, 0x72, 0x42, 0x27, 0xFF, 0x42, 0xFF, 0x21, 0x14, 0x21, 0xFF, 0xE7, 0xD6, 0xB2, 0x89, 0x72, 0x42, 0x27, 0x42, 0x42, 0xFF, 0x42, 0xFF};
static const uint8_t TITLE_HI[64] = {0xC6, 0x00, 0xC6, 0xC6, 0xC6, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0xC5, 0x00, 0xC5, 0xC5, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0xC6, 0x00, 0xC7, 0xC7, 0xC7, 0x00, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00, 0xC6, 0x00};

static const uint8_t END_LO[3][16] = {
    {0x42, 0xFF, 0x42, 0xFF, 0x89, 0xFF, 0x42, 0xFF, 0xB2, 0xFF, 0x89, 0xFF, 0x42, 0xFF, 0x42, 0xFF},
    {0x42, 0x89, 0xD6, 0x05, 0xD6, 0x89, 0x42, 0x89, 0xD6, 0x05, 0xCE, 0xD6, 0x05, 0xCE, 0xD6, 0x89},
    {0x42, 0x72, 0x89, 0xB2, 0xD6, 0xB2, 0x89, 0x72, 0x42, 0x72, 0x89, 0xB2, 0x89, 0x72, 0x42, 0xFF},
};
static const uint8_t END_HI[3][16] = {
    {0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00, 0xC6, 0x00},
    {0xC6, 0xC6, 0xC6, 0xC7, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC7, 0xC5, 0xC6, 0xC7, 0xC5, 0xC6, 0xC6},
    {0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0xC6, 0x00},
};

static void sfx_step(void) {
    audio_last_sfx = 1;
    NR41_REG = 0x30;
    NR42_REG = 0xC1;
    NR43_REG = 0x60;
    NR44_REG = 0xC0;
}

static void sfx_bump(void) {
    audio_last_sfx = 2;
    NR11_REG = 0x80;
    NR12_REG = 0xF1;
    NR13_REG = 0x00;
    NR14_REG = 0xC6;
}

static void sfx_note(uint8_t lo, uint8_t hi) {
    NR11_REG = 0x80;
    NR12_REG = 0xF3;
    NR13_REG = lo;
    NR14_REG = hi;
}

static void sfx_note2(uint8_t lo, uint8_t hi) {
    NR21_REG = 0x80;
    NR22_REG = 0xF3;
    NR23_REG = lo;
    NR24_REG = hi;
}

/* v5 NON-BLOCKING SFX SEQUENCER.
 * v4's multi-note SFX called wait_vbl_done() in a loop inside the
 * event handler, freezing the whole main loop (and swallowing any
 * button press made during the block) for the SFX duration: pickup
 * 24 frames, gate-unlock+reward 58, exit jingle 80. In PyBoy that
 * measured as Cog pickup 47 ticks vs a plain-step baseline of 25
 * ticks. On a phone, where VBlank delivery can differ, that same
 * block is the only mechanism that can stretch into a long stall,
 * matching the ~10s pickup freeze report from playtesting. v5 therefore NEVER
 * waits inside an SFX handler: multi-note SFX are flat step lists
 * (kind, lo, hi, dur-frames, ... , 0) advanced once per vblank
 * by sfx_step_seq() in the main loop. Single-note SFX (step,
 * bump, bell, lever, rotate) were already instant register
 * writes and stay that way. SFX ids: 16 = statue grind (new). */
static const uint8_t SEQ_PICKUP[] = {4, 0x84, 0xC4, 10, 4, 0xAC, 0xC5, 16, 0};
static const uint8_t SEQ_TALK[] = {1, 0x72, 0xC6, 6, 1, 0x89, 0xC6, 8, 0};
static const uint8_t SEQ_GATE_UNLOCK[] = {1, 0x42, 0xC6, 8, 1, 0x89, 0xC6, 10, 0};
/* v10: SEQ_TRANSITION removed — its three-note rise was the
 * repeat-crossing culprit (see sfx_gate_cross). */
static const uint8_t SEQ_REWARD[] = {1, 0x42, 0xC6, 8, 1, 0x89, 0xC6, 8, 1, 0xD6, 0xC6, 8, 1, 0x05, 0xC7, 16, 0};
static const uint8_t SEQ_GATE_JINGLE[] = {1, 0x42, 0xC6, 8, 1, 0x89, 0xC6, 8, 1, 0xD6, 0xC6, 8, 1, 0x05, 0xC7, 16, 0};
/* Statue-solved grind: low noise rumble steps (kind 3 = noise:
 * lo = NR43, hi = NR42) ending in a low square thud. Stone-on-
 * stone, not the rising ping of SEQ_SOLVED (bells/coffins keep
 * SEQ_SOLVED; only statues grind). */
static const uint8_t SEQ_GRIND[] = {3, 0x30, 0xF1, 8, 3, 0x20, 0xE1, 8, 3, 0x40, 0xD1, 8, 1, 0x82, 0xC3, 10, 0};
static const uint8_t *sfx_seq = 0;
static uint8_t sfx_seq_idx = 0, sfx_seq_tick = 0, sfx_active = 0;

static void sfx_play_step(const uint8_t *seq, uint8_t idx) {
    uint8_t kind = seq[idx], lo = seq[idx + 1], hi = seq[idx + 2];
    if (kind == 1) {
        NR11_REG = 0x80; NR12_REG = 0xF3; NR13_REG = lo; NR14_REG = hi;
    } else if (kind == 4) {
        NR11_REG = 0x80; NR12_REG = 0xA2; NR13_REG = lo; NR14_REG = hi;
    } else if (kind == 2) {
        NR21_REG = 0x80; NR22_REG = 0xF3; NR23_REG = lo; NR24_REG = hi;
    } else if (kind == 3) {
        NR41_REG = 0x20; NR42_REG = hi; NR43_REG = lo; NR44_REG = 0xC0;
    }
}

static void sfx_play_seq(const uint8_t *seq, uint8_t id) {
    audio_last_sfx = id;
    sfx_seq = seq;
    sfx_seq_idx = 0;
    sfx_active = 1;
    sfx_seq_tick = seq[3];
    sfx_play_step(seq, 0);
}

static void sfx_step_seq(void) {
    uint8_t kind;
    if (!sfx_active) return;
    if (sfx_seq_tick) { sfx_seq_tick--; return; }
    sfx_seq_idx = (uint8_t)(sfx_seq_idx + 4);
    kind = sfx_seq[sfx_seq_idx];
    if (kind == 0) {
        sfx_active = 0;
        NR42_REG = 0x00;
        return;
    }
    sfx_seq_tick = sfx_seq[sfx_seq_idx + 3];
    sfx_play_step(sfx_seq, sfx_seq_idx);
}

static void sfx_pickup(void) { sfx_play_seq(SEQ_PICKUP, 3); }
static void sfx_talk(void) { sfx_play_seq(SEQ_TALK, 4); }
static void sfx_grind(void) { sfx_play_seq(SEQ_GRIND, 16); }

static void sfx_lever(void) {
    audio_last_sfx = 5;
    NR41_REG = 0x20;
    NR42_REG = 0xF1;
    NR43_REG = 0x40;
    NR44_REG = 0xC0;
    sfx_note(0x82, 0xC3);
}

static void sfx_bell(uint8_t bell) {
    /* Distinct bell pitches: west low A3, east E4, north A4,
     * slow-decay square so they ring rather than blip. */
    audio_last_sfx = (uint8_t)(6 + bell);
    NR11_REG = 0x80;
    NR12_REG = 0xF2;
    if (bell == 0) { NR13_REG = 0xAC; NR14_REG = 0xC5; }
    else if (bell == 1) { NR13_REG = 0x72; NR14_REG = 0xC6; }
    else { NR13_REG = 0xD6; NR14_REG = 0xC6; }
}

static void sfx_rotate(uint8_t coffin) {
    /* v5: instant only — the old 8-frame wait is gone with the
     * rest of the blocking SFX (see the sequencer above). */
    audio_last_sfx = coffin ? 10 : 9;
    if (coffin) {
        NR41_REG = 0x20;
        NR42_REG = 0xC1;
        NR43_REG = 0x50;
        NR44_REG = 0xC0;
        sfx_note(0x82, 0xC3);
    } else {
        sfx_note(0xB2, 0xC6);
    }
}

static void sfx_gate_unlock(void) { sfx_play_seq(SEQ_GATE_UNLOCK, 11); }
static void sfx_reward(void) { sfx_play_seq(SEQ_REWARD, 13); }
static void sfx_solved(void) { sfx_play_seq(SEQ_GRIND, 16); }
static void sfx_gate_jingle(void) { sfx_play_seq(SEQ_GATE_JINGLE, 17); }
static uint8_t sfx_gate_cross(uint8_t pair) {
    /* v10 GATE SOUND AUDIT: the ONLY gate-crossing sounds in
     * v9 were the first-crossing jingle (17) and, on every
     * repeat, sfx_transition() — the old three-note rise
     * (id 12). That transition was the repeat-crossing sound
     * reported in playtesting: forward, return, crypt, and Market gates
     * all funnel through here, so every repeat played it.
     * Fixed: first crossing plays the jingle and returns 1
     * (banner); every later crossing plays NO gate sound at
     * all — audio_last_sfx is set to 0 so QA can prove the
     * silence — and returns 0. No step/arrival sound is
     * triggered by a gate branch either, so repeat crossings
     * are now seamless normal movement. */
    if (pair < 12 && !gate_jingled[pair]) { gate_jingled[pair] = 1; sfx_gate_jingle(); return 1; }
    audio_last_sfx = 0;
    return 0;
}

static void audio_stop_music(void) {
    audio_music_on = 0;
    audio_music_track = 0;
    sfx_active = 0;
    NR12_REG = 0x00;
    NR22_REG = 0x00;
}

static void audio_music_step(void) {
    uint8_t lo, hi;
    if (!audio_music_on) return;
    if (audio_music_tick) { audio_music_tick--; return; }
    if (audio_music_track == 0) {
        audio_music_tick = 29;
        lo = TITLE_LO[audio_music_pos]; hi = TITLE_HI[audio_music_pos];
        audio_music_pos = (uint8_t)((audio_music_pos + 1) & 63);
    } else {
        uint8_t tr = (uint8_t)(audio_music_track - 1);
        audio_music_tick = tr == 0 ? 34 : (tr == 1 ? 20 : 24);
        lo = END_LO[tr][audio_music_pos]; hi = END_HI[tr][audio_music_pos];
        audio_music_pos = (uint8_t)((audio_music_pos + 1) & 15);
    }
    NR11_REG = 0x80;
    if (lo == 0xFF) {
        NR12_REG = 0x00;
        NR14_REG = 0x00;
    } else {
        NR12_REG = 0xF2;
        NR13_REG = lo;
        NR14_REG = hi;
    }
    if ((audio_music_pos & 3) == 0) {
        NR21_REG = 0x80;
        NR22_REG = 0x80;
        NR23_REG = 0x83;
        NR24_REG = 0xC4;
    }
}

#define ST_TITLE 0
#define ST_PLAY 1
#define ST_EXIT 2
#define ST_INV 3
#define ST_MAP 4
#define ST_TALK 5
#define ST_INTRO 6
#define ST_CODE 7
#define ST_DONE 8
#define ST_CHOICE 9
#define ST_END 10
#define ST_BANNER 11
/* v4: ST_INTRO is the story-intro state (A advances, Start skips).
 * v3: ST_INV (inventory, Start to return) and ST_MAP (minimap,
 * Select or Start to return) are overlay states on top of play;
 * g_state stays 1 through them, and returning to play reloads
 * the area palettes and renders one fresh frame. */

/* v8: 13 bytes — [10]=items bits 24-31 (Bone Seals/Gold
 * live above bit 23 and were invisible to QA in the old
 * 12-byte proof); flags move to [11], checksum to [12]. */
static uint8_t proof_block[13];
static uint8_t key_proof[8];
static uint8_t inv_proof[4];
static uint8_t code_proof[8];
static uint8_t clue_proof[4];
/* v5 intro-over-map proof: 'I','M',1,checksum, set when the intro
 * is entered AFTER the Area 1 map frame has been rendered. */
static uint8_t intro_proof[4];
static int8_t g_ptx = 1, g_pty = 1;
static uint8_t g_ang = 0;

/* ---- Static key billboard: v3 sprite casting, for items ----
 * Same technique as the v3 wraith (Lode Raycasting III, per-column
 * ZBuffer occlusion), but for a SMALL floor-standing item, drawn
 * at TILE coordinates, not pixel coordinates:
 * - transform the key's tile-centre offset into camera space
 *   (fwd = tiles ahead, lat = lateral tiles), screen centre tile
 *   from lat/fwd;
 * - floor-anchored in tile terms: floor_tile = (72 + wall_h/2)/8,
 *   the key's bottom tile is floor_tile-1, so its feet sit on the
 *   floor line at its distance, never floating mid-air and never
 *   sunk into the floor tile;
 * - one tile set PER DISTANCE (gen_tiles.py KEY_SIZES), each
 *   downscaled from the master key art to that distance's exact
 *   tile-aligned size: fwd 1: 16x32 (2x4 tiles), fwd 2: 8x24
 *   (1x3), fwd 3: 8x16 (1x2), fwd 4: 8x8 (1x1). Screen tiles map
 *   1:1 to texture tiles (tex = screen - origin), so NO screen
 *   tile ever repeats a texture tile at any distance. This is the
 *   fix for the v1 overlapping clips: v1 sampled a fixed tile set
 *   per screen tile centre, so at 2 tiles the key spanned 5
 *   screen rows against a 4-row texture and the key shape
 *   duplicated; v2 cannot do that, by construction.
 * Per-column occlusion is unchanged: a column is skipped when
 * key depth >= zbuf[col], so a key behind a tree/wall is hidden.
 * NOT hardware sprites, NOT a fixed-size overlay. */
static int8_t billboard_fwd(int8_t tx, int8_t ty);
/* Generic billboard (keys/statues/NPCs share the v3 sprite-casting
 * technique): tall = NPC tileset + palette 3, small = key tileset +
 * palette 6. Floor-anchored, per-column ZBuffer occlusion. */
/* v3 billboard sets: bank-0 sets (NPC tall, KEY fallback) and bank-1
 * sets (every pickup / interactable kind has its own art, so no
 * lever, bell, note or epitaph ever renders as a key again).
 * A bank-1 billboard's attribute is palette | 8 (VRAM bank bit). */
typedef struct {
    uint16_t base[4];
    uint8_t cols[4];
    uint8_t rows[4];
    const unsigned char *solid[4];
} BSet;
static const BSet NPC_SET = {{NPC_D1_BASE, NPC_D2_BASE, NPC_D3_BASE, NPC_D4_BASE},
    {NPC_D1_COLS, NPC_D2_COLS, NPC_D3_COLS, NPC_D4_COLS},
    {NPC_D1_ROWS, NPC_D2_ROWS, NPC_D3_ROWS, NPC_D4_ROWS},
    {npc_solid_d1, npc_solid_d2, npc_solid_d3, npc_solid_d4}};
static const BSet COG_SET = {{COG_D1_BASE, COG_D2_BASE, COG_D3_BASE, COG_D4_BASE},
    {COG_D1_COLS, COG_D2_COLS, COG_D3_COLS, COG_D4_COLS},
    {COG_D1_ROWS, COG_D2_ROWS, COG_D3_ROWS, COG_D4_ROWS},
    {cog_solid_d1, cog_solid_d2, cog_solid_d3, cog_solid_d4}};
static const BSet PIN_SET = {{PIN_D1_BASE, PIN_D2_BASE, PIN_D3_BASE, PIN_D4_BASE},
    {PIN_D1_COLS, PIN_D2_COLS, PIN_D3_COLS, PIN_D4_COLS},
    {PIN_D1_ROWS, PIN_D2_ROWS, PIN_D3_ROWS, PIN_D4_ROWS},
    {pin_solid_d1, pin_solid_d2, pin_solid_d3, pin_solid_d4}};
static const BSet SEAL_SET = {{SEAL_D1_BASE, SEAL_D2_BASE, SEAL_D3_BASE, SEAL_D4_BASE},
    {SEAL_D1_COLS, SEAL_D2_COLS, SEAL_D3_COLS, SEAL_D4_COLS},
    {SEAL_D1_ROWS, SEAL_D2_ROWS, SEAL_D3_ROWS, SEAL_D4_ROWS},
    {seal_solid_d1, seal_solid_d2, seal_solid_d3, seal_solid_d4}};
static const BSet GRAVE_SET = {{GRAVE_D1_BASE, GRAVE_D2_BASE, GRAVE_D3_BASE, GRAVE_D4_BASE},
    {GRAVE_D1_COLS, GRAVE_D2_COLS, GRAVE_D3_COLS, GRAVE_D4_COLS},
    {GRAVE_D1_ROWS, GRAVE_D2_ROWS, GRAVE_D3_ROWS, GRAVE_D4_ROWS},
    {grave_solid_d1, grave_solid_d2, grave_solid_d3, grave_solid_d4}};
static const BSet SIGIL_SET = {{SIGIL_D1_BASE, SIGIL_D2_BASE, SIGIL_D3_BASE, SIGIL_D4_BASE},
    {SIGIL_D1_COLS, SIGIL_D2_COLS, SIGIL_D3_COLS, SIGIL_D4_COLS},
    {SIGIL_D1_ROWS, SIGIL_D2_ROWS, SIGIL_D3_ROWS, SIGIL_D4_ROWS},
    {sigil_solid_d1, sigil_solid_d2, sigil_solid_d3, sigil_solid_d4}};
static const BSet LANTERN_SET = {{LANTERN_D1_BASE, LANTERN_D2_BASE, LANTERN_D3_BASE, LANTERN_D4_BASE},
    {LANTERN_D1_COLS, LANTERN_D2_COLS, LANTERN_D3_COLS, LANTERN_D4_COLS},
    {LANTERN_D1_ROWS, LANTERN_D2_ROWS, LANTERN_D3_ROWS, LANTERN_D4_ROWS},
    {lantern_solid_d1, lantern_solid_d2, lantern_solid_d3, lantern_solid_d4}};
static const BSet LEVER_SET = {{LEVER_D1_BASE, LEVER_D2_BASE, LEVER_D3_BASE, LEVER_D4_BASE},
    {LEVER_D1_COLS, LEVER_D2_COLS, LEVER_D3_COLS, LEVER_D4_COLS},
    {LEVER_D1_ROWS, LEVER_D2_ROWS, LEVER_D3_ROWS, LEVER_D4_ROWS},
    {lever_solid_d1, lever_solid_d2, lever_solid_d3, lever_solid_d4}};
static const BSet BELL_SET = {{BELL_D1_BASE, BELL_D2_BASE, BELL_D3_BASE, BELL_D4_BASE},
    {BELL_D1_COLS, BELL_D2_COLS, BELL_D3_COLS, BELL_D4_COLS},
    {BELL_D1_ROWS, BELL_D2_ROWS, BELL_D3_ROWS, BELL_D4_ROWS},
    {bell_solid_d1, bell_solid_d2, bell_solid_d3, bell_solid_d4}};
static const BSet NOTE_SET = {{NOTE_D1_BASE, NOTE_D2_BASE, NOTE_D3_BASE, NOTE_D4_BASE},
    {NOTE_D1_COLS, NOTE_D2_COLS, NOTE_D3_COLS, NOTE_D4_COLS},
    {NOTE_D1_ROWS, NOTE_D2_ROWS, NOTE_D3_ROWS, NOTE_D4_ROWS},
    {note_solid_d1, note_solid_d2, note_solid_d3, note_solid_d4}};
static const BSet EPITAPH_SET = {{EPITAPH_D1_BASE, EPITAPH_D2_BASE, EPITAPH_D3_BASE, EPITAPH_D4_BASE},
    {EPITAPH_D1_COLS, EPITAPH_D2_COLS, EPITAPH_D3_COLS, EPITAPH_D4_COLS},
    {EPITAPH_D1_ROWS, EPITAPH_D2_ROWS, EPITAPH_D3_ROWS, EPITAPH_D4_ROWS},
    {epitaph_solid_d1, epitaph_solid_d2, epitaph_solid_d3, epitaph_solid_d4}};
static const BSet MECH_SET = {{MECH_D1_BASE, MECH_D2_BASE, MECH_D3_BASE, MECH_D4_BASE},
    {MECH_D1_COLS, MECH_D2_COLS, MECH_D3_COLS, MECH_D4_COLS},
    {MECH_D1_ROWS, MECH_D2_ROWS, MECH_D3_ROWS, MECH_D4_ROWS},
    {mech_solid_d1, mech_solid_d2, mech_solid_d3, mech_solid_d4}};
static const BSet TOWER_SET = {{TOWER_D1_BASE, TOWER_D2_BASE, TOWER_D3_BASE, TOWER_D4_BASE},
    {TOWER_D1_COLS, TOWER_D2_COLS, TOWER_D3_COLS, TOWER_D4_COLS},
    {TOWER_D1_ROWS, TOWER_D2_ROWS, TOWER_D3_ROWS, TOWER_D4_ROWS},
    {tower_solid_d1, tower_solid_d2, tower_solid_d3, tower_solid_d4}};
static const BSet COFFIN_SET = {{COFFIN_D1_BASE, COFFIN_D2_BASE, COFFIN_D3_BASE, COFFIN_D4_BASE},
    {COFFIN_D1_COLS, COFFIN_D2_COLS, COFFIN_D3_COLS, COFFIN_D4_COLS},
    {COFFIN_D1_ROWS, COFFIN_D2_ROWS, COFFIN_D3_ROWS, COFFIN_D4_ROWS},
    {coffin_solid_d1, coffin_solid_d2, coffin_solid_d3, coffin_solid_d4}};

/* Generic billboard: same sprite-casting placement as before
 * (camera transform, floor-anchored, per-column ZBuffer), with
 * the set chosen by the caller. arrow_face is 255 for no arrow,
 * otherwise a facing value (0 east, 64 south, 128 west, 192
 * north): a small direction arrow tile is drawn just above the
 * figure's head, so a statue/coffin visibly points where it
 * faces even at a glance, on top of the facing dialog. */
static void draw_billboard_set(int8_t ex, int8_t ey, const BSet *set,
        uint8_t pal, uint8_t bank1, uint8_t arrow_face) {
    int8_t dx = 0, dy = 0, relx, rely, fwd, lat;
    int16_t depth, wall_h, floor_tile, centre_tile;
    int8_t col0, col1, row0, row1, c, r;
    uint8_t bg_cols, bg_rows, fi;
    uint16_t bg_base;
    const unsigned char *bg_solid;
    uint8_t attr = bank1 ? (uint8_t)(pal | 8) : pal;
    if (g_state != 1) return;
    if (g_ang == 0) dx = 1;
    else if (g_ang == 64) dy = 1;
    else if (g_ang == 128) dx = -1;
    else dy = -1;
    relx = (int8_t)(ex - g_ptx);
    rely = (int8_t)(ey - g_pty);
    if (relx == 0 && rely == 0) return;
    fwd = (int8_t)(relx * dx + rely * dy);
    lat = (int8_t)(relx * (int8_t)-dy + rely * dx);
    if (fwd <= 0 || fwd > 4 || lat < -fwd || lat > fwd) return;
    fi = (uint8_t)(fwd - 1);
    bg_cols = set->cols[fi];
    bg_rows = set->rows[fi];
    bg_base = set->base[fi];
    bg_solid = set->solid[fi];
    depth = (int16_t)fwd * 256;
    wall_h = (int16_t)(144 / (int16_t)fwd);
    if (wall_h > 128) wall_h = 128;
    floor_tile = (int16_t)((72 + wall_h / 2) / 8);
    row1 = (int8_t)(floor_tile - 1);
    row0 = (int8_t)(row1 - bg_rows + 1);
    centre_tile = (int16_t)((80 + (80 * (int16_t)lat) / (int16_t)fwd) / 8);
    col0 = (int8_t)(centre_tile - bg_cols / 2);
    col1 = (int8_t)(col0 + bg_cols - 1);
    for (c = col0; c <= col1; c++) {
        int8_t tex_col;
        if (c < 0 || c > 19) continue;
        if (depth >= zbuf[c]) continue;
        tex_col = (int8_t)(c - col0);
        if (tex_col < 0 || tex_col >= bg_cols) continue;
        for (r = row0; r <= row1; r++) {
            int8_t tex_row;
            uint8_t tile_idx;
            if (r < 0 || r > 17) continue;
            tex_row = (int8_t)(r - row0);
            if (tex_row < 0 || tex_row >= bg_rows) continue;
            tile_idx = (uint8_t)(tex_row * bg_cols + tex_col);
            if (!bg_solid[tile_idx]) continue;
            buf_cell(c, r, (uint8_t)(bg_base + tile_idx), attr);
        }
    }
    if (arrow_face != 255) {
        uint8_t arrow;
        int8_t ar = (int8_t)(row0 - 1);
        if (arrow_face == 0) arrow = ARROW_E;
        else if (arrow_face == 64) arrow = ARROW_S;
        else if (arrow_face == 128) arrow = ARROW_W;
        else arrow = ARROW_N;
        if (ar >= 0 && centre_tile >= 0 && centre_tile <= 19 && depth < zbuf[centre_tile])
            buf_cell((int8_t)centre_tile, ar, arrow, (uint8_t)(6 | 8));
    }
}

static void draw_billboard_ent(const Ent *e) {
    if (e->kind == ENT_PICKUP) {
        const BSet *set = &COG_SET;
        if (e->aux == IT_PIN) set = &PIN_SET;
        else if (e->aux == IT_AMBER) set = &SEAL_SET; /* amber disc */
        else if (e->aux == IT_GOLD) set = &SEAL_SET; /* gold ingot disc */
        else if (e->aux >= IT_BONE1) set = &GRAVE_SET; /* bone seals */
        else if (e->aux == IT_SEAL) set = &SEAL_SET;
        else if (e->aux >= IT_GRAVE1 && e->aux <= IT_GRAVE3) set = &GRAVE_SET;
        else if (e->aux == IT_SIGIL) set = &SIGIL_SET;
        else if (e->aux == IT_LANTERN) set = &LANTERN_SET;
        draw_billboard_set(e->x, e->y, set, 6, 1, 255);
    } else if (e->kind == ENT_STATUE) {
        draw_billboard_set(e->x, e->y, &NPC_SET, 3, 0, statue_face[e->aux]);
    } else if (e->kind == ENT_COFFIN) {
        /* v5: coffins use their own sarcophagus art (bank 1),
         * never the NPC tall figure again. Arrow + facing
         * dialog retained from v3/v4. */
        draw_billboard_set(e->x, e->y, &COFFIN_SET, 6, 1, coffin_face[e->aux]);
    } else if (e->kind == ENT_NPC) {
        draw_billboard_set(e->x, e->y, &NPC_SET, 3, 0, 255);
    } else if (e->kind == ENT_LEVER) {
        /* v4: a pulled lever visibly toggles (dark/red palette)
         * so the player can tell it was toggled. No levers
         * remain in the v4 slice (reserved for a later area),
         * but the engine support is in place for their return. */
        draw_billboard_set(e->x, e->y, &LEVER_SET, lever_pulled[e->aux] ? 5 : 6, 1, 255);
    } else if (e->kind == ENT_BELL) {
        draw_billboard_set(e->x, e->y, &BELL_SET, 6, 1, 255);
    } else if (e->kind == ENT_NOTE) {
        /* v4: notes render in palette 2 (pale grey parchment),
         * never gold, so no note is mistaken for a pickup. */
        draw_billboard_set(e->x, e->y, &NOTE_SET, 2, 1, 255);
    } else if (e->kind == ENT_EPITAPH) {
        draw_billboard_set(e->x, e->y, &EPITAPH_SET, 6, 1, 255);
    } else if (e->kind == ENT_MECH) {
        /* v8 Keeper: an NPC-style person in a distinct red shade
         * (palette 5), never the coffin/mechanism sprite again. */
        if (g_area == 6 && e->aux == 4) draw_billboard_set(e->x, e->y, &NPC_SET, 5, 0, 255);
        else draw_billboard_set(e->x, e->y, &MECH_SET, 6, 1, 255);
    } else if (e->kind == ENT_TOWER) {
        draw_billboard_set(e->x, e->y, &TOWER_SET, 6, 1, 255);
    } else if (e->kind == ENT_COLUMN) {
        draw_billboard_set(e->x, e->y, &TOWER_SET, 6, 1, 255);
    } else if (e->kind == ENT_RVALVE) {
        draw_billboard_set(e->x, e->y, &LEVER_SET, river_pulled[e->aux] ? 5 : 6, 1, 255);
    } else if (e->kind == ENT_WIND) {
        draw_billboard_set(e->x, e->y, &LEVER_SET, wind_pulled[e->aux] ? 5 : 6, 1, 255);
    } else if (e->kind == ENT_SPINE) {
        draw_billboard_set(e->x, e->y, &TOWER_SET, 6, 1, spine_face[e->aux]);
    } else if (e->kind == ENT_RUNE) {
        /* v10: art stays shared (tile/palette budget is at the
         * 32KB limit — distinct palettes did not fit); the
         * dialog label is the identity cue. */
        draw_billboard_set(e->x, e->y, &EPITAPH_SET, 6, 1, 255);
    } else if (e->kind == ENT_ORGAN) {
        draw_billboard_set(e->x, e->y, &LEVER_SET, organ_pulled[e->aux] ? 5 : 6, 1, 255);
    } else if (e->kind == ENT_THRONE) {
        draw_billboard_set(e->x, e->y, &TOWER_SET, 6, 1, 255);
    } else if (e->kind == ENT_CHAMBER) {
        uint8_t done = e->aux == 0 ? item_held(IT_ORGAN) : item_held(IT_THREAD);
        draw_billboard_set(e->x, e->y, &MECH_SET, done ? 6 : 5, 1, 255);
    }
}

static void draw_all_billboards(void) {
    int8_t f;
    uint8_t i;
    for (f = 4; f >= 1; f--) {
        for (i = 0; i < ENT_COUNT[g_area]; i++) {
            const Ent *e = &ENTS[g_area][i];
            if (e->kind == ENT_PICKUP && item_held(e->aux)) continue;
            if (billboard_fwd(e->x, e->y) != f) continue;
            draw_billboard_ent(e);
        }
    }
}

/* Forward distance (tiles ahead, 1..4) of a billboard target in
 * the player's view cone, or 0 if it is not visible. Used only
 * to order billboard drawing: the farther billboard is drawn
 * first so the nearer one correctly overwrites it where their
 * columns overlap (keys and the NPC are both in the Meadow). */
static int8_t billboard_fwd(int8_t tx, int8_t ty) {
    int8_t dx = 0, dy = 0, relx, rely, fwd, lat;
    if (g_ang == 0) dx = 1;
    else if (g_ang == 64) dy = 1;
    else if (g_ang == 128) dx = -1;
    else dy = -1;
    relx = (int8_t)(tx - g_ptx);
    rely = (int8_t)(ty - g_pty);
    if (relx == 0 && rely == 0) return 0;
    fwd = (int8_t)(relx * dx + rely * dy);
    lat = (int8_t)(relx * (int8_t)-dy + rely * dx);
    if (fwd <= 0 || fwd > 4 || lat < -fwd || lat > fwd) return 0;
    return fwd;
}

/* ---- v4 Meadow NPC billboard: same sprite-casting technique
 * as the keys (camera transform, perspective height, per-column
 * ZBuffer occlusion, floor-anchored, tile-coordinate placement
 * with one tile set per distance so no tile ever repeats), but
 * for a TALLER standing figure: about half wall height at
 * 1 tile (64px vs 128), shrinking with distance. Tile sets
 * (gen_tiles.py NPC_SIZES): fwd 1: 32x64 (4x8 tiles), fwd 2:
 * 16x32 (2x4), fwd 3: 16x24 (2x3), fwd 4: 8x16 (1x2).
 * Palette 3 in the Meadow is the NPC palette (see AREA_PALS:
 * Meadow walls never use palette 3, it was identical to
 * palette 2). The NPC never blocks movement -- its tile is an
 * open map cell, the player walks through it, and the billboard
 * hides when on the player's tile, exactly like the old
 * wraith walk-through rule. Meadow only. */

/* v8 shared sequence puzzle (wind/runes/organs): order array,
 * length, index/done state, optional pulled-visual array, and
 * bell (runes) vs lever (wind/organs) step sound. Dialogs are
 * the shared generic set: 103 step, 104 solved, 105 wrong,
 * 106 already set. */
static void seq_interact(uint8_t *idxp, uint8_t *donep, const uint8_t *order,
        uint8_t len, uint8_t aux, uint8_t *pulled, uint8_t bell) {
    if (*donep) { draw_dialog_lines(106); sfx_talk(); }
    else if (aux == order[*idxp]) {
        if (pulled) pulled[aux] = 1;
        (*idxp)++;
        if (*idxp >= len) { *donep = 1; draw_dialog_lines(104); }
        else draw_dialog_lines(103);
        if (bell) sfx_bell((uint8_t)(aux % 3));
        else sfx_lever();
        if (*donep) sfx_solved();
    } else {
        *idxp = 0;
        if (pulled) { pulled[0] = 0; pulled[1] = 0; pulled[2] = 0; }
        draw_dialog_lines(105);
        if (bell) sfx_bell((uint8_t)(aux % 3));
        else sfx_lever();
    }
}

static void render_one_frame(void) {
    int16_t px = (int16_t)g_ptx * 256 + 128;
    int16_t py = (int16_t)g_pty * 256 + 128;
    DISPLAY_OFF;
    invalidate_prev();
    compose_raycaster(px, py, g_ang);
    draw_all_billboards();
    scan_diff();
    write_list_all();
    sync_prev_cur();
    ch_count = 0;
    DISPLAY_ON;
}

void main(void) {
    uint8_t state = ST_TITLE;
    uint8_t prev_joy = 0, joy, held;
    uint8_t bump_timer = 0;
    uint8_t idle_frames = 0;
    uint8_t c;

    cpu_fast();
    SCX_REG = 0;
    set_bkg_data(0, TILE_COUNT, tiles);
    VBK_REG = 1;
    set_bkg_data(0, EXTRA_TILE_COUNT, extra_tiles);
    VBK_REG = 0;
    load_area_palettes(0);
    hide_all_sprites();
    NR52_REG = 0x80;
    NR51_REG = 0xFF;
    NR50_REG = 0x77;
    SHOW_BKG;
    SHOW_SPRITES;
    DISPLAY_ON;

    for (c = 0; c < 20; c++)
        cam_x[c] = (int16_t)(((int16_t)(2 * c - 19) * 256) / 19);

    draw_title();

    while (1) {
        uint8_t pressed, dir = 0;
        wait_vbl_done();
        audio_music_step();
        sfx_step_seq();
        if (ch_count) write_list_chunked();
        joy = joypad();
        held = joy;
        pressed = (uint8_t)(joy & (uint8_t)~prev_joy);
        prev_joy = joy;
        if (bump_timer) bump_timer--;

        if (state == ST_TITLE) {
            if (pressed & J_START) {
                audio_stop_music();
                g_ptx = 1; g_pty = 1; g_ang = 0;
                g_area = 0; g_items = 0;
                statue_face[0] = 192; statue_face[1] = 128; statue_face[2] = 64;
                coffin_face[0] = 0; coffin_face[1] = 192;
                lever_idx = 0; lever_done = 0; bell_idx = 0; bell_done = 0;
                lever_pulled[0] = 0; lever_pulled[1] = 0; lever_pulled[2] = 0;
                epitaph_read = 0; slice_done = 0;
                bone_idx = 0; bone_done = 0; flow_idx = 0; flow_done = 0;
                river_pulled[0] = 0; river_pulled[1] = 0; river_pulled[2] = 0;
                merchant_clues = 0; trader_done = 0; crypt_open = 0;
                wind_idx = 0; wind_done = 0; wind_pulled[0] = 0; wind_pulled[1] = 0; wind_pulled[2] = 0;
                mem_idx = 0; mem_done = 0; organ_idx = 0; organ_done = 0;
                organ_pulled[0] = 0; organ_pulled[1] = 0; organ_pulled[2] = 0;
                spine_face[0] = 0; spine_face[1] = 64; spine_face[2] = 128;
                choice_idx = 0; ending_id = 0;
                for (c = 0; c < 12; c++) gate_jingled[c] = 0;
                inv_page = 0; code_digit[0] = 0; code_digit[1] = 0; code_digit[2] = 0;
                code_cursor = 0; code_wrong = 0;
                clear_visited();
                mark_visited(0, 1, 1);
                idle_frames = 0;
                /* v5 intro timing: render the Area 1 map FIRST,
                 * then show the intro dialogs over the map's
                 * bottom half — never over the title screen.
                 * Title -> map visible -> intro dialogs -> play. */
                load_area_palettes(0);
                g_state = 1;
                render_one_frame();
                intro_proof[0] = 'I'; intro_proof[1] = 'M';
                intro_proof[2] = 1;
                intro_proof[3] = (uint8_t)('I' + 'M' + 1);
                intro_idx = 0;
                state = ST_INTRO;
                draw_intro_lines(33);
            }
        } else if (state == ST_CHOICE) {
            if (pressed & J_START) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            } else if (pressed & J_LEFT) {
                choice_idx = (uint8_t)((choice_idx + 2) % 3);
                draw_choice();
            } else if (pressed & J_RIGHT) {
                choice_idx = (uint8_t)((choice_idx + 1) % 3);
                draw_choice();
            } else if (pressed & J_A) {
                state = ST_END;
                g_state = 10;
                ch_count = 0;
                draw_ending(choice_idx);
            }
        } else if (state == ST_END) {
            if (pressed & J_START) {
                audio_stop_music();
                state = ST_TITLE;
                g_state = 0;
                ch_count = 0;
                draw_title();
            }
        } else if (state == ST_INV) {
            if (pressed & J_START) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            } else if (pressed & J_LEFT) {
                inv_page = (uint8_t)((inv_page + 3) % 4);
                draw_inventory();
            } else if (pressed & J_RIGHT) {
                inv_page = (uint8_t)((inv_page + 1) % 4);
                draw_inventory();
            }
        } else if (state == ST_MAP) {
            if ((pressed & J_SELECT) || (pressed & J_START)) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            }
        } else if (state == ST_TALK) {
            /* v4 dialog: ONLY A (again) or Start closes it.
             * Movement, turns, gates, inventory and minimap
             * are all ignored while dialog is open. Closing
             * restores the full view with one frame render;
             * no palette was changed by the dialog, so no
             * palette reload is needed. */
            if ((pressed & J_A) || (pressed & J_START)) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                render_one_frame();
            }
        } else if (state == ST_CODE) {
            /* v6 crypt code entry: Left/Right select digit,
             * Up/Down change it (wraps), A confirms (advances,
             * submits on the third digit), Start cancels. */
            if (pressed & J_START) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            } else if (pressed & J_LEFT) {
                code_cursor = (uint8_t)((code_cursor + 2) % 3);
                draw_code();
            } else if (pressed & J_RIGHT) {
                code_cursor = (uint8_t)((code_cursor + 1) % 3);
                draw_code();
            } else if (pressed & J_UP) {
                code_digit[code_cursor] = (uint8_t)((code_digit[code_cursor] + 1) % 10);
                code_wrong = 0;
                draw_code();
            } else if (pressed & J_DOWN) {
                code_digit[code_cursor] = (uint8_t)((code_digit[code_cursor] + 9) % 10);
                code_wrong = 0;
                draw_code();
            } else if (pressed & J_A) {
                if (code_cursor < 2) {
                    code_cursor++;
                    draw_code();
                } else if (code_digit[0] == 7 && code_digit[1] == 3 && code_digit[2] == 9) {
                    g_items = (uint32_t)(g_items | (1 << IT_FRAGMENT));
                    crypt_open = 1;
                    state = ST_TALK;
                    ch_count = 0;
                    draw_dialog_lines(65);
                    sfx_gate_unlock(); sfx_reward();
                } else {
                    code_digit[0] = 0; code_digit[1] = 0; code_digit[2] = 0;
                    code_cursor = 0; code_wrong = 1;
                    sfx_bump();
                    draw_code();
                }
            }
        } else if (state == ST_INTRO) {
            /* v4 intro sequence: A advances, Start skips the
             * rest. On the last dialog, A begins play. */
            if (pressed & J_START) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            } else if (pressed & J_A) {
                if (intro_idx < 4) {
                    intro_idx++;
                    draw_intro_lines((uint8_t)(33 + intro_idx));
                } else {
                    state = ST_PLAY;
                    g_state = 1;
                    idle_frames = 0;
                    load_area_palettes(g_area);
                    render_one_frame();
                }
            }
        } else if (state == ST_BANNER) {
            /* v9 banner: A dismisses early; otherwise the
             * 180-vblank bump_timer (decremented at the loop
             * top) auto-hides it. Movement/input is ignored
             * while the banner is up; closing re-renders. */
            if ((pressed & J_A) || bump_timer == 0) {
                state = ST_PLAY;
                g_state = 1;
                idle_frames = 0;
                load_area_palettes(g_area);
                render_one_frame();
            }
        } else if (pressed & J_START) {
            /* v3 inventory: edge-triggered Start in play only;
             * it moves nothing and starts nothing else. */
            state = ST_INV;
            ch_count = 0;
            inv_page = 0;
            draw_inventory();
        } else if (pressed & J_SELECT) {
            /* v3 minimap: edge-triggered Select in play only. */
            state = ST_MAP;
            ch_count = 0;
            draw_minimap();
        } else if (pressed & J_A) {
            /* A: interact with the entity on the facing tile, if any.
             * Statues/coffins rotate +64 (east/south targets);
             * levers/bells advance their sequences (wrong = reset);
             * notes/NPCs show dialog; mechanisms check prerequisites
             * and grant their item; epitaph needs the Ash Lantern. */
            int8_t fdx = 0, fdy = 0, ei;
            const Ent *e;
            if (g_ang == 0) fdx = 1;
            else if (g_ang == 64) fdy = 1;
            else if (g_ang == 128) fdx = -1;
            else fdy = -1;
            ei = find_ent(g_area, (int8_t)(g_ptx + fdx), (int8_t)(g_pty + fdy));
            if (ei >= 0) {
                e = &ENTS[g_area][ei];
                state = ST_TALK;
                ch_count = 0;
                if (e->kind == ENT_NPC || e->kind == ENT_NOTE) {
                    /* v7 Market trader (east merchant, aux 70):
                     * Amber -> Pale Idol, a real item trade.
                     * Without Amber: refusal (dialog 82). With
                     * Amber: Amber is removed, Idol is added,
                     * trader_done set (dialog 83 + trade SFX).
                     * After the trade: dialog 85. */
                    if (e->kind == ENT_NPC && g_area == 6 && e->aux == 74) {
                        /* v8 backtrack B: Gold Ingot (Bone Labyrinth)
                         * buys the Market Key from this merchant. */
                        if (item_held(IT_MARKETKEY)) { draw_dialog_lines(76); sfx_talk(); }
                        else if (item_held(IT_GOLD)) {
                            g_items = (uint32_t)(g_items & (uint32_t)~(1UL << IT_GOLD));
                            g_items = (uint32_t)(g_items | (1UL << IT_MARKETKEY));
                            draw_dialog_lines(75); sfx_gate_unlock(); sfx_reward();
                        } else { draw_dialog_lines(74); sfx_talk(); }
                    } else if (e->kind == ENT_NPC && g_area == 6 && e->aux == 61) {
                        if (trader_done) { draw_dialog_lines(71); sfx_talk(); }
                        else if (item_held(IT_AMBER)) {
                            g_items = (uint32_t)(g_items & (uint32_t)~(1UL << IT_AMBER));
                            g_items = (uint32_t)(g_items | (1 << IT_IDOL));
                            trader_done = 1;
                            draw_dialog_lines(69); sfx_gate_unlock(); sfx_reward();
                        } else { draw_dialog_lines(68); sfx_talk(); }
                    } else {
                        draw_dialog_lines(e->aux);
                        sfx_talk();
                    }
                    /* v6 Market: talking to a merchant records
                     * that merchant's clue (dialogs 69-71). In
                     * v7 the clues point to the trader; they no
                     * longer gate the Keeper trade. */
                    if (e->kind == ENT_NPC && g_area == 6 && e->aux >= 60 && e->aux <= 62)
                        merchant_clues = (uint8_t)(merchant_clues | (1 << (e->aux - 60)));
                } else if (e->kind == ENT_STATUE) {
                    statue_face[e->aux] = (uint8_t)(statue_face[e->aux] + 64);
                    draw_dialog_lines(statues_solved() ? 5 : (uint8_t)(25 + (statue_face[e->aux] >> 6)));
                    /* v5: statue completion grinds like stone, it
                     * does NOT play the rising ping. Coffins and
                     * bells keep SEQ_SOLVED; only statues grind. */
                    if (statues_solved()) sfx_grind();
                    else sfx_rotate(0);
                } else if (e->kind == ENT_COFFIN) {
                    coffin_face[e->aux] = (uint8_t)(coffin_face[e->aux] + 64);
                    draw_dialog_lines(coffins_solved() ? 19 : (uint8_t)(29 + (coffin_face[e->aux] >> 6)));
                    sfx_rotate(1);
                    if (coffins_solved()) sfx_solved();
                } else if (e->kind == ENT_LEVER) {
                    /* v6 Forest valves: order north, west, east. */
                    if (lever_done) { draw_dialog_lines(66); sfx_talk(); }
                    else if (e->aux == LEVER_ORDER[lever_idx]) {
                        lever_pulled[e->aux] = 1;
                        lever_idx++;
                        if (lever_idx >= 3) { lever_done = 1; draw_dialog_lines(67); }
                        else draw_dialog_lines(54);
                        sfx_lever();
                        if (lever_done) sfx_solved();
                    } else { lever_idx = 0; draw_dialog_lines(55); sfx_lever(); }
                } else if (e->kind == ENT_COLUMN) {
                    /* v6 Cathedral bone columns: order north
                     * (high), west (low), east (middle). */
                    if (bone_done) { draw_dialog_lines(48); sfx_talk(); }
                    else if (e->aux == BONE_ORDER[bone_idx]) {
                        bone_idx++;
                        if (bone_idx >= 3) { bone_done = 1; draw_dialog_lines(48); }
                        else draw_dialog_lines(46);
                        sfx_bell(e->aux);
                        if (bone_done) sfx_solved();
                    } else { bone_idx = 0; draw_dialog_lines(47); sfx_bell(e->aux); }
                } else if (e->kind == ENT_RVALVE) {
                    /* v6 River valves: order west, east, north. */
                    if (flow_done) { draw_dialog_lines(56); sfx_talk(); }
                    else if (e->aux == FLOW_ORDER[flow_idx]) {
                        river_pulled[e->aux] = 1;
                        flow_idx++;
                        if (flow_idx >= 3) { flow_done = 1; draw_dialog_lines(56); }
                        else draw_dialog_lines(54);
                        sfx_lever();
                        if (flow_done) sfx_solved();
                    } else { flow_idx = 0; draw_dialog_lines(55); sfx_lever(); }
                } else if (e->kind == ENT_BELL) {
                    if (bell_done) { draw_dialog_lines(13); sfx_talk(); }
                    else if (e->aux == BELL_ORDER[bell_idx]) {
                        bell_idx++;
                        if (bell_idx >= 3) { bell_done = 1; draw_dialog_lines(12); }
                        else draw_dialog_lines(10);
                        sfx_bell(e->aux);
                        if (bell_done) sfx_solved();
                    } else { bell_idx = 0; draw_dialog_lines(11); sfx_bell(e->aux); }
                } else if (e->kind == ENT_WIND) {
                    seq_interact(&wind_idx, &wind_done, WIND_ORDER, 3, e->aux, wind_pulled, 0);
                } else if (e->kind == ENT_SPINE) {
                    spine_face[e->aux] = (uint8_t)(spine_face[e->aux] + 64);
                    draw_dialog_lines(spine_solved() ? 104 : 103);
                    if (spine_solved()) sfx_solved();
                    else sfx_rotate(0);
                } else if (e->kind == ENT_RUNE) {
                    /* v10: same sequence logic as seq_interact,
                     * but every interaction names its rune. */
                    if (mem_done) { draw_rune_dialog(e->aux, 3); sfx_talk(); }
                    else if (e->aux == MEM_ORDER[mem_idx]) {
                        mem_idx++;
                        if (mem_idx >= 4) { mem_done = 1; draw_rune_dialog(e->aux, 1); sfx_bell((uint8_t)(e->aux % 3)); sfx_solved(); }
                        else { draw_rune_dialog(e->aux, 0); sfx_bell((uint8_t)(e->aux % 3)); }
                    } else { mem_idx = 0; draw_rune_dialog(e->aux, 2); sfx_bell((uint8_t)(e->aux % 3)); }
                } else if (e->kind == ENT_ORGAN) {
                    seq_interact(&organ_idx, &organ_done, ORGAN_ORDER, 3, e->aux, organ_pulled, 0);
                } else if (e->kind == ENT_CHAMBER) {
                    if (e->aux == 0) {
                        if (!item_held(IT_FORK)) { draw_dialog_lines(72); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1UL << IT_ORGAN)); draw_dialog_lines(73); sfx_gate_unlock(); sfx_reward(); }
                    } else {
                        if (!item_held(IT_LUNG)) { draw_dialog_lines(77); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1UL << IT_THREAD)); draw_dialog_lines(78); sfx_gate_unlock(); sfx_reward(); }
                    }
                } else if (e->kind == ENT_THRONE) {
                    if (!item_held(IT_NINTH)) { draw_dialog_lines(101); sfx_talk(); }
                    else {
                        state = ST_CHOICE;
                        g_state = 9;
                        choice_idx = 0;
                        ch_count = 0;
                        draw_choice();
                    }
                } else if (e->kind == ENT_MECH) {
                    if (e->aux == 0) {
                        if (item_held(IT_SIGIL)) { draw_dialog_lines(23); sfx_talk(); }
                        else if (!item_held(IT_COG) || !item_held(IT_PIN) || !item_held(IT_SEAL)) { draw_dialog_lines(7); sfx_talk(); }
                        else if (!statues_solved()) { draw_dialog_lines(8); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1 << IT_SIGIL)); draw_dialog_lines(9); sfx_gate_unlock(); sfx_reward(); }
                    } else if (e->aux == 1) {
                        if (item_held(IT_BLOOD)) { draw_dialog_lines(42); sfx_talk(); }
                        else if (!lever_done) { draw_dialog_lines(41); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1 << IT_BLOOD)); draw_dialog_lines(43); sfx_gate_unlock(); sfx_reward(); }
                    } else if (e->aux == 2) {
                        if (item_held(IT_FORK)) { draw_dialog_lines(51); sfx_talk(); }
                        else if (!bone_done) { draw_dialog_lines(49); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1 << IT_FORK)); draw_dialog_lines(50); sfx_gate_unlock(); sfx_reward(); }
                    } else if (e->aux == 3) {
                        if (item_held(IT_RIVER)) { draw_dialog_lines(59); sfx_talk(); }
                        else if (!flow_done) { draw_dialog_lines(57); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1 << IT_RIVER)); draw_dialog_lines(58); sfx_gate_unlock(); sfx_reward(); }
                    } else if (e->aux == 4) {
                        /* v7 Market keeper: a real item trade.
                         * Clues alone (v6 path) no longer grant
                         * the Eye Lens. Without the Pale Idol:
                         * refusal (dialog 84). With the Idol:
                         * Idol is removed, Eye Lens is added,
                         * and the game ends (pending_done
                         * turns the dialog close into ST_DONE). */
                        if (item_held(IT_LENS)) { draw_dialog_lines(64); sfx_talk(); }
                        else if (!item_held(IT_IDOL)) { draw_dialog_lines(70); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items & (uint32_t)~(1UL << IT_IDOL)); g_items = (uint32_t)(g_items | (1 << IT_LENS)); draw_dialog_lines(63); sfx_gate_unlock(); sfx_reward(); }
                    } else if (e->aux >= 5) {
                        /* v8 mechanisms 5-9 (table-driven): item,
                         * locked / reward / already dialogs. Aux 8
                         * has a second locked dialog (122) when
                         * the organs are synced but the Vein
                         * Organ is missing. */
                        static const uint8_t MECH_ITEM[5] = {IT_LUNG, IT_SILVER, IT_HEARTSEAL, IT_HEARTKEY, IT_NINTH};
                        static const uint8_t MECH_LOCK[5] = {81, 86, 92, 96, 99};
                        static const uint8_t MECH_REW[5] = {83, 88, 93, 98, 100};
                        static const uint8_t MECH_ALR[5] = {82, 87, 87, 87, 100};
                        uint8_t mi = (uint8_t)(e->aux - 5);
                        uint8_t ok = 0;
                        if (e->aux == 5) ok = wind_done;
                        else if (e->aux == 6) ok = bone_seals_all() && spine_solved();
                        else if (e->aux == 7) ok = mem_done;
                        else if (e->aux == 8) ok = organ_done && item_held(IT_ORGAN);
                        else ok = item_held(IT_SIGIL) && item_held(IT_SILVER) && item_held(IT_HEARTSEAL) && item_held(IT_FRAGMENT) && item_held(IT_RIVER) && item_held(IT_THREAD);
                        if (item_held(MECH_ITEM[mi])) { draw_dialog_lines(MECH_ALR[mi]); sfx_talk(); }
                        else if (!ok) { draw_dialog_lines((e->aux == 8 && organ_done) ? 97 : MECH_LOCK[mi]); sfx_talk(); }
                        else { g_items = (uint32_t)(g_items | (1UL << MECH_ITEM[mi])); draw_dialog_lines(MECH_REW[mi]); sfx_gate_unlock(); sfx_reward(); }
                    }
                } else if (e->kind == ENT_TOWER) {
                    if (item_held(IT_LANTERN)) { draw_dialog_lines(16); sfx_talk(); }
                    else if (!bell_done) { draw_dialog_lines(14); sfx_talk(); }
                    else { g_items = (uint32_t)(g_items | (1 << IT_LANTERN)); draw_dialog_lines(15); sfx_gate_unlock(); sfx_reward(); }
                } else if (e->kind == ENT_EPITAPH) {
                    if (!item_held(IT_LANTERN)) { draw_dialog_lines(17); sfx_talk(); }
                    else { epitaph_read = 1; draw_dialog_lines(18); sfx_talk(); }
                }
            }
        } else {
            if (pressed & J_UP) dir = 1;
            else if (pressed & J_DOWN) dir = 2;
            else if (pressed & J_LEFT) dir = 3;
            else if (pressed & J_RIGHT) dir = 4;
            else if (idle_frames >= 20) {
                if (held & J_UP) dir = 1;
                else if (held & J_DOWN) dir = 2;
                else if (held & J_LEFT) dir = 3;
                else if (held & J_RIGHT) dir = 4;
            }
            if (dir == 1 || dir == 2) {
                int8_t dx = 0, dy = 0, tx, ty;
                uint8_t cell;
                if (g_ang == 0) dx = 1;
                else if (g_ang == 64) dy = 1;
                else if (g_ang == 128) dx = -1;
                else dy = -1;
                if (dir == 2) { dx = (int8_t)-dx; dy = (int8_t)-dy; }
                tx = (int8_t)(g_ptx + dx);
                ty = (int8_t)(g_pty + dy);
                cell = cell_at(g_area, tx, ty);
                if (cell == 0) {
                    g_ptx = tx; g_pty = ty;
                    mark_visited(g_area, tx, ty);
                    { int8_t pei = find_ent(g_area, tx, ty);
                      if (pei >= 0 && ENTS[g_area][pei].kind == ENT_PICKUP
                          && !item_held(ENTS[g_area][pei].aux)) {
                        g_items = (uint32_t)(g_items | (1UL << ENTS[g_area][pei].aux));
                        render_one_frame();
                        sfx_pickup();
                      } else {
                        sfx_step();
                        render_one_frame();
                      }
                    }
                } else if (cell == 'G') {
                    if (gate_held(g_area)) {
                        /* v5 FACING-PRESERVING seamless
                         * transition (general rule, all gates):
                         * destination = destination gate + the
                         * step direction D, facing UNCHANGED.
                         * Forward gate: destination gate is
                         * the next area's return gate (1,1).
                         * E.g. east onto (10,10) -> (2,1)
                         * facing east (forward step, facing
                         * == travel, gate behind); west-facing
                         * Down (back east) onto (10,10) ->
                         * (2,1) facing WEST (backward step,
                         * gate ahead). The source gate tile
                         * itself counts as visited (stepped
                         * onto it) for fog of war. */
                        int8_t dgx = RET_POS[g_area][0];
                        int8_t dgy = RET_POS[g_area][1];
                        mark_visited(g_area, tx, ty);
                        uint8_t pair = g_area;
                        g_area++;
                        g_ptx = (int8_t)(dgx + dx);
                        g_pty = (int8_t)(dgy + dy);
                        /* g_ang deliberately untouched: v5
                         * preserves facing, it does NOT set
                         * facing = travel (that was the v4
                         * backward-step spin bug). */
                        mark_visited(g_area, g_ptx, g_pty);
                        idle_frames = 0;
                        state = ST_PLAY;
                        g_state = 1;
                        load_area_palettes(g_area);
                        /* v9: banner only on FIRST forward
                         * crossing; returns never banner. */
                        if (sfx_gate_cross(pair)) {
                            render_one_frame();
                            state = ST_BANNER;
                            bump_timer = 180;
                            draw_banner(g_area);
                        } else render_one_frame();
                    } else {
                        if (bump_timer == 0) {
                            sfx_bump();
                            bump_timer = 20;
                        }
                    }
                } else if (cell == 'B') {
                    /* v5 FACING-PRESERVING seamless backtrack
                     * (same general rule as forward gates):
                     * destination = previous area's forward
                     * gate + the step direction D, facing
                     * UNCHANGED. Return gates are always open
                     * (arrival already unlocked that
                     * direction), never block, never need a
                     * key, and render gold like any unlocked
                     * gate. E.g. west-facing Up (forward
                     * west) onto (1,1) -> (9,10) facing WEST
                     * in the Dungeon (gate (10,10) behind, to
                     * the east), so Down (back east) returns
                     * through it; east-facing Down (back
                     * west) onto (1,1) -> (9,10) facing EAST
                     * (gate ahead, to the east), so Up
                     * (forward east) returns through it.
                     * The source return-gate tile counts as
                     * visited. */
                    uint8_t da = (uint8_t)(g_area - 1);
                    int8_t dgx = GATE_POS[da][0];
                    int8_t dgy = GATE_POS[da][1];
                    mark_visited(g_area, tx, ty);
                    g_area = da;
                    g_ptx = (int8_t)(dgx + dx);
                    g_pty = (int8_t)(dgy + dy);
                    /* g_ang deliberately untouched (v5). */
                    mark_visited(g_area, g_ptx, g_pty);
                    idle_frames = 0;
                    state = ST_PLAY;
                    g_state = 1;
                    load_area_palettes(g_area);
                    sfx_gate_cross(da);
                    render_one_frame();
                } else if (cell == 3) {
                    /* v6 crypt exit: needs all 3 Grave Seals,
                     * coffins facing the pulse, and the epitaph
                     * read. With those met, stepping onto it
                     * does NOT walk out: it opens the code
                     * entry screen (ST_CODE), and the player
                     * stays on (10,9). Only a correct 7-3-9
                     * grants the Fragment and opens the crypt;
                     * after that, stepping onto it crosses
                     * into the Forest of Veins (facing kept,
                     * destination = Forest return gate + D). */
                    if (!item_held(IT_GRAVE1) || !item_held(IT_GRAVE2) || !item_held(IT_GRAVE3)) {
                        state = ST_TALK; ch_count = 0; draw_dialog_lines(20);
                    } else if (!coffins_solved()) {
                        state = ST_TALK; ch_count = 0; draw_dialog_lines(21);
                    } else if (!epitaph_read) {
                        state = ST_TALK; ch_count = 0; draw_dialog_lines(22);
                    } else if (!crypt_open) {
                        code_digit[0] = 0; code_digit[1] = 0; code_digit[2] = 0;
                        code_cursor = 0; code_wrong = 0;
                        state = ST_CODE;
                        g_state = 7;
                        ch_count = 0;
                        draw_code();
                    } else {
                        mark_visited(g_area, tx, ty);
                        g_area = 3;
                        g_ptx = (int8_t)(1 + dx);
                        g_pty = (int8_t)(1 + dy);
                        mark_visited(g_area, g_ptx, g_pty);
                        idle_frames = 0;
                        state = ST_PLAY;
                        g_state = 1;
                        load_area_palettes(g_area);
                        /* v9: crypt is a forward gate;
                         * banner only on its first crossing. */
                        if (sfx_gate_cross(11)) {
                            render_one_frame();
                            state = ST_BANNER;
                            bump_timer = 180;
                            draw_banner(g_area);
                        } else render_one_frame();
                    }
                } else {
                    if (bump_timer == 0) {
                        sfx_bump();
                        bump_timer = 20;
                    }
                }
                idle_frames = 0;
            } else if (dir == 3 || dir == 4) {
                if (dir == 4) g_ang = (uint8_t)(g_ang + 64);
                else g_ang = (uint8_t)(g_ang - 64);
                sfx_step();
                render_one_frame();
                idle_frames = 0;
            } else {
                if (held) idle_frames++;
                else idle_frames = 0;
            }
        }
        proof_block[0] = 'N'; proof_block[1] = 'W';
        proof_block[2] = state;
        proof_block[3] = g_area;
        proof_block[4] = (uint8_t)g_ptx;
        proof_block[5] = (uint8_t)g_pty;
        proof_block[6] = g_ang;
        proof_block[7] = (uint8_t)(g_items & 0xFF);
        proof_block[8] = (uint8_t)((g_items >> 8) & 0xFF);
        proof_block[9] = (uint8_t)((g_items >> 16) & 0xFF);
        /* v6 flags: 0 statues, 1 Forest valves, 2 bells,
         * 3 coffins, 4 epitaph, 5 crypt open, 6 bone columns,
         * 7 river valves. */
        proof_block[10] = (uint8_t)((g_items >> 24) & 0xFF);
        proof_block[11] = (uint8_t)((statues_solved() ? 1 : 0) | (lever_done ? 2 : 0)
            | (bell_done ? 4 : 0) | (coffins_solved() ? 8 : 0)
            | (epitaph_read ? 16 : 0) | (crypt_open ? 32 : 0)
            | (bone_done ? 64 : 0) | (flow_done ? 128 : 0));
        proof_block[12] = (uint8_t)((state + g_area + (uint8_t)g_ptx + (uint8_t)g_pty + g_ang
            + proof_block[7] + proof_block[8] + proof_block[9] + proof_block[10] + proof_block[11]) & 0xFF);
        /* v2 audio proof: 'A','U',music_on,last_sfx,music_pos,0,0xA5,checksum. */
        audio_proof[0] = 'A'; audio_proof[1] = 'U';
        audio_proof[2] = audio_music_on; audio_proof[3] = audio_last_sfx;
        audio_proof[4] = audio_music_pos; audio_proof[5] = audio_music_track; audio_proof[6] = 0xA5;
        audio_proof[7] = (uint8_t)((audio_music_on + audio_last_sfx + audio_music_pos + audio_music_track) & 0xFF);
        /* v8: legacy key_proof update removed to save ROM (unused by QA). */
        keeper_proof[0] = 'K'; keeper_proof[1] = 'S'; keeper_proof[2] = 1;
        keeper_proof[3] = (uint8_t)('K' + 'S' + 1);
        choice_proof[0] = 'C'; choice_proof[1] = 'H'; choice_proof[2] = choice_idx;
        choice_proof[3] = (uint8_t)('C' + 'H' + choice_idx);
        /* v6 inventory proof: 'I','P',page,checksum. */
        inv_proof[0] = 'I'; inv_proof[1] = 'P'; inv_proof[2] = inv_page;
        inv_proof[3] = (uint8_t)('I' + 'P' + inv_page);
        /* v6 code proof: 'C','E',d0,d1,d2,cursor,wrong,checksum. */
        code_proof[0] = 'C'; code_proof[1] = 'E';
        code_proof[2] = code_digit[0]; code_proof[3] = code_digit[1];
        code_proof[4] = code_digit[2]; code_proof[5] = code_cursor;
        code_proof[6] = code_wrong;
        code_proof[7] = (uint8_t)('C' + 'E' + code_digit[0] + code_digit[1] + code_digit[2] + code_cursor + code_wrong);
        /* v6 merchant-clue proof: 'M','C',clues,checksum. */
        clue_proof[0] = 'M'; clue_proof[1] = 'C'; clue_proof[2] = merchant_clues;
        clue_proof[3] = (uint8_t)('M' + 'C' + merchant_clues);
    }
}
