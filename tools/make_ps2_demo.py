"""Original bare-metal EE subset demo; no Sony BIOS/kernel or game assets."""
from pathlib import Path
import struct
root=Path(__file__).resolve().parent.parent
code=[0x24080007,0x24090005,0x01091021,0xac020200,0x08040004,0]
data=bytearray(0x100+len(code)*4)
data[:16]=b'\x7fELF\x01\x01\x01'+bytes(9)
struct.pack_into('<HHIIIIIHHHHHH',data,16,2,8,1,0x100000,52,0,0,52,32,1,0,0,0)
struct.pack_into('<IIIIIIII',data,52,1,0x100,0x100000,0x100000,len(code)*4,len(code)*4,7,16)
for n,word in enumerate(code):struct.pack_into('<I',data,0x100+n*4,word)
(root/'examples'/'ps2_ee_demo.elf').write_bytes(data)
print('Wrote original PS2 EE subset ELF')
