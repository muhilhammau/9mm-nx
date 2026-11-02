import sys
from elftools.elf.elffile import ELFFile
want = {"ptsname", "unlockpt"}
for lib in ("libStormGLOFT.so", "libCopStory.so"):
    e = ELFFile(open(r".apk_libs\\" + lib, "rb"))
    sec = e.get_section_by_name(".dynsym")
    hit = [s.name for s in sec.iter_symbols() if s.name in want and s["st_shndx"] == "SHN_UNDEF"]
    print(lib, "imports:", hit)
