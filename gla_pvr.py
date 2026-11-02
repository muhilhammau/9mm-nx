import os, re, struct
d = r"G:\switch\9mm_nx\gameloft\games\Gloft9MHM"
names = [rb"MC_color", rb"MC_normal", rb"00_fpp_props", rb"car_texture", rb"00_fpp_skybox"]
fmtname = {0x0C:"PVRTC2",0x0D:"PVRTC4",0x10:"RGBA4444",0x11:"RGBA5551",0x12:"RGBA8888",0x13:"RGB565",0x14:"RGB555",0x15:"RGB888",0x16:"I8",0x17:"AI88",0x18:"PVRTC2",0x19:"PVRTC4",0x1B:"BGRA8888",0x36:"ETC1"}
for n in ("menu_hud.gla","actors.gla","location_main_menu.gla","location_001_latino.gla"):
    p = os.path.join(d, n); fm = {}; found = {}
    with open(p, "rb") as f:
        off = 0; prev = b""
        while True:
            b = f.read(16 << 20)
            if not b: break
            blk = prev + b; base = off - len(prev)
            for m in re.finditer(rb"PVR!", blk):
                s = m.start() - 44
                if s >= 0 and s + 52 <= len(blk):
                    hs, h, w, mips, flags = struct.unpack_from("<5I", blk, s)
                    if hs == 52:
                        k = fmtname.get(flags & 0xFF, hex(flags & 0xFF)); fm[k] = fm.get(k, 0) + 1
            for nm in names:
                for m in re.finditer(nm + rb"[A-Za-z0-9_.]{0,12}", blk):
                    found.setdefault(nm.decode(), set()).add(m.group(0).decode("latin1"))
            prev = blk[-64:]; off += len(b)
    print(n, "formats:", fm)
    for k, v in found.items(): print("   name", k, "->", sorted(v)[:4])
