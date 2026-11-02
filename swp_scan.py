import struct
from elftools.elf.elffile import ELFFile
def scan(p):
    f = open(p, "rb"); e = ELFFile(f); hits = []
    for seg in e.iter_segments():
        if seg["p_type"] != "PT_LOAD" or not (seg["p_flags"] & 1): continue
        data = seg.data(); base = seg["p_vaddr"]
        for i in range(0, len(data) - 3, 4):
            w = struct.unpack_from("<I", data, i)[0]
            if (w & 0x0FB00FF0) == 0x01000090:
                hits.append((base + i, w))
    return hits
for n in ("libCopStory.so", "libStormGLOFT.so"):
    h = scan(r"H:\GAMES\9MM-NX\GAME\APK_EXTRACTED\lib\armeabi-v7a\%s" % n)
    print(n, "SWP candidates:", len(h))
    for a, w in h[:40]:
        print("  0x%06x  %08x%s" % (a, w, "   <- crash" if a == 0x740c64 else ""))
