import os, re
d = r"G:\switch\9mm_nx\gameloft\games\Gloft9MHM"
magics = {"PVR!": rb"PVR!", "PVR3": rb"PVR\x03", "DDS ": rb"DDS \x7c\x00\x00\x00", "KTX": rb"\xabKTX 11", "ATC": rb"ATITC", "ETC1": rb"PKM 10"}
for n in ("menu_hud.gla","effects.gla","levels.gla","actors.gla","location_main_menu.gla","location_001_latino.gla"):
    p = os.path.join(d, n); cnt = {k: 0 for k in magics}
    with open(p, "rb") as f:
        prev = b""
        while True:
            b = f.read(16 << 20)
            if not b: break
            blk = prev + b
            for k, pat in magics.items():
                cnt[k] += len(re.findall(pat, blk))
            prev = blk[-16:]
    print(n, {k: v for k, v in cnt.items() if v})
