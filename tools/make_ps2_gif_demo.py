"""Original two-segment bare-metal EE/GIF demo; no BIOS or Sony code."""
from pathlib import Path
import struct
code=[0x3c081000,0x35086000,0x24090100,0x792a0000,0x7d0a0000,
      0x25290010,0x792a0000,0x7d0a0000,0x08040008,0]
data=bytearray(0x140);data[:16]=b'\x7fELF\x01\x01\x01'+bytes(9)
struct.pack_into('<HHIIIIIHHHHHH',data,16,2,8,1,0x100000,52,0,0,52,32,2,0,0,0)
struct.pack_into('<IIIIIIII',data,52,1,0x100,0x100000,0x100000,len(code)*4,len(code)*4,5,16)
struct.pack_into('<IIIIIIII',data,84,1,0x130,0x100,0x100,32,32,6,16)
data.extend(bytes(0x150-len(data)))
for n,word in enumerate(code):struct.pack_into('<I',data,0x100+4*n,word)
struct.pack_into('<QQQQ',data,0x130,1|(1<<15)|(1<<60),14,0x10001,0x4c)
(Path(__file__).resolve().parent.parent/'examples'/'ps2_gif_demo.elf').write_bytes(data)
