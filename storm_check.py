from elftools.elf.elffile import ELFFile
def syms(p):
    e = ELFFile(open(p, "rb")); out = {"def": set(), "und": set()}
    for sn in (".dynsym",):
        sec = e.get_section_by_name(sn)
        for s in sec.iter_symbols():
            if not s.name: continue
            (out["und"] if s["st_shndx"] == "SHN_UNDEF" else out["def"]).add(s.name)
    return out
a = syms(r"H:\GAMES\9MM-NX\GAME\APK_EXTRACTED\lib\armeabi-v7a\libCopStory.so")
b = syms(r"H:\GAMES\9MM-NX\GAME\APK_EXTRACTED\lib\armeabi-v7a\libStormGLOFT.so")
print("libCopStory undefined:", len(a["und"]))
print("served by libStormGLOFT:", sorted(a["und"] & b["def"])[:30])
