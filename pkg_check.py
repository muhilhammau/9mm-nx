import re
d = open(r"H:\GAMES\9MM-NX\GAME\APK_EXTRACTED\AndroidManifest.xml", "rb").read()
found = set()
for off in (0, 1):
    s = d[off:].decode("utf-16le", "ignore")
    found.update(re.findall(r"com\.gameloft\.android\.[A-Za-z0-9_.]+", s))
for f in sorted(found)[:20]: print(f)
