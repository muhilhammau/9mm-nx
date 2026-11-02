import re
p = r"G:\switch\9mm_nx\gameloft\games\Gloft9MHM\menu_hud.gla"
with open(p, "rb") as f:
    head = f.read(0x1600)
print("first 48 bytes:", head[:48].hex(" "))
for m in re.finditer(rb"[ -~]{4,}", head):
    s = m.group().decode()
    print(f"{m.start():#06x}  {s}")
