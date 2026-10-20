# .TTRGF v1 (little-endian, binary)

## Header (36 bytes)
| Field        | Type | Notes                                   |
|--------------|------|-----------------------------------------|
| magic        | char[4] | "TTRG"                               |
| version      | u16  | 1                                       |
| flags        | u16  | reserved, 0                             |
| fps          | f32  | counter FPS chosen by the user (e.g. 60)|
| tps          | f32  | replay tick rate (e.g. 240)             |
| levelId      | u32  |                                         |
| totalFrames  | u32  |                                         |
| inputCount   | u32  |                                         |
| resultCount  | u32  |                                         |
| soundCount   | u32  |                                         |

## Sections (in order)
**Inputs** (6 bytes each): frame u32, button u8, flags u8 (bit0 = down, bit1 = player 2)

**Results** (24 bytes each):
id u32, clickIndex u32, firstValidFrame u32, lastValidFrame u32,
frames u16 (whole frames), type u8 (0 = window, 1 = frame-perfect), pad u8,
precise f32 (e.g. 2.8)
HUD shows `frames / precise`.

**Sounds** (9 bytes each): frame u32, resultId u32, soundId u8
Keyed to frame index, never wall time, so audio stays in sync.

## Footer
crc32 u32 over every byte before it. Readers must reject on mismatch or version > supported.
