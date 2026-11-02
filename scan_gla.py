import os, re, sys
d = r"G:\switch\9mm_nx\gameloft\games\Gloft9MHM"
names = ["HUD.tga", "splash.tga", "loading.tga", "fire_explosions_blood.tga",
         "01_mm_poor_urban_asphalt.tga", "mm_lightmap1024x1024.tga", "bullet_hole.tga"]
pats = {}
for n in names:
    pats[n] = [re.compile(re.escape(n.encode()), re.I), re.compile(re.escape(n.encode("utf-16le")), re.I)]
for f in sorted(os.listdir(d)):
    p = os.path.join(d, f)
    if not os.path.isfile(p) or not f.lower().endswith((".gla", ".glza")):
        continue
    hits = {n: 0 for n in names}
    first = {}
    pos = 0
    tail = b""
    with open(p, "rb") as fh:
        while True:
            chunk = fh.read(8 << 20)
            if not chunk:
                break
            buf = tail + chunk
            for n, (a, w) in pats.items():
                for m in a.finditer(buf):
                    hits[n] += 1
                    first.setdefault(n, pos - len(tail) + m.start())
                for m in w.finditer(buf):
                    hits[n] += 1
                    first.setdefault(n, pos - len(tail) + m.start())
            pos += len(chunk)
            tail = buf[-80:]
    found = {n: (c, hex(first[n])) for n, c in hits.items() if c}
    print(f"{f:40s} {os.path.getsize(p)//1048576:5d} MB  {found if found else '-'}")
