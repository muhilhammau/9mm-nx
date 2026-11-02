from elftools.elf.elffile import ELFFile
e = ELFFile(open(r"H:\GAMES\9MM-NX\GAME\APK_EXTRACTED\lib\armeabi-v7a\libCopStory.so","rb"))
for s in e.get_section_by_name(".dynsym").iter_symbols():
    if s.name.startswith("Java_") or s.name=="JNI_OnLoad":
        if s["st_shndx"]!="SHN_UNDEF": print(s.name)
