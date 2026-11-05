"""The Ninth Wound v10 PyBoy QA (Areas 1-12, complete + area banners;
v10: silent repeat gate crossings, Memory Vault rune labels, Vein Organ hints).
Tap = button_press + 2 ticks + release + settle (holds, NOT pulses).
Proof in WRAM: 'N','W',state,area,ptx,pty,ang,items_lo,items_hi,flags,
checksum(state+area+ptx+pty+ang+items_lo+items_hi+flags).
States: 0 title, 1 play, 2 slice-complete, 3 inventory, 4 minimap, 5 dialog, 6 intro, 11 banner.
Flags bits: 0 statues, 1 levers (unused in v4, always 0), 2 bells, 3 coffins, 4 epitaph, 5 slice.
Items bits: 0 Cog, 1 Pin, 2 Gate Seal, 3 Bronze Sigil, 4 Ash Lantern,
5-7 Grave Seals 1-3, 8 Ancient Key Fragment.
Flags bits: 0 statues, 1 levers, 2 bells, 3 coffins, 4 epitaph, 5 slice.
Maps are the Area Quest maps (12x12 each); gates: A0 (10,10) -> A1,
A1 (10,9) -> A2, return gates (1,1) in A1/A2, exit A2 (10,10).
"""
import os, sys
os.environ.setdefault("SDL_VIDEODRIVER", "dummy")
sys.path.insert(0, os.path.expanduser("~/workspace/gbc/tools/venv/lib/python3.12/site-packages"))
from pyboy import PyBoy
from collections import deque

ROM = os.path.expanduser("~/workspace/ninth-wound/releases/ninth-wound.gbc")
OUT = os.path.expanduser("~/workspace/ninth-wound/qa/shots")
os.makedirs(OUT, exist_ok=True)

MAPS = [
    ["1111111111111111", "1000001000000001", "1011101011111001",
     "1010001010000101", "1000101000100101", "1110100010000101",
     "1000101000100001", "1011101100111100", "101000000000000G",
     "1011101110111100", "1000001000000001", "1111101001111101",
     "1000101000100001", "1010101010100101", "1000001000000101",
     "1111111111111111"],
    ["TTTTTTTTTTTT", "TB000000000T", "T00T000R000T", "T0000000000T",
     "T0R00000T00T", "T0000000000T", "T000T0000R0T", "T0000000000T",
     "T00R000T000T", "T000000000GT", "T0000000000T", "TTTTTTTTTTTT"],
    ["101111111111", "1B0000000111", "101111010001", "100000010101",
     "101110110101", "100010000101", "111010111101", "100010100001",
     "101110101111", "100000101001", "111011000031", "111111111111"],
    ["TTTTTTTTTTTT", "TB000000000T", "T000T000000T", "T00T000T000T",
     "T0000000000T", "T0000000000T", "T000T000T00T", "T0000000000T",
     "T00T0000000T", "T000000000GT", "T000000000TT", "TTTTTTTTTTTT"],
    ["111111111111", "1B0000000001", "100000000001", "100100100001",
     "100000000001", "100000000001", "100100000001", "100000000001",
     "100001000001", "1000000000G1", "100000000011", "111111111111"],
    ["111111111111", "1B0000000001", "100000000001", "100100100001",
     "100000000001", "100000000001", "100000100001", "100000000001",
     "100100000001", "1000000000G1", "100000000011", "111111111111"],
    ["111111111111", "1B0000000001", "100000100001", "100000000001",
     "100100000001", "100000000001", "100000100001", "100000000001",
     "100100000001", "1000000000G1", "100000000001", "111111111111"],
    ["11111111111111111111", "1B000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000010000001000001", "10000000000000000001", "10000000000000000001",
     "100000000000000000G1", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000010000001000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "11111111111111111111"],
    ["11111111111111111111", "1B000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000010000001000001", "10000000000000000001", "10000000000000000001",
     "100000000000000000G1", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000010000001000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "11111111111111111111"],
    ["11111111111111111111", "1B000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000010000001000001", "10000000000000000001", "10000000000000000001",
     "100000000000000000G1", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000010000001000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "11111111111111111111"],
    ["11111111111111111111", "1B000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000010000001000001", "10000000000000000001", "10000000000000000001",
     "100000000000000000G1", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000010000001000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "11111111111111111111"],
    ["11111111111111111111", "1B000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000010000001000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "10000010000001000001", "10000000000000000001",
     "10000000000000000001", "10000000000000000001", "10000000000000000001",
     "10000000000000000001", "11111111111111111111"],
]
DIRS = [(1, 0), (0, 1), (-1, 0), (0, -1)]
ANG = {(1, 0): 0, (0, 1): 64, (-1, 0): 128, (0, -1): 192}
checks = []
def check(label, ok, detail=""):
    checks.append(bool(ok))
    print(f"{'PASS' if ok else 'FAIL'} {label} {detail}", flush=True)

def tick(pb, n):
    for _ in range(n):
        pb.tick()

def tap(pb, btn, settle=30, press=2):
    pb.button_press(btn); tick(pb, press); pb.button_release(btn); tick(pb, settle)

def scan(pb):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 13):
        if mem[a] == 0x4E and mem[a+1] == 0x57:
            state, area, ptx, pty, ang = mem[a+2], mem[a+3], mem[a+4], mem[a+5], mem[a+6]
            lo, mid, hi, hi2, fl = mem[a+7], mem[a+8], mem[a+9], mem[a+10], mem[a+11]
            if state <= 11 and area <= 11 and ptx <= 19 and pty <= 19 and ang in (0, 64, 128, 192):
                if mem[a+12] == ((state + area + ptx + pty + ang + lo + mid + hi + hi2 + fl) & 0xFF):
                    return (state, area, ptx, pty, ang, lo | (mid << 8) | (hi << 16) | (hi2 << 24), fl)
    return None

def scan_audio(pb):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 8):
        if mem[a] == 0x41 and mem[a+1] == 0x55 and mem[a+6] == 0xA5:
            music, last, pos, track = mem[a+2], mem[a+3], mem[a+4], mem[a+5]
            if music <= 1 and last <= 17 and pos <= 63 and track <= 3:
                if mem[a+7] == ((music + last + pos + track) & 0xFF):
                    return (music, last, pos, track)
    return None

def scan_intro(pb):
    # v5 intro-over-map proof: 'I','M',1,checksum in WRAM.
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 4):
        if mem[a] == 0x49 and mem[a+1] == 0x4D and mem[a+2] == 1:
            if mem[a+3] == ((0x49 + 0x4D + 1) & 0xFF):
                return True
    return False

def scan_inv(pb):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 4):
        if mem[a] == 0x49 and mem[a+1] == 0x50 and mem[a+3] == ((0x49 + 0x50 + mem[a+2]) & 0xFF):
            return mem[a+2]
    return None

def scan_code(pb):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 8):
        if mem[a] == 0x43 and mem[a+1] == 0x45:
            d0, d1, d2, cur, wrong = mem[a+2], mem[a+3], mem[a+4], mem[a+5], mem[a+6]
            if d0 <= 9 and d1 <= 9 and d2 <= 9 and cur <= 2 and wrong <= 1:
                if mem[a+7] == ((0x43 + 0x45 + d0 + d1 + d2 + cur + wrong) & 0xFF):
                    return (d0, d1, d2, cur, wrong)
    return None

def scan_clues(pb):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 4):
        if mem[a] == 0x4D and mem[a+1] == 0x43 and mem[a+3] == ((0x4D + 0x43 + mem[a+2]) & 0xFF):
            return mem[a+2]
    return None

def scan_dialog(pb):
    # v10 dialog proof: 'D','L',dialog id, checksum in WRAM.
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 4):
        if mem[a] == 0x44 and mem[a+1] == 0x4C and mem[a+3] == ((0x44 + 0x4C + mem[a+2]) & 0xFF):
            return mem[a+2]
    return None

def scan_rune(pb):
    # v10 rune proof: 'R','N',aux,status, checksum in WRAM.
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 5):
        if mem[a] == 0x52 and mem[a+1] == 0x4E and mem[a+4] == ((0x52 + 0x4E + mem[a+2] + mem[a+3]) & 0xFF):
            return (mem[a+2], mem[a+3])
    return None

def set_code(pb, digits):
    """Enter digits on the code screen; submit on the last digit.
    Returns True if the code screen was left (state != 7)."""
    for i, target in enumerate(digits):
        for _ in range(4):
            c = scan_code(pb)
            if c and c[3] == i:
                break
            tap(pb, "right", 40)
        for _ in range(12):
            c = scan_code(pb)
            if c and c[i] == target:
                break
            tap(pb, "up", 40)
        tap(pb, "a", 40)
    p = scan(pb)
    return bool(p and p[0] != 7)

def audio_check(label, last=None, music=None, track=None):
    a = scan_audio(pb)
    ok = a is not None
    if music is not None:
        ok = ok and a[0] == music
    if last is not None:
        ok = ok and a[1] == last
    if track is not None:
        ok = ok and a[3] == track
    check(label, ok, f"audio={a}")

def scan_proof2(pb, b0, b1):
    mem = pb.memory
    for a in range(0xC000, 0xE000 - 4):
        if mem[a] == b0 and mem[a+1] == b1 and mem[a+3] == ((b0 + b1 + mem[a+2]) & 0xFF):
            return mem[a+2]
    return None

def scan_title(pb):
    return scan_proof2(pb, 0x54, 0x47) == 1

def scan_keeper(pb):
    return scan_proof2(pb, 0x4B, 0x53) == 1

def scan_ending(pb):
    return scan_proof2(pb, 0x45, 0x4E)

def scan_choice(pb):
    return scan_proof2(pb, 0x43, 0x48)

def shot(pb, name):
    path = os.path.join(OUT, name)
    pb.screen.image.save(path)
    return path

def bfs(area, start, goal, allow="0", avoid=()):
    m = MAPS[area]
    q = deque([start]); prev = {start: None}
    while q:
        cur = q.popleft()
        if cur == goal:
            break
        for d in DIRS:
            nx, ny = cur[0] + d[0], cur[1] + d[1]
            if not (0 <= nx < len(m[0]) and 0 <= ny < len(m)):
                continue
            if (nx, ny) in avoid or (nx, ny) in prev:
                continue
            if m[ny][nx] not in allow:
                continue
            prev[(nx, ny)] = cur
            q.append((nx, ny))
    if goal not in prev:
        return None
    path, cur = [], goal
    while cur is not None:
        path.append(cur); cur = prev[cur]
    return path[::-1]

def face(pb, want):
    for _ in range(4):
        p = scan(pb)
        if p and p[4] == want:
            return True
        cur = p[4] if p else 0
        diff = (want - cur) % 256
        tap(pb, "right" if diff in (64, 128) else "left")
    p = scan(pb)
    return bool(p and p[4] == want)

def step_to(pb, path):
    for nxt in path[1:]:
        p = scan(pb)
        cur = (p[2], p[3])
        d = (nxt[0] - cur[0], nxt[1] - cur[1])
        if not face(pb, ANG[d]):
            return False
        tap(pb, "up")
        tick(pb, 60)
        p = scan(pb)
        if not p or (p[2], p[3]) != nxt:
            return False
    return True

def go(pb, area, goal, allow="0", avoid=()):
    p = scan(pb)
    path = bfs(area, (p[2], p[3]), goal, allow, avoid)
    if path is None:
        return False
    return step_to(pb, path)

def interact(pb, area, ent, label):
    p = scan(pb)
    cur = (p[2], p[3])
    best = None
    for d in DIRS:
        nx, ny = ent[0] + d[0], ent[1] + d[1]
        if not (0 <= nx < len(MAPS[area][0]) and 0 <= ny < len(MAPS[area])):
            continue
        if MAPS[area][ny][nx] != "0":
            continue
        path = bfs(area, cur, (nx, ny), "0")
        if path is not None and (best is None or len(path) < len(best[0])):
            best = (path, (nx, ny), d)
    if best is None:
        check(f"{label}: no reachable neighbour", False)
        return False
    path, neigh, d = best
    if not step_to(pb, path):
        check(f"{label}: walk to neighbour {neigh} failed", False, f"proof={scan(pb)}")
        return False
    want = ANG[(-d[0], -d[1])]
    if not face(pb, want):
        check(f"{label}: face entity failed", False, f"proof={scan(pb)}")
        return False
    # v2: interact SFX block inside the handler (gate-unlock + reward
    # is ~76 vblanks), and the proof block only updates when the
    # handler returns, so settle long past the longest SFX.
    tap(pb, "a", 120)
    p = scan(pb)
    if not p or p[0] != 5:
        check(f"{label}: dialog did not open", False, f"proof={p}")
        return False
    tap(pb, "a", 80)
    p = scan(pb)
    ok = bool(p and p[0] == 1)
    check(f"{label}: dialog opens and closes", ok, f"proof={p}")
    return ok

def go_neighbour(pb, area, target, label):
    """Walk to the nearest open neighbour of target; return (neigh, d)
    where d is the offset from neighbour to target, or False."""
    p = scan(pb)
    cur = (p[2], p[3])
    best = None
    for d in DIRS:
        nx, ny = target[0] + d[0], target[1] + d[1]
        if not (0 <= nx < len(MAPS[area][0]) and 0 <= ny < len(MAPS[area])):
            continue
        if MAPS[area][ny][nx] != "0":
            continue
        path = bfs(area, cur, (nx, ny), "0")
        if path is not None and (best is None or len(path) < len(best[0])):
            best = (path, (nx, ny), d)
    if best is None:
        check(f"{label}: no reachable neighbour", False)
        return False
    path, neigh, d = best
    if not step_to(pb, path):
        check(f"{label}: walk to neighbour {neigh} failed", False, f"proof={scan(pb)}")
        return False
    return (neigh, d)

def timed_step_onto(pb, target, label, budget=40):
    """From an adjacent tile facing target, hold Up and count ticks
    until proof first shows the target tile. v5 budget is 40 ticks:
    the PyBoy input-latency baseline is ~25 ticks, and v4's blocking
    pickup SFX added ~22 more (Cog measured 47). A non-blocking
    SFX must land at baseline, well under budget."""
    p = scan(pb)
    cur = (p[2], p[3])
    d = (target[0] - cur[0], target[1] - cur[1])
    if d not in ANG or not face(pb, ANG[d]):
        check(f"{label}: face target failed", False, f"proof={scan(pb)}")
        return None
    pb.button_press("up")
    for t in range(1, 121):
        tick(pb, 1)
        p = scan(pb)
        if p and (p[2], p[3]) == target:
            pb.button_release("up")
            tick(pb, 30)
            check(f"{label}: responsive ({t} ticks <= {budget})", t <= budget, f"ticks={t} proof={p}")
            return t
    pb.button_release("up")
    check(f"{label}: reached target within 120 ticks", False, f"proof={scan(pb)}")
    return None

def timed_pickup(pb, area, tile, bit, name):
    nb = go_neighbour(pb, area, tile, f"pickup {name}")
    if not nb:
        return False
    t = timed_step_onto(pb, tile, f"pickup {name} timing")
    p = scan(pb)
    ok = bool(p and (p[5] >> bit) & 1)
    check(f"pickup {name} at {tile}", ok, f"proof={p}")
    return ok

def timed_interact(pb, area, ent, label, budget=40):
    """Walk adjacent + face, press A, count ticks until dialog state 5.
    v5 budget 40: baseline ~25; v4's blocking grant SFX (unlock+reward
    = 58 frames) pushed this past 80. Returns True if dialog opened."""
    nb = go_neighbour(pb, area, ent, label)
    if not nb:
        return False
    neigh, d = nb
    if not face(pb, ANG[(-d[0], -d[1])]):
        check(f"{label}: face entity failed", False, f"proof={scan(pb)}")
        return False
    pb.button_press("a")
    for t in range(1, 121):
        tick(pb, 1)
        p = scan(pb)
        if p and p[0] == 5:
            pb.button_release("a")
            tick(pb, 30)
            check(f"{label}: dialog opened responsively ({t} ticks <= {budget})", t <= budget, f"ticks={t} proof={p}")
            return True
    pb.button_release("a")
    check(f"{label}: dialog opened within 120 ticks", False, f"proof={scan(pb)}")
    return False

def gate_cross(pb, area, approach, facing, expect, label):
    if not go(pb, area, approach):
        check(f"{label}: reach approach failed", False, f"proof={scan(pb)}")
        return False
    if not face(pb, facing):
        check(f"{label}: face gate failed", False, f"proof={scan(pb)}")
        return False
    tap(pb, "up"); tick(pb, 40)
    p = scan(pb)
    ok = bool(p and p[0:5] == expect[0:5])  # items/flags may change en route (pickups)
    check(label, ok, f"proof={p} expect={expect}")
    return ok

def gate_cross_banner(pb, area, approach, facing, dest_area, dest_pos, label, dismiss="a", shot_name=None):
    """v9: first forward crossing must land in banner state 11 for
    the destination area (proof state 11 = ST_BANNER), then either
    A-dismiss or auto-hide after ~180 ticks back to play state 1.
    Returns True if the banner appeared and play resumed."""
    if not go(pb, area, approach):
        check(f"{label}: reach approach failed", False, f"proof={scan(pb)}")
        return False
    if not face(pb, facing):
        check(f"{label}: face gate failed", False, f"proof={scan(pb)}")
        return False
    tap(pb, "up"); tick(pb, 40)
    p = scan(pb)
    ok = bool(p and p[0] == 11 and p[1] == dest_area and (p[2], p[3]) == dest_pos)
    check(f"{label}: banner state 11 for area {dest_area}", ok, f"proof={p}")
    if shot_name:
        shot(pb, shot_name)
    if dismiss == "a":
        tap(pb, "a", 40)
        p = scan(pb)
        check(f"{label}: A dismisses banner to play", bool(p and p[0] == 1 and p[1] == dest_area), f"proof={p}")
    else:
        tick(pb, 220)
        p = scan(pb)
        check(f"{label}: banner auto-hides to play", bool(p and p[0] == 1 and p[1] == dest_area), f"proof={p}")
    return ok

pb = PyBoy(ROM, window="null")
pb.set_emulation_speed(0)
tick(pb, 90)
shot(pb, "title.png")
p = scan(pb)
check("title state", p is not None and p[0] == 0, f"proof={p}")
a0 = scan_audio(pb)
tick(pb, 90)
a1 = scan_audio(pb)
check("audio: title music playing and advancing", a0 and a0[0] == 1 and a1 and a1[0] == 1 and a1[2] != a0[2], f"audio before={a0} after={a1}")
check("v8 title graphic proof (T,G,1)", scan_title(pb))

tap(pb, "start", 40)
p = scan(pb)
check("start -> intro state 6", p and p[0] == 6 and p[1:5] == (0, 1, 1, 0), f"proof={p}")
check("v5 intro entered over the map (intro proof block)", scan_intro(pb))
shot(pb, "intro-over-map.png")
for i in range(4):
    tap(pb, "a", 40)
    p = scan(pb)
    check(f"intro advances dialog {i+1} (still state 6)", p and p[0] == 6, f"proof={p}")
tap(pb, "a", 40)
p = scan(pb)
check("intro final A -> Gatehouse (1,1) east, play", p == (1, 0, 1, 1, 0, 0, 0), f"proof={p}")
# v4: first note moved away from spawn — A from spawn (1,1) facing east must do nothing.
tap(pb, "a", 40)
p = scan(pb)
check("no note at spawn: A from (1,1) east does nothing", p == (1, 0, 1, 1, 0, 0, 0), f"proof={p}")
audio_check("audio: title music stopped in game", music=0)
tap(pb, "right", 40)
audio_check("audio: step/turn SFX", last=1)
tap(pb, "left", 40)

p0 = scan(pb)
tap(pb, "start", 40)
p = scan(pb)
check("inventory opens state 3", p and p[0] == 3 and p[1:5] == p0[1:5], f"proof={p}")
tap(pb, "start", 40)
p = scan(pb)
check("inventory closes to play", p and p[0] == 1, f"proof={p}")
tap(pb, "select", 40)
p = scan(pb)
check("minimap opens state 4", p and p[0] == 4, f"proof={p}")
shot(pb, "minimap-gatehouse.png")
tap(pb, "select", 40)
p = scan(pb)
check("minimap closes to play", p and p[0] == 1, f"proof={p}")

gate_cross(pb, 0, (14, 8), 0, (1, 0, 14, 8, 0, 0, 0), "gate locked before Sigil (stays, no pass)")
audio_check("audio: bump on locked gate", last=2)

for tile, bit, name in [((13, 2), 0, "Rusted Cog"), ((3, 13), 1, "Iron Pin"), ((12, 13), 2, "Gate Seal")]:
    timed_pickup(pb, 0, tile, bit, name)
    if name == "Rusted Cog":
        audio_check(f"audio: pickup SFX for {name}", last=3)
        # v4 Cog-pickup regression: game must stay alive and responsive
        # after the Cog (turn, inventory open/close, idle ticks).
        tap(pb, "right", 40)
        tap(pb, "start", 40)
        p2 = scan(pb)
        check("Cog regression: inventory opens after Cog", p2 and p2[0] == 3, f"proof={p2}")
        tap(pb, "start", 40)
        tick(pb, 120)
        p2 = scan(pb)
        check("Cog regression: still playing after Cog + idle", p2 and p2[0] == 1 and (p2[5] >> 0) & 1, f"proof={p2}")

interact(pb, 0, (5, 7), "note: first note moved into Area 1 (gate goal)")
interact(pb, 0, (7, 4), "note: statues riddle (no direction given)")

for ent, presses in [((6, 5), 1), ((10, 5), 2), ((8, 7), 3)]:
    for _ in range(presses):
        interact(pb, 0, ent, f"statue {ent} rotate")
p = scan(pb)
check("statues solved flag", p and (p[6] & 1), f"proof={p}")
audio_check("audio: statue-solved grind, not ping (last=16)", last=16)

timed_interact(pb, 0, (13, 8), "gate mechanism grants Bronze Sigil (components + statues only, no levers)")
tap(pb, "a", 80)
p = scan(pb)
check("gate mechanism dialog closes to play", p and p[0] == 1, f"proof={p}")
p = scan(pb)
check("Bronze Sigil held (bit 3)", p and (p[5] >> 3) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Bronze Sigil", last=13)

gate_cross_banner(pb, 0, (14, 8), 0, 1, (2, 1), "gate Gatehouse -> Ash Village (2,1) east", shot_name="banner-ash.png")
audio_check("audio: gate jingle FIRST crossing only (last=17)", last=17)
gate_cross(pb, 1, (2, 1), 128, (1, 0, 14, 8, 128, p[5], p[6]), "return gate Ash Village -> Gatehouse (14,8) west")
audio_check("audio: return crossing same gate: NO gate sound (last=0)", last=0)
gate_cross(pb, 0, (14, 8), 0, (1, 1, 2, 1, 0, p[5], p[6]), "re-enter Ash Village (2,1) east")
audio_check("audio: later crossing same gate: NO gate sound (last=0)", last=0)

gate_cross(pb, 1, (10, 8), 64, (1, 1, 10, 8, 64, p[5], p[6]), "gate locked before Ash Lantern (stays)")

for ent, name in [((2, 7), "Elder"), ((5, 2), "Merchant"), ((8, 5), "Child"), ((4, 8), "Monk")]:
    interact(pb, 1, ent, f"NPC {name} dialog")

interact(pb, 1, (9, 5), "bell east (wrong, resets)")
p = scan(pb)
check("bells not solved after wrong order", p and not (p[6] & 4), f"proof={p}")
interact(pb, 1, (3, 5), "bell west in order")
audio_check("audio: bell west SFX (distinct pitch)", last=6)
for ent in [(9, 5), (6, 2)]:
    interact(pb, 1, ent, f"bell {ent} in order")
audio_check("audio: unified puzzle-solved grind after bells", last=16)
p = scan(pb)
check("bells solved flag", p and (p[6] & 4), f"proof={p}")

timed_interact(pb, 1, (6, 9), "lantern tower grants Ash Lantern")
tap(pb, "a", 80)
p = scan(pb)
check("lantern tower dialog closes to play", p and p[0] == 1, f"proof={p}")
p = scan(pb)
check("Ash Lantern held (bit 4)", p and (p[5] >> 4) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Ash Lantern", last=13)

gate_cross_banner(pb, 1, (10, 8), 64, 2, (1, 2), "gate Ash Village -> Burial Maze (1,2) south")
gate_cross(pb, 2, (2, 1), 128, (1, 1, 9, 9, 128, p[5], p[6]), "return gate Burial Maze -> Ash Village (9,9) west")
gate_cross(pb, 1, (9, 9), 0, (1, 2, 2, 1, 0, p[5], p[6]), "re-enter Burial Maze (2,1) east")

ok = go(pb, 2, (10, 9))
face(pb, 64)
tap(pb, "up"); tick(pb, 40)
p = scan(pb)
check("crypt exit blocked before completion (dialog, no exit)", p and p[0] == 5 and p[1] == 2, f"proof={p}")
tap(pb, "a", 40)
p = scan(pb)
check("exit-block dialog closes to play", p and p[0] == 1 and (p[2], p[3]) == (10, 9), f"proof={p}")

interact(pb, 2, (5, 5), "note: Area 3 pulse hint (flows south, face the pulse)")

for tile, bit in [((3, 3), 5), ((7, 5), 6), ((9, 7), 7)]:
    timed_pickup(pb, 2, tile, bit, f"Grave Seal bit {bit}")

# v5 coffin sprite visual check: stand adjacent facing coffin (5,1) and shoot.
nb = go_neighbour(pb, 2, (5, 1), "coffin view")
if nb:
    neigh, d = nb
    face(pb, ANG[(-d[0], -d[1])])
    shot(pb, "v5-coffin-view.png")

for ent, presses in [((5, 1), 1), ((3, 9), 2)]:
    for _ in range(presses):
        interact(pb, 2, ent, f"coffin {ent} rotate to pulse")
audio_check("audio: unified puzzle-solved grind after coffins", last=16)
p = scan(pb)
check("coffins solved flag (tomb puzzle)", p and (p[6] & 8), f"proof={p}")

interact(pb, 2, (8, 3), "epitaph read with Ash Lantern")
p = scan(pb)
check("epitaph flag", p and (p[6] & 16), f"proof={p}")

# --- v6 crypt code entry: stepping onto the crypt with all
# prerequisites met must NOT walk out; it opens code entry.
ok = go(pb, 2, (10, 9))
face(pb, 64)
tap(pb, "up"); tick(pb, 40)
p = scan(pb)
check("crypt step with prereqs -> code entry state 7 (no walk-out)", p and p[0] == 7 and (p[2], p[3]) == (10, 9), f"proof={p}")
c = scan_code(pb)
check("code screen starts at 000 cursor 0", c == (0, 0, 0, 0, 0), f"code={c}")
tap(pb, "start", 40)
p = scan(pb)
check("code entry Start cancels back to play at (10,9)", p and p[0] == 1 and (p[2], p[3]) == (10, 9), f"proof={p}")
tap(pb, "up"); tick(pb, 40)
p = scan(pb)
check("crypt step again -> code entry state 7", p and p[0] == 7, f"proof={p}")
# Wrong code 0-0-0: door stays closed, message, retry allowed.
set_code(pb, (0, 0, 0))
p = scan(pb)
c = scan_code(pb)
check("wrong code rejected: still code entry, wrong flag set", p and p[0] == 7 and c and c[4] == 1, f"proof={p} code={c}")
check("wrong code: no Fragment granted", p and not (p[5] >> 8) & 1, f"proof={p}")
check("wrong code: player still at (10,9), no progress lost", p and (p[2], p[3]) == (10, 9), f"proof={p}")
# Correct code 7-3-9.
ok = set_code(pb, (7, 3, 9))
p = scan(pb)
check("correct code leaves code entry to dialog state 5", ok and p and p[0] == 5, f"proof={p}")
tap(pb, "a", 80)
p = scan(pb)
check("crypt-open dialog closes to play at (10,9)", p and p[0] == 1 and (p[2], p[3]) == (10, 9), f"proof={p}")
check("Ancient Key Fragment held (bit 8)", p and (p[5] >> 8) & 1, f"proof={p}")
check("crypt-open flag (bit 5)", p and (p[6] & 32), f"proof={p}")

# --- v6 inventory paging with 9 items (page 1: 8, page 2: 1).
p_before = scan(pb)
tap(pb, "start", 40)
check("inventory opens (9 items) state 3", scan(pb) and scan(pb)[0] == 3, f"proof={scan(pb)}")
check("inventory page starts at 0", scan_inv(pb) == 0, f"inv={scan_inv(pb)}")
tap(pb, "right", 40)
check("inventory Right -> page 1", scan_inv(pb) == 1, f"inv={scan_inv(pb)}")
tap(pb, "right", 40)
check("inventory Right -> page 2 (4 pages in v8)", scan_inv(pb) == 2, f"inv={scan_inv(pb)}")
tap(pb, "right", 40)
check("inventory Right -> page 3", scan_inv(pb) == 3, f"inv={scan_inv(pb)}")
tap(pb, "right", 40)
check("inventory Right at last page wraps to page 0", scan_inv(pb) == 0, f"inv={scan_inv(pb)}")
tap(pb, "left", 40)
check("inventory Left at first page wraps to page 3", scan_inv(pb) == 3, f"inv={scan_inv(pb)}")
tap(pb, "start", 40)
p = scan(pb)
check("inventory closes, items unchanged by paging", p and p[0] == 1 and p[5] == p_before[5], f"proof={p}")

# Crypt now open: step south onto it -> banner for Forest of Veins
# (first crypt crossing), then A dismisses to play at (1,2) south.
tap(pb, "up"); tick(pb, 40)
p = scan(pb)
check("crypt open -> banner state 11 for Forest of Veins", p and p[0] == 11 and p[1] == 3 and (p[2], p[3]) == (1, 2), f"proof={p}")
shot(pb, "banner-forest.png")
tap(pb, "a", 40)
p = scan(pb)
check("crypt banner A-dismiss -> Forest of Veins (1,2) south, play", p and p[0:5] == (1, 3, 1, 2, 64), f"proof={p}")
audio_check("audio: crypt gate jingle first crossing (last=17)", last=17)
gate_cross(pb, 3, (2, 1), 128, (1, 2, 9, 10, 128, p[5], p[6]), "return gate Forest -> Burial Maze (9,10) west")
audio_check("audio: return gate pair 2 first crossing: jingle (last=17)", last=17)
gate_cross(pb, 2, (10, 9), 64, (1, 3, 1, 2, 64, p[5], p[6]), "re-enter Forest via crypt (1,2) south")
audio_check("audio: crypt re-entry repeat: NO gate sound (last=0)", last=0)

# --- Backtrack A locked: Forest chamber needs the Bone Fork (Area 5),
# so on first Forest visit it must refuse and grant nothing.
interact(pb, 3, (9, 2), "Backtrack A: Forest chamber before Bone Fork (locked)")
p = scan(pb)
check("Backtrack A locked: no Vein Organ before Bone Fork", p and not (p[5] >> 20) & 1, f"proof={p}")
check("v10 Vein Organ hint: locked Forest chamber refusal is dialog 96 (NEEDS BONE TUNING FORK)", scan_dialog(pb) == 96, f"dialog={scan_dialog(pb)}")
# --- Area 4: Forest of Veins (valve puzzle, order north-west-east).
gate_cross(pb, 3, (9, 9), 0, (1, 3, 9, 9, 0, p[5], p[6]), "Forest gate locked before Blood Sigil (stays)")
interact(pb, 3, (5, 5), "Forest note: valve order north-west-east")
interact(pb, 3, (4, 8), "Forest NPC Rootkeeper dialog")
interact(pb, 3, (3, 5), "Forest valve west first (wrong, resets)")
p = scan(pb)
check("Forest valves not solved after wrong order", p and not (p[6] & 2), f"proof={p}")
for ent in [(6, 2), (3, 5), (9, 5)]:
    interact(pb, 3, ent, f"Forest valve {ent} in order")
p = scan(pb)
check("Forest valves solved flag (bit 1)", p and (p[6] & 2), f"proof={p}")
timed_interact(pb, 3, (6, 9), "Forest barrier mechanism grants Blood Sigil")
tap(pb, "a", 80)
p = scan(pb)
check("Blood Sigil held (bit 9)", p and (p[5] >> 9) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Blood Sigil", last=13)
gate_cross_banner(pb, 3, (9, 9), 0, 4, (2, 1), "gate Forest -> Cathedral (2,1) east")
gate_cross(pb, 4, (2, 1), 128, (1, 3, 9, 9, 128, p[5], p[6]), "return gate Cathedral -> Forest (9,9) west")
gate_cross(pb, 3, (9, 9), 0, (1, 4, 2, 1, 0, p[5], p[6]), "re-enter Cathedral (2,1) east")

# --- Backtrack C locked: Cathedral chamber needs Lung Bellows (Area 8).
interact(pb, 4, (9, 2), "Backtrack C: Cathedral chamber before Lung Bellows (locked)")
p = scan(pb)
check("Backtrack C locked: no Binding Thread before Lung Bellows", p and not (p[5] >> 23) & 1, f"proof={p}")
# --- Area 5: Cathedral of Teeth (resonance, order high-low-middle).
gate_cross(pb, 4, (9, 9), 0, (1, 4, 9, 9, 0, p[5], p[6]), "Cathedral gate locked before Bone Fork (stays)")
interact(pb, 4, (5, 5), "Cathedral note: bones sing high-low-middle")
interact(pb, 4, (4, 8), "Cathedral NPC Cantor dialog")
interact(pb, 4, (3, 5), "Cathedral column west first (wrong, resets)")
p = scan(pb)
check("Cathedral columns not solved after wrong order", p and not (p[6] & 64), f"proof={p}")
for ent in [(6, 2), (3, 5), (9, 5)]:
    interact(pb, 4, ent, f"Cathedral column {ent} in order")
audio_check("audio: unified puzzle-solved grind after columns", last=16)
p = scan(pb)
check("Cathedral columns solved flag (bit 6)", p and (p[6] & 64), f"proof={p}")
timed_interact(pb, 4, (6, 9), "Cathedral mechanism grants Bone Fork")
tap(pb, "a", 80)
p = scan(pb)
check("Bone Fork held (bit 10)", p and (p[5] >> 10) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Bone Fork", last=13)
gate_cross(pb, 4, (2, 1), 128, (1, 3, 9, 9, 128, p[5], p[6]), "Backtrack A route: return Cathedral -> Forest (9,9) west")
interact(pb, 3, (9, 2), "Backtrack A: Forest chamber with Bone Fork (grants Vein Organ)")
p = scan(pb)
check("Backtrack A: Vein Organ held (bit 20)", p and (p[5] >> 20) & 1, f"proof={p}")
gate_cross(pb, 3, (9, 9), 0, (1, 4, 2, 1, 0, p[5], p[6]), "Backtrack A return: Forest -> Cathedral (2,1) east")
gate_cross_banner(pb, 4, (9, 9), 0, 5, (2, 1), "gate Cathedral -> Blood River (2,1) east")
gate_cross(pb, 5, (2, 1), 128, (1, 4, 9, 9, 128, p[5], p[6]), "return gate River -> Cathedral (9,9) west")
gate_cross(pb, 4, (9, 9), 0, (1, 5, 2, 1, 0, p[5], p[6]), "re-enter Blood River (2,1) east")

# --- Area 6: Blood River (flow valves, order west-east-north).
gate_cross(pb, 5, (9, 9), 0, (1, 5, 9, 9, 0, p[5], p[6]), "River gate locked before River Seal (stays)")
interact(pb, 5, (5, 5), "River note: flow west-east-north")
interact(pb, 5, (4, 8), "River NPC Ferryman dialog")
interact(pb, 5, (9, 5), "River valve east first (wrong, resets)")
p = scan(pb)
check("River valves not solved after wrong order", p and not (p[6] & 128), f"proof={p}")
for ent in [(3, 5), (9, 5), (6, 2)]:
    interact(pb, 5, ent, f"River valve {ent} in order")
p = scan(pb)
check("River valves solved flag (bit 7)", p and (p[6] & 128), f"proof={p}")
timed_interact(pb, 5, (6, 9), "River dock mechanism grants River Seal")
tap(pb, "a", 80)
p = scan(pb)
check("River Seal held (bit 11)", p and (p[5] >> 11) & 1, f"proof={p}")
audio_check("audio: reward fanfare for River Seal", last=13)
gate_cross_banner(pb, 5, (9, 9), 0, 6, (2, 1), "gate River -> Flesh Market (2,1) east")
gate_cross(pb, 6, (2, 1), 128, (1, 5, 9, 9, 128, p[5], p[6]), "return gate Market -> River (9,9) west")
gate_cross(pb, 5, (9, 9), 0, (1, 6, 2, 1, 0, p[5], p[6]), "re-enter Flesh Market (2,1) east")

# --- Area 7: Flesh Market (investigation/trading chain).
p_before = scan(pb)
tap(pb, "start", 40)
check("inventory opens (12 items) state 3", scan(pb) and scan(pb)[0] == 3, f"proof={scan(pb)}")
tap(pb, "right", 40)
check("inventory page 1 with 12 items", scan_inv(pb) == 1, f"inv={scan_inv(pb)}")
tap(pb, "left", 40)
check("inventory page back to 0", scan_inv(pb) == 0, f"inv={scan_inv(pb)}")
tap(pb, "start", 40)
p = scan(pb)
check("inventory closes, 12 items unchanged", p and p[0] == 1 and p[5] == p_before[5], f"proof={p}")
# v9 minimap name (middle area, 12x12): Flesh Market name above grid.
tap(pb, "select", 40)
p = scan(pb)
check("minimap opens in Flesh Market state 4", p and p[0] == 4 and p[1] == 6, f"proof={p}")
shot(pb, "minimap-market.png")
tap(pb, "select", 40)
p = scan(pb)
check("minimap closes to play in Flesh Market", p and p[0] == 1, f"proof={p}")
# --- v7 trade chain: Amber -> Idol -> Eye Lens. ---
interact(pb, 6, (6, 9), "Market keeper before Pale Idol (refuses)")
p = scan(pb)
check("Eye Lens not granted before Pale Idol", p and not (p[5] >> 12) & 1, f"proof={p}")
interact(pb, 6, (9, 5), "Market trader before Blood Amber (refuses)")
p = scan(pb)
check("trader refusal: no Pale Idol without Blood Amber", p and not (p[5] >> 14) & 1, f"proof={p}")
check("trader talk records clue bit 1", scan_clues(pb) == 2, f"clues={scan_clues(pb)}")
for ent, want in [((3, 5), 3), ((5, 8), 7)]:
    interact(pb, 6, ent, f"Market merchant {ent} clue (points to east trader)")
    check(f"merchant clues now {want}", scan_clues(pb) == want, f"clues={scan_clues(pb)}")
# Old v6 clue-only path must NOT grant the Eye Lens, even with all clues.
interact(pb, 6, (6, 9), "Market keeper after all clues but before Pale Idol (still refuses)")
p = scan(pb)
check("clue-only path dead: no Eye Lens with clues but no Idol", p and not (p[5] >> 12) & 1, f"proof={p}")
# Hidden ingredient: Blood Amber at the southeast back corner (10,10).
timed_pickup(pb, 6, (10, 10), 13, "Blood Amber")
p_before = scan(pb)
tap(pb, "start", 40)
check("inventory opens (13 items) state 3", scan(pb) and scan(pb)[0] == 3, f"proof={scan(pb)}")
tap(pb, "right", 40)
check("inventory page 1 with 13 items (Amber on page 2)", scan_inv(pb) == 1, f"inv={scan_inv(pb)}")
tap(pb, "start", 40)
p = scan(pb)
check("inventory closes, 13 items unchanged", p and p[0] == 1 and p[5] == p_before[5], f"proof={p}")
# Trader trade: Amber removed, Pale Idol added.
interact(pb, 6, (9, 5), "Market trader trade: Blood Amber -> Pale Idol")
p = scan(pb)
check("trader trade: Blood Amber removed", p and not (p[5] >> 13) & 1, f"proof={p}")
check("trader trade: Pale Idol added (bit 14)", p and (p[5] >> 14) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Pale Idol trade", last=13)
interact(pb, 6, (9, 5), "Market trader after trade (trade complete, no duplicate)")
p = scan(pb)
check("trader repeat: no duplicate Idol, no Amber back", p and (p[5] >> 14) & 1 and not (p[5] >> 13) & 1, f"proof={p}")
# Keeper trade in v8: Idol -> Eye Lens, game CONTINUES (no DONE screen).
nb = go_neighbour(pb, 6, (6, 9), "v8 keeper view")
if nb:
    neigh, d = nb
    face(pb, ANG[(-d[0], -d[1])])
    shot(pb, "v8-keeper-view.png")
    check("v8 Keeper sprite proof (K,S,1 = NPC set)", scan_keeper(pb))
interact(pb, 6, (6, 9), "Market keeper trade: Pale Idol -> Eye Lens")
p = scan(pb)
check("keeper trade: Eye Lens held (bit 12)", p and (p[5] >> 12) & 1, f"proof={p}")
check("keeper trade: Pale Idol removed", p and not (p[5] >> 14) & 1, f"proof={p}")
check("keeper trade: bits 0-12 + Vein Organ (bit 20, backtrack A), no ghosts", p and p[5] == ((1 << 13) - 1) | (1 << 20), f"proof={p}")
audio_check("audio: reward fanfare for Eye Lens", last=13)

# --- Area 8: Lung Gardens (20x20, wind puzzle east-north-west) ---
gate_cross_banner(pb, 6, (9, 9), 0, 7, (2, 1), "gate Market -> Lung Gardens (2,1) east", dismiss="wait", shot_name="banner-lung.png")
audio_check("audio: Market gate jingle first crossing (last=17)", last=17)
# v9 minimap name (late area, 20x20): Lung Gardens name must
# overlay the minimap's top row even with no free row.
tap(pb, "select", 40)
p = scan(pb)
check("minimap opens in Lung Gardens state 4", p and p[0] == 4 and p[1] == 7, f"proof={p}")
shot(pb, "minimap-lung.png")
tap(pb, "select", 40)
p = scan(pb)
check("minimap closes to play in Lung Gardens", p and p[0] == 1, f"proof={p}")
gate_cross(pb, 7, (17, 9), 0, (1, 7, 17, 9, 0, p[5], p[6]), "Lung gate locked before Lung Bellows (stays)")
interact(pb, 7, (10, 10), "Lung note: winds east-north-west")
interact(pb, 7, (4, 14), "Lung NPC dialog")
interact(pb, 7, (4, 4), "Lung vane west first (wrong, resets)")
interact(pb, 7, (10, 9), "Lung mechanism before winds set (refuses)")
p = scan(pb)
check("Lung Bellows not granted before winds set", p and not (p[5] >> 15) & 1, f"proof={p}")
for ent in [(15, 4), (10, 16), (4, 4)]:
    interact(pb, 7, ent, f"Lung vane {ent} in order")
audio_check("audio: unified puzzle-solved grind after winds", last=16)
timed_interact(pb, 7, (10, 9), "Lung mechanism grants Lung Bellows")
tap(pb, "a", 80)
p = scan(pb)
check("Lung Bellows held (bit 15)", p and (p[5] >> 15) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Lung Bellows", last=13)

# --- Backtrack C grant: full there-and-back Lung -> Cathedral -> Lung ---
gate_cross(pb, 7, (2, 1), 128, (1, 6, 9, 9, 128, p[5], p[6]), "Backtrack C route: Lung -> Market (9,9) west")
gate_cross(pb, 6, (2, 1), 128, (1, 5, 9, 9, 128, p[5], p[6]), "Backtrack C route: Market -> River (9,9) west")
gate_cross(pb, 5, (2, 1), 128, (1, 4, 9, 9, 128, p[5], p[6]), "Backtrack C route: River -> Cathedral (9,9) west")
interact(pb, 4, (9, 2), "Backtrack C: Cathedral chamber with Lung Bellows (grants Binding Thread)")
p = scan(pb)
check("Backtrack C: Binding Thread held (bit 23)", p and (p[5] >> 23) & 1, f"proof={p}")
gate_cross(pb, 4, (9, 9), 0, (1, 5, 2, 1, 0, p[5], p[6]), "Backtrack C return: Cathedral -> River (2,1) east")
gate_cross(pb, 5, (9, 9), 0, (1, 6, 2, 1, 0, p[5], p[6]), "Backtrack C return: River -> Market (2,1) east")
gate_cross(pb, 6, (9, 9), 0, (1, 7, 2, 1, 0, p[5], p[6]), "Backtrack C return: Market -> Lung (2,1) east")
gate_cross_banner(pb, 7, (17, 9), 0, 8, (2, 1), "gate Lung -> Bone Labyrinth (2,1) east")

# --- Area 9: Bone Labyrinth (20x20, 4 Bone Seals + spine north) ---
gate_cross(pb, 8, (17, 9), 0, (1, 8, 17, 9, 0, p[5], p[6]), "Bone gate locked before Silver Sigil (stays)")
for tile, bit in [((3, 3), 24), ((17, 3), 25), ((3, 17), 26), ((17, 17), 27)]:
    timed_pickup(pb, 8, tile, bit, f"Bone Seal bit {bit}")
timed_pickup(pb, 8, (10, 10), 21, "Gold Ingot")
interact(pb, 8, (10, 14), "Bone note: spine points north")
interact(pb, 8, (10, 2), "Bone NPC dialog")
interact(pb, 8, (10, 9), "Bone mechanism before spine true (refuses)")
p = scan(pb)
check("Silver Sigil not granted before spine true", p and not (p[5] >> 16) & 1, f"proof={p}")
for ent, presses in [((6, 10), 3), ((10, 6), 2), ((14, 10), 1)]:
    for _ in range(presses):
        interact(pb, 8, ent, f"spine {ent} rotate to north")
audio_check("audio: unified puzzle-solved grind after spine", last=16)
timed_interact(pb, 8, (10, 9), "Bone mechanism grants Silver Sigil")
tap(pb, "a", 80)
p = scan(pb)
check("Silver Sigil held (bit 16)", p and (p[5] >> 16) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Silver Sigil", last=13)
gate_cross_banner(pb, 8, (17, 9), 0, 9, (2, 1), "gate Bone -> Memory Vault (2,1) east")

# --- Area 10: Memory Vault (20x20, sequence dream-birth-war-peace) ---
gate_cross(pb, 9, (17, 9), 0, (1, 9, 17, 9, 0, p[5], p[6]), "Memory gate locked before Heart Seal (stays)")
interact(pb, 9, (10, 10), "Memory note: order dream-birth-war-peace")
interact(pb, 9, (4, 14), "Memory true record NPC dialog")
interact(pb, 9, (15, 10), "Memory propaganda record NPC dialog")
interact(pb, 9, (4, 4), "Memory rune birth first (wrong, resets)")
check("v10 rune label: wrong Birth toggle shows BIRTH (aux 0, reset)", scan_rune(pb) == (0, 2), f"rune={scan_rune(pb)}")
interact(pb, 9, (10, 9), "Memory mechanism before sequence (refuses)")
p = scan(pb)
check("Heart Seal not granted before memory sequence", p and not (p[5] >> 17) & 1, f"proof={p}")
for ent, aux, status in [((10, 16), 2, 0), ((4, 4), 0, 0), ((15, 15), 3, 0), ((15, 4), 1, 1)]:
    interact(pb, 9, ent, f"Memory rune {ent} in order")
    check(f"v10 rune label: rune {ent} shows aux {aux} status {status}", scan_rune(pb) == (aux, status), f"rune={scan_rune(pb)}")
audio_check("audio: unified puzzle-solved grind after memory sequence", last=16)
timed_interact(pb, 9, (10, 9), "Memory mechanism grants Heart Seal")
tap(pb, "a", 80)
p = scan(pb)
check("Heart Seal held (bit 17)", p and (p[5] >> 17) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Heart Seal", last=13)
gate_cross_banner(pb, 9, (17, 9), 0, 10, (2, 1), "gate Memory -> Heart Chamber (2,1) east")

# --- Area 11: Heart Chamber (20x20, organs west-north-east + Vein Organ) ---
gate_cross(pb, 10, (17, 9), 0, (1, 10, 17, 9, 0, p[5], p[6]), "Heart gate locked before Heart Key (stays)")
interact(pb, 10, (10, 10), "Heart note: organs west-north-east")
interact(pb, 10, (12, 9), "Heart note: Vein Organ in Forest of Veins chamber (v10)")
check("v10 Vein Organ hint: Heart Chamber note is dialog 128", scan_dialog(pb) == 128, f"dialog={scan_dialog(pb)}")
interact(pb, 10, (4, 14), "Heart NPC dialog (Throne gate needs Heart + Market Key)")
interact(pb, 10, (15, 4), "Heart organ east first (wrong, resets)")
interact(pb, 10, (10, 9), "Heart mechanism before organs sync (refuses)")
p = scan(pb)
check("Heart Key not granted before organs sync", p and not (p[5] >> 18) & 1, f"proof={p}")
for ent in [(4, 4), (10, 16), (15, 4)]:
    interact(pb, 10, ent, f"Heart organ {ent} in order")
audio_check("audio: unified puzzle-solved grind after organs", last=16)
timed_interact(pb, 10, (10, 9), "Heart mechanism grants Heart Key (Vein Organ held)")
tap(pb, "a", 80)
p = scan(pb)
check("Heart Key held (bit 18)", p and (p[5] >> 18) & 1, f"proof={p}")
check("Heart Key grant kept Vein Organ (backtrack A item still held)", p and (p[5] >> 20) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Heart Key", last=13)

# --- Backtrack B: Throne gate needs Market Key; go back to Market, trade, return ---
gate_cross(pb, 10, (17, 9), 0, (1, 10, 17, 9, 0, p[5], p[6]), "Throne gate locked before Market Key (stays, Heart Key held)")
gate_cross(pb, 10, (2, 1), 128, (1, 9, 17, 9, 128, p[5], p[6]), "Backtrack B route: Heart -> Memory (17,9) west")
gate_cross(pb, 9, (2, 1), 128, (1, 8, 17, 9, 128, p[5], p[6]), "Backtrack B route: Memory -> Bone (17,9) west")
gate_cross(pb, 8, (2, 1), 128, (1, 7, 17, 9, 128, p[5], p[6]), "Backtrack B route: Bone -> Lung (17,9) west")
gate_cross(pb, 7, (2, 1), 128, (1, 6, 9, 9, 128, p[5], p[6]), "Backtrack B route: Lung -> Market (9,9) west")
interact(pb, 6, (2, 8), "Backtrack B: Market merchant trade Gold Ingot -> Market Key")
p = scan(pb)
check("Backtrack B: Market Key held (bit 22)", p and (p[5] >> 22) & 1, f"proof={p}")
check("Backtrack B: Gold Ingot spent", p and not (p[5] >> 21) & 1, f"proof={p}")
gate_cross(pb, 6, (9, 9), 0, (1, 7, 2, 1, 0, p[5], p[6]), "Backtrack B return: Market -> Lung (2,1) east")
audio_check("audio: Flesh Market forward-gate repeat: NO gate sound (last=0)", last=0)
gate_cross(pb, 7, (17, 9), 0, (1, 8, 2, 1, 0, p[5], p[6]), "Backtrack B return: Lung -> Bone (2,1) east")
gate_cross(pb, 8, (17, 9), 0, (1, 9, 2, 1, 0, p[5], p[6]), "Backtrack B return: Bone -> Memory (2,1) east")
gate_cross(pb, 9, (17, 9), 0, (1, 10, 2, 1, 0, p[5], p[6]), "Backtrack B return: Memory -> Heart (2,1) east")
gate_cross_banner(pb, 10, (17, 9), 0, 11, (2, 1), "gate Heart -> Throne of the Sleeper (2,1) east", shot_name="banner-throne.png")

# --- Area 12: Throne of the Sleeper (20x20, Ninth Key + final choice) ---
interact(pb, 11, (10, 5), "Throne before Ninth Key (sleeps, no choice)")
p = scan(pb)
check("Throne before Ninth Key stays in play (no choice screen)", p and p[0] == 1, f"proof={p}")
timed_interact(pb, 11, (10, 9), "Throne mechanism assembles Ninth Key")
tap(pb, "a", 80)
p = scan(pb)
check("Ninth Key held (bit 19)", p and (p[5] >> 19) & 1, f"proof={p}")
check("Ninth Key assembly kept all components", p and (p[5] >> 3) & 1 and (p[5] >> 16) & 1 and (p[5] >> 17) & 1 and (p[5] >> 8) & 1 and (p[5] >> 11) & 1 and (p[5] >> 23) & 1, f"proof={p}")
audio_check("audio: reward fanfare for Ninth Key", last=13)
# Throne -> choice screen (state 9)
nb = go_neighbour(pb, 11, (10, 5), "Throne choice")
if nb:
    neigh, d = nb
    face(pb, ANG[(-d[0], -d[1])])
    tap(pb, "a", 120)
    p = scan(pb)
    check("Throne with Ninth Key -> final choice state 9", p and p[0] == 9 and p[1] == 11, f"proof={p}")
    check("choice starts at Warden (idx 0)", scan_choice(pb) == 0, f"choice={scan_choice(pb)}")
    tap(pb, "right", 40)
    check("choice Right -> Awakening (idx 1)", scan_choice(pb) == 1, f"choice={scan_choice(pb)}")
    tap(pb, "right", 40)
    check("choice Right -> Escape (idx 2)", scan_choice(pb) == 2, f"choice={scan_choice(pb)}")
    tap(pb, "right", 40)
    check("choice Right wraps -> Warden (idx 0)", scan_choice(pb) == 0, f"choice={scan_choice(pb)}")
    tap(pb, "left", 40)
    check("choice Left wraps -> Escape (idx 2)", scan_choice(pb) == 2, f"choice={scan_choice(pb)}")
    shot(pb, "final-choice.png")
    tap(pb, "a", 80)
    p = scan(pb)
    check("Escape confirmed -> ending state 10", p and p[0] == 10 and p[1] == 11, f"proof={p}")
    check("ending proof = Escape (id 2)", scan_ending(pb) == 2, f"ending={scan_ending(pb)}")
    audio_check("audio: Escape ending music on, track 3", music=1, track=3)
    shot(pb, "ending-escape.png")
else:
    check("Throne choice: no reachable neighbour", False)
tap(pb, "start", 40)
p = scan(pb)
check("ending screen Start -> title", p and p[0] == 0, f"proof={p}")
tap(pb, "start", 40)
p = scan(pb)
check("title Start -> intro again", p and p[0] == 6, f"proof={p}")
tap(pb, "start", 40)
p = scan(pb)
check("intro Start skips to play", p and p[0] == 1 and (p[2], p[3]) == (1, 1), f"proof={p}")

print(f"\n{sum(checks)}/{len(checks)} checks passed")
pb.stop()
sys.exit(0 if all(checks) else 1)
