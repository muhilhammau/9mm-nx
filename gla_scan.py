import re, sys
for n in ("menu_hud.gla", "effects.gla", "location_main_menu.gla"):
    p = r"G:\switch\9mm_nx\gameloft\games\Gloft9MHM\%s" % n
    cnt = {}
    with open(p, "rb") as f:
        prev = b""
        while True:
            b = f.read(8 << 20)
            if not b: break
            d = prev + b
            for m in re.finditer(rb"[A-Za-z0-9_./-]{2,60}\.(tga|pvr|dds|atc|etc|etc1|ktx|png|dxt)", d, re.I):
                e = m.group(1).lower().decode()
                cnt[e] = cnt.get(e, 0) + 1
            prev = d[-64:]
    print(n, cnt)
