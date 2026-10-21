# Vevor Controller Enclosure Design Specification

**Date:** 2026-03-29
**Version:** 1.0
**Designed for:** ESP32 + OLED Display + Rotary Encoder Controller

---

## Box Dimensions (Inner Measurements)

| Dimension | Measurement |
|-----------|-------------|
| **Height** | 20 mm |
| **Length** | 80 mm |
| **Width** | 40 mm |

**Total Internal Volume:** 64,000 mm³ (64 cm³)

---

## Front Panel Layout

### OLED Display
- **Opening Size**: 36 mm × 24 mm (width × height)
- **Vertical Position**: Centered (10 mm from top, 10 mm from bottom)
- **Horizontal Position**: 12 mm from left edge
- **Notes**:
  - Display is 1.3" (128×64 pixels, actual viewing area ~27×16mm)
  - Opening sized for display module bezel
  - Secure with M2 screws or adhesive mount

### Rotary Encoder
- **Location**: Top right of front panel
- **Hole Diameter**: 7 mm (shaft diameter)
- **Center Position**: 12 mm from top edge
- **Horizontal Position**: Calculate from right edge (see diagram)
- **Notes**:
  - Standard rotary encoder shaft: 6mm diameter
  - 7mm hole provides 0.5mm clearance
  - Threaded bushing secures through panel
  - Knob mounts on shaft above panel

### Push Button
- **Hole Diameter**: 10 mm
- **Center Position**: 7 mm from bottom edge
- **Horizontal Alignment**: Same horizontal position as rotary encoder
- **Notes**:
  - Standard 12mm momentary push button
  - Panel mount type with threaded collar
  - LED illuminated button optional

---

## Front Panel Diagram (View from Outside)

```
┌────────────────────────────────────────────────────────────────────────────┐
│                         FRONT PANEL - 80mm × 40mm                          │
├────────────────────────────────────────────────────────────────────────────┤
│                                                                            │
│    12mm                                                    ⊙  ← Encoder   │
│    ↓                                                      12mm from top    │
│ ←─→┌────────────────────────────┐                                         │
│ 12 │                            │                                         │
│ mm │     OLED Display Area      │                                         │
│    │     36mm × 24mm            │                                         │
│    │   (Vertically Centered)    │                                         │
│    │                            │                                         │
│    └────────────────────────────┘                         ⊙  ← Button    │
│                                                          7mm from bottom   │
│                                                                            │
└────────────────────────────────────────────────────────────────────────────┘
      ↑                                                      ↑
   Left Edge                                            Right Side
```

---

## Detailed Component Positioning

### OLED Display Opening
```
Top Edge of Box (0mm)
    ↓
    |
    | 8mm (to center display vertically)
    ↓
    ┌─────────────────────────────┐ ← Top of OLED opening
    │                             │
    │    OLED Display Window      │ 24mm height
    │        36mm × 24mm          │
    │                             │
    └─────────────────────────────┘ ← Bottom of OLED opening
    ↑                             ↑
  12mm                         48mm from left
from left                    (12mm + 36mm)
```

**Coordinates:**
- **Top-left corner**: X=12mm, Y=8mm (from top)
- **Bottom-right corner**: X=48mm, Y=32mm (from top)
- **Center point**: X=30mm, Y=20mm (from top)

### Rotary Encoder Mounting Hole
```
              Right Edge
                  ↓
    ┌─────────────┼──────────────┐
    │             ?mm             │
    │            from            │
 12mm          right edge         │
  ↓             ↓                │
  ⊙─────────────●                │  ← Encoder shaft center
  │           7mm ⌀               │
  │          hole                │
```

**Coordinates:**
- **Y Position**: 12mm from top edge
- **X Position**: TBD - measure from your encoder module placement
  - Typical: 65-70mm from left edge (10-15mm from right edge)
  - Must clear OLED display (48mm from left)
  - Recommend: 68mm from left (12mm from right edge)

**Suggested X Position:** 68mm from left edge (12mm from right edge)

### Push Button Mounting Hole
```
  ⊙ ← Encoder (Y=12mm from top)
  │
  │
  │   Horizontally aligned
  │   (same X coordinate)
  │
  ⊙ ← Button (Y=33mm from top, or 7mm from bottom)
  │
  └─ 7mm from bottom edge
```

**Coordinates:**
- **Y Position**: 33mm from top (or 7mm from bottom edge)
- **X Position**: Same as rotary encoder (68mm from left, suggested)
- **Hole Diameter**: 10mm (for 12mm button body)

---

## Component Spacing & Clearance

### Between OLED and Encoder/Button
- **OLED right edge**: 48mm from left
- **Encoder/Button centerline**: 68mm from left (suggested)
- **Clearance**: 20mm (68 - 48 = 20mm)
- **Status**: ✅ Adequate spacing

### Encoder to Button Vertical Spacing
- **Encoder center**: 12mm from top
- **Button center**: 33mm from top (7mm from bottom)
- **Spacing**: 21mm center-to-center
- **Status**: ✅ Good spacing for operation

### Wall Clearances
- **Top**: 12mm to encoder center (5mm to edge of 7mm hole)
- **Bottom**: 7mm to button center (2mm to edge of 10mm hole)
- **Right**: 12mm to encoder/button center (suggested)
- **Left**: 12mm to OLED left edge
- **Status**: ✅ Adequate for panel mounting

---

## Internal Component Layout

### ESP32 Placement
- **Orientation**: USB port facing bottom or side for programming access
- **Mounting**: Standoffs or double-sided foam tape
- **Height Available**: 20mm internal (ESP32 DevKit ~18mm with USB)
- **Clearance**: Must not obstruct OLED module behind front panel

### OLED Module Mounting
- **Behind Front Panel**: Module sits behind 36×24mm opening
- **Depth**: SH1106 module ~3-4mm thick
- **Mounting**:
  - Option 1: M2 screws through module holes
  - Option 2: Hot glue around edges (non-conductive)
  - Option 3: 3D printed bracket/clips

### Rotary Encoder Mounting
- **Through Panel**: Shaft passes through 7mm hole
- **Securing**: Threaded bushing and nut from inside
- **Wiring**: 5 wires (CLK, DT, SW, +3.3V, GND)
- **PCB**: Small breakout board or direct solder
- **Clearance**: Allow 15mm behind panel for module

### Push Button Mounting
- **Through Panel**: Button body through 10mm hole
- **Securing**: Threaded collar/nut from inside
- **Wiring**: 2 wires (signal + GND)
- **Type**:
  - Momentary normally-open
  - Optional: LED illuminated (requires 2 additional wires)

### Wiring Routing
- **I2C to OLED**: GPIO21 (SDA), GPIO22 (SCL), 3.3V, GND
- **Encoder to ESP32**: GPIO25 (CLK), GPIO26 (DT), GPIO27 (SW), 3.3V, GND
- **Button to ESP32**:
  - Option 1: Separate GPIO (e.g., GPIO33)
  - Option 2: Shared with encoder SW (if mechanically separate control desired)
- **Cable Management**: Use ribbon cable or individual wires with heat shrink
- **Strain Relief**: Hot glue or cable tie anchor points

---

## Material Recommendations

### Front Panel
- **Material Options**:
  - **3D Printed**: PLA or PETG, 2-3mm thick
  - **Laser Cut**: 3mm acrylic (clear or black)
  - **Machined**: 2mm aluminum (painted/anodized)
- **Finish**: Matte or textured for reduced fingerprints
- **Color**: Black recommended (hides electronics, OLED contrast)

### Box Body
- **Material Options**:
  - **3D Printed**: PLA/PETG with 20% infill
  - **Laser Cut**: Acrylic or plywood, glued/bolted assembly
  - **Injection Molded**: ABS (for production quantities)
- **Wall Thickness**: 2-3mm minimum
- **Mounting**:
  - Screw bosses or heat-set inserts for front panel
  - Mounting holes in back for wall/surface mounting

### Hardware
- **Front Panel Mounting**: M3 screws × 4 corners (or snap-fit clips)
- **Component Mounting**: M2 screws for OLED, M2.5 for ESP32
- **Encoder/Button**: Included nuts/washers with components
- **Cable Gland**: Optional rubber grommet for external wiring (power, sensors)

---

## Assembly Notes

### Order of Assembly
1. **Prepare components**: Solder wires to encoder, button, OLED (if needed)
2. **Mount front panel components**:
   - Insert encoder through 7mm hole, secure with nut from inside
   - Insert button through 10mm hole, secure with collar/nut
   - Position OLED behind window opening
3. **Mount ESP32**: Use standoffs or foam tape on back panel/bottom
4. **Connect wiring**: All components to ESP32 GPIO pins
5. **Test functionality**: Power on and verify all components work
6. **Close enclosure**: Secure front panel with screws or clips
7. **Apply labels**: Label encoder function, button, display (optional)

### Wiring Best Practices
- **Use colored wires**: Red=3.3V, Black=GND, others for signals
- **Label wires**: Use heat shrink or tape labels
- **Secure bundles**: Zip ties or twist together, avoid loose wires
- **Strain relief**: Hot glue or tie-down points at panel penetrations
- **Test before closing**: Verify all functions before final assembly

### Design Variations
- **Recessed Display**: Countersink OLED opening for flush mount
- **Viewing Angle**: Tilt display 10-15° by angling mounting
- **Button Types**:
  - Single function (power toggle)
  - Mode selector (RF/Serial/Hybrid)
  - Emergency stop
- **Additional Openings**:
  - Status LED holes (3mm × 3-4 LEDs)
  - Buzzer grille (5mm holes pattern)
  - External sensor ports (4-6mm cable glands)

---

## Dimensions Summary Table

| Feature | X Position | Y Position | Size/Diameter | Notes |
|---------|------------|------------|---------------|-------|
| **Box Inner** | - | - | 80×40×20mm | Length×Width×Height |
| **OLED Opening** | 12mm from left | 8mm from top | 36×24mm | Centered vertically |
| **Encoder Hole** | 68mm from left* | 12mm from top | 7mm ⌀ | Top right position |
| **Button Hole** | 68mm from left* | 33mm from top | 10mm ⌀ | Aligned with encoder |

*Suggested position - adjust based on your preference

---

## Technical Drawing Notes

### For 3D Printing (STL/CAD)
- **Export separate files**:
  - `enclosure_front_panel.stl`
  - `enclosure_main_body.stl`
  - `enclosure_back_cover.stl` (optional)
- **Print orientation**:
  - Front panel: Face down for best finish
  - Body: Opening up to minimize supports
- **Supports**: Required for overhangs >45°
- **Layer height**: 0.2mm standard, 0.1mm for better finish

### For Laser Cutting (DXF/AI)
- **Export 2D profiles**:
  - Front panel with all holes/openings
  - Side panels (40×20mm)
  - Top/bottom panels (80×40mm)
- **Kerf compensation**: Add 0.1-0.2mm to inside cuts
- **Assembly**: Finger joints, slots, or butt joints with glue
- **Ventilation**: Consider adding vent slots/holes in top or sides

### For CNC Machining
- **Material**: Aluminum 6061-T6, 2mm thick for panel
- **Tool paths**:
  - OLED opening: 3mm end mill, multiple passes
  - Holes: Center drill + appropriate drill bits
- **Deburring**: Required on all edges
- **Finishing**: Anodize, powder coat, or brush finish

---

## Files to Create

For a complete design package, create:

### Documentation
- [ ] This specification file (✅ DONE)
- [ ] Component BOM (Bill of Materials)
- [ ] Wiring diagram
- [ ] Assembly instructions with photos

### Design Files
- [ ] CAD model (FreeCAD, Fusion 360, OnShape)
  - [ ] Front panel with accurate hole placement
  - [ ] Main box body
  - [ ] Back cover (if applicable)
- [ ] 2D technical drawings (PDF with dimensions)
- [ ] STL files for 3D printing (if using)
- [ ] DXF/AI files for laser cutting (if using)

### Manufacturing
- [ ] Print/cut test piece (verify hole positions)
- [ ] Dry fit all components before final assembly
- [ ] Document any adjustments needed

---

## Design Validation Checklist

Before manufacturing:
- [ ] OLED opening accommodates display module (36×24mm)
- [ ] Encoder hole (7mm) fits shaft with clearance
- [ ] Button hole (10mm) accommodates button body
- [ ] All components clear each other (no interference)
- [ ] ESP32 fits inside 80×40×20mm box
- [ ] USB port accessible for programming
- [ ] Wiring can route without sharp bends
- [ ] Panel thickness supports component mounting
- [ ] Enclosure provides adequate ventilation
- [ ] Design allows for future modifications

After assembly:
- [ ] All components mount securely
- [ ] Display visible and readable
- [ ] Encoder rotates smoothly
- [ ] Button presses reliably
- [ ] No rattling or loose parts
- [ ] Professional appearance
- [ ] Labels/markings if desired

---

## Example Component BOM

| Qty | Component | Specification | Notes |
|-----|-----------|---------------|-------|
| 1 | ESP32 DevKit | ESP32-WROOM-32 | Main controller |
| 1 | OLED Display | SH1106, 1.3", I2C | 128×64 pixels |
| 1 | Rotary Encoder | KY-040 or similar | With push button |
| 1 | Push Button | 12mm momentary | Panel mount, NO |
| 1 | Enclosure | 80×40×20mm | Custom (3D print/laser cut) |
| 4 | M3 Screws | 10-15mm length | Panel mounting |
| 4 | M3 Standoffs | 5-10mm height | ESP32 mounting |
| 2 | 4.7kΩ Resistors | 1/4W through-hole | I2C pull-ups |
| 1 | Encoder Knob | 6mm shaft | Aluminum or plastic |
| - | Wire | 22-26 AWG | Various colors |
| - | Heat Shrink | Various sizes | Wire management |

---

## Future Enhancements

### Display
- Add protective clear window over OLED opening
- Sunlight-readable transflective display option
- Adjustable display brightness (PWM control)

### Controls
- Second rotary encoder (dual parameter control)
- RGB LED ring around encoder (status indication)
- Capacitive touch buttons (no moving parts)
- E-paper display for always-on status

### Enclosure
- IP54/IP65 rated gasket sealing (outdoor use)
- DIN rail mounting clips (electrical panel mount)
- VESA mounting holes (monitor/wall mount)
- Magnetic back plate (metal surface mount)

---

**Design Status:** 📐 Specification Complete
**Next Step:** Create CAD model or order laser cutting service
**Estimated Manufacturing Time:**
- 3D Print: 4-6 hours
- Laser Cut: 15-30 minutes
- CNC Machine: 1-2 hours

---

**Document Version:** 1.0
**Created:** 2026-03-29
**Author:** User specifications + Claude AI documentation
**License:** Personal use
