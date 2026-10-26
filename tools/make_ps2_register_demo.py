"""Generate original bare-metal EE -> GS register test, without Sony assets."""
from pathlib import Path
import struct
code=[0x3c081200,0x3409bb55,0xfd0900e0,0x400a7800,0x240b0007,0x240c0005,0x016c0018,0x00001012,0xac020200,0x08040009,0]
data=bytearray(0x100+4*len(code));data[:16]=b'\x7fELF\x01\x01\x01'+bytes(9)
struct.pack_into('<HHIIIIIHHHHHH',data,16,2,8,1,0x100000,52,0,0,52,32,1,0,0,0)
struct.pack_into('<IIIIIIII',data,52,1,0x100,0x100000,0x100000,len(code)*4,len(code)*4,7,16)
for n,word in enumerate(code):struct.pack_into('<I',data,0x100+4*n,word)
(Path(__file__).resolve().parent.parent/'examples'/'ps2_register_demo.elf').write_bytes(data)
