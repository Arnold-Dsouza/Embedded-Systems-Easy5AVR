# VMA419 LED Matrix Complete Operation Diagram

## Physical Hardware Structure

```
VMA419 LED Matrix Panel (32x16 pixels)
┌─────────────────────────────────────────────────────────────────┐
│  ROW 0:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●  (32 LEDs)          │
│  ROW 1:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 2:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 3:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 4:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 5:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 6:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 7:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 8:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 9:  ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 10: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 11: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 12: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 13: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 14: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
│  ROW 15: ●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●●                    │
└─────────────────────────────────────────────────────────────────┘

Control Pins:
- CLK (PB7):    Serial Clock for shift registers
- DATA (PB5):   Serial Data input (MOSI)
- A (PA1):      Row select bit 0
- B (PA2):      Row select bit 1  
- LATCH (PA4):  Latch/Store clock
- OE (PD7):     Output Enable (active low)
```

## Internal LED Matrix Architecture

The VMA419 uses shift registers and multiplexing to control all 512 LEDs (32×16) with minimal pins:

```
┌─────────────────────────────────────────────────────────────────┐
│                    SHIFT REGISTER CHAIN                          │
│                                                                 │
│  DATA → [SR1] → [SR2] → [SR3] → [SR4] → ... → [SR32]           │
│    ↑      ↓       ↓       ↓       ↓              ↓             │
│   CLK    LED1    LED2    LED3    LED4           LED32           │
│          ↓       ↓       ↓       ↓              ↓             │
│        ROW 0   ROW 0   ROW 0   ROW 0          ROW 0           │
│                                                                 │
│  LATCH pulse stores all 32 bits simultaneously                  │
│                                                                 │
│  Row Selection (A,B pins) determines which row is active:       │
│  A=0,B=0 → Rows 0,4,8,12    (scan_cycle 0)                    │
│  A=1,B=0 → Rows 1,5,9,13    (scan_cycle 1)                    │
│  A=0,B=1 → Rows 2,6,10,14   (scan_cycle 2)                    │
│  A=1,B=1 → Rows 3,7,11,15   (scan_cycle 3)                    │
└─────────────────────────────────────────────────────────────────┘
```

## Multiplexing Time Diagram

```
Time →  |← 1ms →|← 1ms →|← 1ms →|← 1ms →|← 1ms →|← 1ms →|
        
Cycle:     0       1       2       3       0       1    ...
        
A pin:   ──────┐       ┐       ┐       ┐       ┐       
               └───────┘       └───────┘       └───────
        
B pin:   ──────────────┐               ┐               
                       └───────────────┘       

Active   Rows    Rows    Rows    Rows    Rows    Rows
Rows:   0,4,8,12 1,5,9,13 2,6,10,14 3,7,11,15 0,4,8,12 1,5,9,13

OE:     ─┐     ┌─┐     ┌─┐     ┌─┐     ┌─┐     ┌─
         └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ 
         ^       ^       ^       ^       ^
         |       |       |       |       |
     Load data for each scan cycle, then enable output
```

## Data Flow Step-by-Step

### Step 1: Frame Buffer Organization
```
Frame Buffer (Linear Array):
[Row0_Data][Row1_Data][Row2_Data]...[Row15_Data]

Each Row Data = 4 bytes (32 pixels ÷ 8 bits/byte)
Row0_Data: [Byte0][Byte1][Byte2][Byte3]
           Pixels: 0-7  8-15 16-23 24-31

Bit mapping within each byte:
Byte0: [P7][P6][P5][P4][P3][P2][P1][P0]
       MSB                          LSB
```

### Step 2: Scan Cycle Data Selection
```
For scan_cycle = 0 (rows 0,4,8,12):
┌─────────────────────────────────────┐
│ Send Row 0 data: 4 bytes            │
│ Send Row 4 data: 4 bytes            │  
│ Send Row 8 data: 4 bytes            │
│ Send Row 12 data: 4 bytes           │
└─────────────────────────────────────┘
Total: 16 bytes shifted out

For scan_cycle = 1 (rows 1,5,9,13):
┌─────────────────────────────────────┐
│ Send Row 1 data: 4 bytes            │
│ Send Row 5 data: 4 bytes            │
│ Send Row 9 data: 4 bytes            │
│ Send Row 13 data: 4 bytes           │
└─────────────────────────────────────┘
```

### Step 3: SPI Bit-Bang Transmission
```
For each byte to be transmitted:

DATA Pin:  ──┐   ┌───┐       ┌───────────┐   ┌───
             └───┘   └───────┘           └───┘
             
CLK Pin:   ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
           └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘
           
Bit:        7   6   5   4   3   2   1   0
           MSB                         LSB

Data is shifted MSB first, with 2μs delays between clock edges
```

### Step 4: Latch and Display
```
After all data is shifted:

LATCH Pin: ────┐     ┌────────────
               └─────┘  (10μs pulse)
               ^
               Data is latched into output registers

OE Pin:    ─────────┐           ┌──────  
                    └───────────┘  
                    ^           ^
                Start display   End display
                (active low)    (disable for next cycle)
```

## Current Code Flow Analysis

### Main Loop:
```c
while(1) {
    // Cycle through all 4 scan phases rapidly
    for(uint8_t cycle = 0; cycle < 4; cycle++) {
        dmd_display.scan_cycle = cycle;           // Set which rows to scan
        vma419_scan_display_quarter(&dmd_display); // Load and display data
        _delay_ms(1);                             // 1ms per scan cycle
    }
}
// Total refresh rate: 4ms = 250Hz
```

### Scan Function Flow:
```c
void vma419_scan_display_quarter(VMA419_Display* disp) {
    // 1. Disable output
    PIN_SET_HIGH(disp->pins.oe_port_out, disp->pins.oe_pin_mask);
    
    // 2. Set row selection pins A,B based on scan_cycle
    select_row_pair(disp, disp->scan_cycle);
    
    // 3. Calculate which rows to send data for
    // scan_cycle 0: rows 0,4,8,12
    // scan_cycle 1: rows 1,5,9,13 etc.
    
    // 4. Send data for selected rows via SPI
    for (each row in current scan cycle) {
        for (each byte in row) {
            spi_transfer(byte);  // Bit-bang 8 bits
        }
    }
    
    // 5. Latch the data
    LATCH_pulse();
    
    // 6. Enable output
    PIN_SET_LOW(disp->pins.oe_port_out, disp->pins.oe_pin_mask);
}
```

## Row Selection Logic Issue (FIXED)

### Original Problem:
```c
// WRONG: Direct mapping
A = row_pair & 0x01;  // cycle 0: A=0, B=0
B = row_pair & 0x02;  // cycle 1: A=1, B=0
                      // cycle 2: A=0, B=1  
                      // cycle 3: A=1, B=1

This caused: cycle 0 → displayed row 3 instead of row 0
```

### Fixed Solution:
```c
// CORRECT: Inverted mapping
uint8_t inverted_pair = 3 - row_pair;  // Invert: 0→3, 1→2, 2→1, 3→0
A = inverted_pair & 0x01;
B = inverted_pair & 0x02;

Now: cycle 0 (inverted=3) → A=1,B=1 → correctly displays rows 0,4,8,12
     cycle 1 (inverted=2) → A=0,B=1 → correctly displays rows 1,5,9,13
     cycle 2 (inverted=1) → A=1,B=0 → correctly displays rows 2,6,10,14  
     cycle 3 (inverted=0) → A=0,B=0 → correctly displays rows 3,7,11,15
```

## Test Pattern Visualization

### Current Test (8 pixels in row 0):
```c
for(uint8_t x = 0; x < 8; x++) {
    vma419_set_pixel(&dmd_display, x, 0, 1);  // Turn ON pixels 0-7 in row 0
}
```

### Frame Buffer Contents:
```
Row 0: [11111111][00000000][00000000][00000000]  ← 8 LEDs ON, 24 OFF
       Pixels:   0-7        8-15       16-23      24-31
                 ████████   ........   ........   ........

Row 1-15: All zeros (LEDs OFF)
```

### Expected Display:
```
During scan_cycle 0 (rows 0,4,8,12 active):
ROW 0:  ████████........................  ← 8 LEDs ON
ROW 4:  ................................  ← All OFF  
ROW 8:  ................................  ← All OFF
ROW 12: ................................  ← All OFF

During other scan cycles: All rows OFF
```

## Timing Characteristics

- **Refresh Rate**: 250Hz (4ms per complete frame)
- **Row Display Time**: 1ms per scan cycle
- **SPI Clock**: ~250kHz (2μs high + 2μs low = 4μs period)
- **Latch Pulse**: 10μs duration
- **Data Setup Time**: Sufficient due to slow SPI clock

This multiplexing ensures all LEDs appear continuously lit to the human eye while only consuming power for 1/4 of the LEDs at any given time.
