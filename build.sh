#!/bin/bash
# Build THE NINTH WOUND v8 GBC (Areas 1-12, complete). Produces releases/ninth-wound.gbc
# v8 stays PLAIN 32KB (cart type 0x00, CGB flag 0x80), no MBC5: the
# 12 areas fit because the 256-entry trig tables (1024 bytes, of
# which the raycaster only ever used the 4 cardinal entries)
# were replaced by direct cardinal values, plus shared palettes
# for Lung/Memory/Heart and trimmed dialog strings.
set -e
cd "$(dirname "$0")"
python3 gen_tiles.py
GBDK="${GBDK:-$HOME/workspace/gbc/tools/gbdk}"
mkdir -p releases
"$GBDK/bin/lcc" -Wm-yc -Wm-yn"NINTHWOUND" -Wl-m -o releases/ninth-wound.gbc src/main.c
ls -la releases/
python3 - <<'PYEOF'
d = open("releases/ninth-wound.gbc", "rb").read()
print("size:", len(d), "bytes; cart type: 0x%02X" % d[0x147], "; CGB flag: 0x%02X" % d[0x143])
assert d[0x143] == 0x80, "expected CGB-compatible flag"
assert d[0x147] == 0x00, "expected plain 32KB cart type 0x00"
assert len(d) == 32768, "expected 32KB ROM"
print("header OK: plain 32KB, CGB")
PYEOF
