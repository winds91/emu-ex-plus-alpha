#include "core/gba/gbaGfx.h"

#include "core/gba/gbaGlobals.h"

#define BLDMOD ioMem.BLDMOD
#define COLEV ioMem.COLEV
#define COLY ioMem.COLY
#define VCOUNT ioMem.VCOUNT
#define MOSAIC ioMem.MOSAIC
#define DISPCNT ioMem.DISPCNT
#define coreOptions lcd
#define BG0CNT ioMem.BG0CNT
#define BG0HOFS ioMem.BG0HOFS
#define BG0VOFS ioMem.BG0VOFS
#define g_line0 lcd.line0
#define BG1CNT ioMem.BG1CNT
#define BG1HOFS ioMem.BG1HOFS
#define BG1VOFS ioMem.BG1VOFS
#define g_line1 lcd.line1
#define BG2CNT ioMem.BG2CNT
#define BG2HOFS ioMem.BG2HOFS
#define BG2VOFS ioMem.BG2VOFS
#define g_line2 lcd.line2
#define BG3CNT ioMem.BG3CNT
#define BG3HOFS ioMem.BG3HOFS
#define BG3VOFS ioMem.BG3VOFS
#define g_line3 lcd.line3
#define g_lineOBJ lcd.lineOBJ
#define g_lineOBJWin lcd.lineOBJWin
#define WIN0V ioMem.WIN0V
#define WIN1V ioMem.WIN1V
#define WININ ioMem.WININ
#define WINOUT ioMem.WINOUT
#define gfxBG2Changed lcd.gfxBG2Changed
#define gfxBG3Changed lcd.gfxBG3Changed
#define gfxLastVCOUNT lcd.gfxLastVCOUNT
#define gfxInWin0 lcd.gfxInWin0
#define gfxInWin1 lcd.gfxInWin1
#define gfxDrawTextScreen(BGCNT, BGHOFS, BGVOFS, line) gfxDrawTextScreen(lcd.vram, BGCNT, BGHOFS, BGVOFS, line, VCOUNT, MOSAIC, palette)
#define gfxDrawSprites(g_lineOBJ) gfxDrawSprites(lcd, g_lineOBJ, VCOUNT, MOSAIC, DISPCNT)
#define gfxDrawOBJWin(g_lineOBJWin) gfxDrawOBJWin(lcd, g_lineOBJWin, VCOUNT, DISPCNT)

void mode1RenderLine(MixColorType *g_lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  if (coreOptions.layerEnable & 0x0100) {
    gfxDrawTextScreen(BG0CNT, BG0HOFS, BG0VOFS, g_line0);
  }

  if (coreOptions.layerEnable & 0x0200) {
    gfxDrawTextScreen(BG1CNT, BG1HOFS, BG1VOFS, g_line1);
  }

  if (coreOptions.layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;
    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
    		ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD,
    		lcd.gfxBG2X, lcd.gfxBG2Y, changed, g_line2, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(g_lineOBJ);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;

    if (g_line0[x] < color) {
      color = g_line0[x];
      top = 0x01;
    }

    if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_line1[x];
      top = 0x02;
    }

    if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_line2[x];
      top = 0x04;
    }

    if ((uint8_t)(g_lineOBJ[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_lineOBJ[x];
      top = 0x10;
    }

    if ((top & 0x10) && (color & 0x00010000)) {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((uint8_t)(g_line0[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line0[x];
        top2 = 0x01;
      }

      if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line1[x];
        top2 = 0x02;
      }

      if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line2[x];
        top2 = 0x04;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              g_coeff[COLEV & 0x1F],
                              g_coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        }
      }
    }

    g_lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}

void mode1RenderLineNoWindow(MixColorType *g_lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  if (coreOptions.layerEnable & 0x0100) {
    gfxDrawTextScreen(BG0CNT, BG0HOFS, BG0VOFS, g_line0);
  }


  if (coreOptions.layerEnable & 0x0200) {
    gfxDrawTextScreen(BG1CNT, BG1HOFS, BG1VOFS, g_line1);
  }

  if (coreOptions.layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;
    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
    		ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD,
    		lcd.gfxBG2X, lcd.gfxBG2Y, changed, g_line2, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(g_lineOBJ);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;

    if (g_line0[x] < color) {
      color = g_line0[x];
      top = 0x01;
    }

    if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_line1[x];
      top = 0x02;
    }

    if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_line2[x];
      top = 0x04;
    }

    if ((uint8_t)(g_lineOBJ[x] >> 24) < (uint8_t)(color >> 24)) {
      color = g_lineOBJ[x];
      top = 0x10;
    }

    if (!(color & 0x00010000)) {
      switch ((BLDMOD >> 6) & 3) {
      case 0:
        break;
      case 1: {
          if (top & BLDMOD) {
            uint32_t back = backdrop;
            uint8_t top2 = 0x20;
            if ((uint8_t)(g_line0[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x01) {
                back = g_line0[x];
                top2 = 0x01;
              }
            }

            if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x02) {
                back = g_line1[x];
                top2 = 0x02;
              }
            }

            if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x04) {
                back = g_line2[x];
                top2 = 0x04;
              }
            }

            if ((uint8_t)(g_lineOBJ[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x10) {
                back = g_lineOBJ[x];
                top2 = 0x10;
              }
            }

            if (top2 & (BLDMOD >> 8))
              color = gfxAlphaBlend(color, back,
                                    g_coeff[COLEV & 0x1F],
                                    g_coeff[(COLEV >> 8) & 0x1F]);
          }
        } break;
      case 2:
        if (BLDMOD & top)
          color = gfxIncreaseBrightness(color, g_coeff[COLY & 0x1F]);
        break;
      case 3:
        if (BLDMOD & top)
          color = gfxDecreaseBrightness(color, g_coeff[COLY & 0x1F]);
        break;
      }
    } else {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((uint8_t)(g_line0[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line0[x];
        top2 = 0x01;
      }

      if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line1[x];
        top2 = 0x02;
      }

      if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line2[x];
        top2 = 0x04;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              g_coeff[COLEV & 0x1F],
                              g_coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        }
      }
    }

    g_lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}

void mode1RenderLineAll(MixColorType *g_lineMix, GBALCD &lcd, const GBAMem::IoMem &ioMem)
{
  const uint16_t *palette = (uint16_t *)lcd.paletteRAM;

  bool inWindow0 = false;
  bool inWindow1 = false;

  if (coreOptions.layerEnable & 0x2000) {
    uint8_t v0 = WIN0V >> 8;
    uint8_t v1 = WIN0V & 255;
    inWindow0 = ((v0 == v1) && (v0 >= 0xe8));
    if (v1 >= v0)
      inWindow0 |= (VCOUNT >= v0 && VCOUNT < v1);
    else
      inWindow0 |= (VCOUNT >= v0 || VCOUNT < v1);
  }
  if (coreOptions.layerEnable & 0x4000) {
    uint8_t v0 = WIN1V >> 8;
    uint8_t v1 = WIN1V & 255;
    inWindow1 = ((v0 == v1) && (v0 >= 0xe8));
    if (v1 >= v0)
      inWindow1 |= (VCOUNT >= v0 && VCOUNT < v1);
    else
      inWindow1 |= (VCOUNT >= v0 || VCOUNT < v1);
  }

  if (coreOptions.layerEnable & 0x0100) {
    gfxDrawTextScreen(BG0CNT, BG0HOFS, BG0VOFS, g_line0);
  }

  if (coreOptions.layerEnable & 0x0200) {
    gfxDrawTextScreen(BG1CNT, BG1HOFS, BG1VOFS, g_line1);
  }

  if (coreOptions.layerEnable & 0x0400) {
    int changed = gfxBG2Changed;
    if (gfxLastVCOUNT > VCOUNT)
      changed = 3;
    gfxDrawRotScreen(lcd.vram, BG2CNT, ioMem.BG2X_L, ioMem.BG2X_H, ioMem.BG2Y_L, ioMem.BG2Y_H,
    		ioMem.BG2PA, ioMem.BG2PB, ioMem.BG2PC, ioMem.BG2PD,
    		lcd.gfxBG2X, lcd.gfxBG2Y, changed, g_line2, VCOUNT, MOSAIC, palette);
  }

  gfxDrawSprites(g_lineOBJ);
  gfxDrawOBJWin(g_lineOBJWin);

  uint32_t backdrop;
  if (customBackdropColor == -1) {
    backdrop = (READ16LE(&palette[0]) | 0x30000000);
  } else {
    backdrop = ((customBackdropColor & 0x7FFF) | 0x30000000);
  }

  uint8_t inWin0Mask = WININ & 0xFF;
  uint8_t inWin1Mask = WININ >> 8;
  uint8_t outMask = WINOUT & 0xFF;

  for (int x = 0; x < 240; x++) {
    uint32_t color = backdrop;
    uint8_t top = 0x20;
    uint8_t mask = outMask;

    if (!(g_lineOBJWin[x] & 0x80000000)) {
      mask = WINOUT >> 8;
    }

    if (inWindow1) {
      if (gfxInWin1[x])
        mask = inWin1Mask;
    }

    if (inWindow0) {
      if (gfxInWin0[x]) {
        mask = inWin0Mask;
      }
    }

    if (g_line0[x] < color && (mask & 1)) {
      color = g_line0[x];
      top = 0x01;
    }

    if ((uint8_t)(g_line1[x] >> 24) < (uint8_t)(color >> 24) && (mask & 2)) {
      color = g_line1[x];
      top = 0x02;
    }

    if ((uint8_t)(g_line2[x] >> 24) < (uint8_t)(color >> 24) && (mask & 4)) {
      color = g_line2[x];
      top = 0x04;
    }

    if ((uint8_t)(g_lineOBJ[x] >> 24) < (uint8_t)(color >> 24) && (mask & 16)) {
      color = g_lineOBJ[x];
      top = 0x10;
    }

    if (color & 0x00010000) {
      // semi-transparent OBJ
      uint32_t back = backdrop;
      uint8_t top2 = 0x20;

      if ((mask & 1) && (uint8_t)(g_line0[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line0[x];
        top2 = 0x01;
      }

      if ((mask & 2) && (uint8_t)(g_line1[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line1[x];
        top2 = 0x02;
      }

      if ((mask & 4) && (uint8_t)(g_line2[x] >> 24) < (uint8_t)(back >> 24)) {
        back = g_line2[x];
        top2 = 0x04;
      }

      if (top2 & (BLDMOD >> 8))
        color = gfxAlphaBlend(color, back,
                              g_coeff[COLEV & 0x1F],
                              g_coeff[(COLEV >> 8) & 0x1F]);
      else {
        switch ((BLDMOD >> 6) & 3) {
        case 2:
          if (BLDMOD & top)
            color = gfxIncreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        case 3:
          if (BLDMOD & top)
            color = gfxDecreaseBrightness(color, g_coeff[COLY & 0x1F]);
          break;
        }
      }
    } else if (mask & 32) {
      // special FX on the window
      switch ((BLDMOD >> 6) & 3) {
      case 0:
        break;
      case 1: {
          if (top & BLDMOD) {
            uint32_t back = backdrop;
            uint8_t top2 = 0x20;

            if ((mask & 1) && (uint8_t)(g_line0[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x01) {
                back = g_line0[x];
                top2 = 0x01;
              }
            }

            if ((mask & 2) && (uint8_t)(g_line1[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x02) {
                back = g_line1[x];
                top2 = 0x02;
              }
            }

            if ((mask & 4) && (uint8_t)(g_line2[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x04) {
                back = g_line2[x];
                top2 = 0x04;
              }
            }

            if ((mask & 16) && (uint8_t)(g_lineOBJ[x] >> 24) < (uint8_t)(back >> 24)) {
              if (top != 0x10) {
                back = g_lineOBJ[x];
                top2 = 0x10;
              }
            }

            if (top2 & (BLDMOD >> 8))
              color = gfxAlphaBlend(color, back,
                                    g_coeff[COLEV & 0x1F],
                                    g_coeff[(COLEV >> 8) & 0x1F]);
          }
        } break;
      case 2:
        if (BLDMOD & top)
          color = gfxIncreaseBrightness(color, g_coeff[COLY & 0x1F]);
        break;
      case 3:
        if (BLDMOD & top)
          color = gfxDecreaseBrightness(color, g_coeff[COLY & 0x1F]);
        break;
      }
    }

    g_lineMix[x] = color;
  }
  gfxBG2Changed = 0;
  gfxLastVCOUNT = VCOUNT;
}
