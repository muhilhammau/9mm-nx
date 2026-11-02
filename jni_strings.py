import re
data = open(r".apk_libs\libCopStory.so", "rb").read()
strs = set(m.group().decode() for m in re.finditer(rb"[ -~]{4,200}", data))
cls = sorted(s for s in strs if re.fullmatch(r"(com|android|java|javax|org)/[A-Za-z0-9_/$]+", s))
sig = sorted(s for s in strs if re.fullmatch(r"\([A-Za-z0-9_/;\[]*\)[A-Za-z0-9_/;\[]+", s))
names = ["getResourceFull","getResourceBytes","getResourceLength","getSoundRaw","getResourceLengthSoundRaw","DetectDeviceGPU","getBuildType","detectLanguage","HasGyroscope","Is3GEnabled","IsWifiEnabled","isExternalMusicActive","isNotUseSoundEngine","IsAcerA500","IsSGHi897","IsSCHi510","IsUseExternalCard1","isHTC","getWifiIP","isKeyboardVisible"]
print("== classes ==")
for s in cls: print(s)
print("== signatures:", len(sig))
for s in sig[:80]: print(s)
print("== method names present as strings ==")
for n in names: print(n, n in strs)
