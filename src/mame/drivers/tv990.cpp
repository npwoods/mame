// license:BSD-3-Clause
// copyright-holders:R. Belmont
/***************************************************************************

  TeleVideo 990 terminal
  
  Preliminary driver by R. Belmont
  
  H/W:
  68000-P16 CPU (clock unknown, above 10 MHz it outruns the AT keyboard controller)
  16C452 dual 16450 (PC/AT standard) UART + PC-compatible Centronics
  AMI MEGA-KBD-H-Q PS/2 keyboard interface
  Televideo ASIC marked "134446-00 TVI1111-0 427"
  3x AS7C256 (32K x 8 SRAM)
  
  IRQs:
  2 = PS/2 keyboard
  3 = Centronics
  4 = UART 1
  5 = UART 0
  6 = VBL (9003b is status, write 3 to 9003b to reset

  Video modes include 80 or 132 wide by 24, 25, 42, 43, 48, or 49 lines high plus an
                      optional status bar
  Modes include TeleVideo 990, 950, and 955, Wyse WY-60, WY-150/120/50+/50, ANSI, 
                      DEC VT320/220, VT100/52, SCO Console, and PC TERM.

  ASIC registers:
  0x01 = possibly width (80 written at startup)
  0x09 = possibly height (50 written at startup; 49 + the status bar)
  0x16 = cursor X position
  
  Status bar is out of the way of used VRAM at offset 0x36b0.  

****************************************************************************/

#include "emu.h"
#include "cpu/m68000/m68000.h"
#include "machine/ins8250.h"
#include "machine/at_keybc.h"
#include "bus/rs232/rs232.h"
#include "bus/pc_kbd/pc_kbdc.h"
#include "bus/pc_kbd/keyboards.h"

#define UART0_TAG		"ns16450_0"
#define UART1_TAG		"ns16450_1"
#define RS232A_TAG      "rs232a"
#define RS232B_TAG      "rs232b"

class tv990_state : public driver_device
{
public:
	tv990_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
		m_maincpu(*this, "maincpu"),
		m_vram(*this, "vram"),
		m_fontram(*this, "fontram"),
		m_uart0(*this, UART0_TAG),
		m_uart1(*this, UART1_TAG),
		m_keybc(*this, "keybc")
	{
	}

	required_device<m68000_device> m_maincpu;
	required_shared_ptr<UINT16> m_vram;
	required_shared_ptr<UINT16> m_fontram;
	required_device<ns16450_device> m_uart0, m_uart1;
	required_device<at_keyboard_controller_device> m_keybc;
		
	virtual void machine_reset() override;
	
	UINT32 screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	
	DECLARE_READ16_MEMBER(tvi1111_r);
	DECLARE_WRITE16_MEMBER(tvi1111_w);
	DECLARE_READ16_MEMBER(keyb_r);
	DECLARE_WRITE16_MEMBER(keyb_w);
	
	WRITE_LINE_MEMBER(keybc_irq);
	WRITE_LINE_MEMBER(uart0_irq);
	WRITE_LINE_MEMBER(uart1_irq);

private:
	UINT16 tvi1111_regs[0xff/2];
};

WRITE_LINE_MEMBER(tv990_state::keybc_irq)
{
	m_maincpu->set_input_line(M68K_IRQ_2, state);
}

WRITE_LINE_MEMBER(tv990_state::uart0_irq)
{
	m_maincpu->set_input_line(M68K_IRQ_5, state);
}

WRITE_LINE_MEMBER(tv990_state::uart1_irq)
{
	m_maincpu->set_input_line(M68K_IRQ_4, state);
}

READ16_MEMBER(tv990_state::tvi1111_r)
{
	if (offset == (0x32/2))
	{
		tvi1111_regs[offset] |= 8;	// loop at 109ca wants this set
	}

	return tvi1111_regs[offset];
}

WRITE16_MEMBER(tv990_state::tvi1111_w)
{
#if 0
	//if ((offset != 0x50) && (offset != 0x68) && (offset != 0x1d))
	{
		if (mem_mask == 0x00ff)
		{
			printf("%x (%d) to ASIC @ %x (mask %04x)\n", data & 0xff, data & 0xff, offset, mem_mask);
		}
		else if (mem_mask == 0xff00)
		{
			printf("%x (%d) to ASIC @ %x (mask %04x)\n", data & 0xff, data & 0xff, offset, mem_mask);
		}
		else
		{
			printf("%x (%d) to ASIC @ %x (mask %04x)\n", data, data, offset, mem_mask);
		}
	}
#endif
	COMBINE_DATA(&tvi1111_regs[offset]);
}

UINT32 tv990_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	UINT32 *scanline;
	int x, y;
	UINT8 pixels, pixels2;
	static const UINT32 palette[2] = { 0, 0xffffff };
	static const UINT32 invpalette[2] = { 0xffffff, 0 };
	UINT16 *vram = (UINT16 *)m_vram.target();
	UINT8 *fontram = (UINT8 *)m_fontram.target();
	UINT16 *curchar;
	UINT8 *fontptr;
	
	for (y = 0; y < 50; y++)
	{
		if (y == 49)
		{
			curchar = &vram[14000/2];
		}
		else
		{
			curchar = &vram[80 * y];
		}
		
		for (x = 0; x < 80; x++)
		{
			fontptr = (UINT8 *)&fontram[((curchar[x]>>8) & 0xff) * 64];
	
			for (int chary = 0; chary < 16; chary++)
			{
				scanline = &bitmap.pix32((y*16)+chary, (x*16));
			
				pixels = *fontptr++;
				pixels2 = *fontptr++;
					
				if (curchar[x] & 0x4)	// inverse video?
				{
					*scanline++ = invpalette[(pixels>>7)&1];
					*scanline++ = invpalette[(pixels2>>7)&1];
					*scanline++ = invpalette[(pixels>>6)&1];
					*scanline++ = invpalette[(pixels2>>6)&1];
					*scanline++ = invpalette[(pixels>>5)&1];
					*scanline++ = invpalette[(pixels2>>5)&1];
					*scanline++ = invpalette[(pixels>>4)&1];
					*scanline++ = invpalette[(pixels2>>4)&1];
					*scanline++ = invpalette[(pixels>>3)&1];
					*scanline++ = invpalette[(pixels2>>3)&1];
					*scanline++ = invpalette[(pixels>>2)&1];
					*scanline++ = invpalette[(pixels2>>2)&1];
					*scanline++ = invpalette[(pixels>>1)&1];
					*scanline++ = invpalette[(pixels2>>1)&1];
					*scanline++ = invpalette[(pixels&1)];
					*scanline++ = invpalette[(pixels2&1)];
				}
				else
				{
					*scanline++ = palette[(pixels>>7)&1];
					*scanline++ = palette[(pixels2>>7)&1];
					*scanline++ = palette[(pixels>>6)&1];
					*scanline++ = palette[(pixels2>>6)&1];
					*scanline++ = palette[(pixels>>5)&1];
					*scanline++ = palette[(pixels2>>5)&1];
					*scanline++ = palette[(pixels>>4)&1];
					*scanline++ = palette[(pixels2>>4)&1];
					*scanline++ = palette[(pixels>>3)&1];
					*scanline++ = palette[(pixels2>>3)&1];
					*scanline++ = palette[(pixels>>2)&1];
					*scanline++ = palette[(pixels2>>2)&1];
					*scanline++ = palette[(pixels>>1)&1];
					*scanline++ = palette[(pixels2>>1)&1];
					*scanline++ = palette[(pixels&1)];
					*scanline++ = palette[(pixels2&1)];
				}
			}	
		}
	}

	return 0;
}

// 2ac = test
READ16_MEMBER(tv990_state::keyb_r)
{
	UINT8 rv = 0;
	
	if (offset == 0)
	{
		rv = m_keybc->data_r(space, 0);
	}
	else
	{
		rv = m_keybc->status_r(space, 0);
	}
	
	return rv;
}

WRITE16_MEMBER(tv990_state::keyb_w)
{
	if (offset == 0)
	{
		m_keybc->data_w(space, 0, data);
	}
	else
	{
		m_keybc->command_w(space, 0, data);
	}
}

static ADDRESS_MAP_START(tv990_mem, AS_PROGRAM, 16, tv990_state)
	AM_RANGE(0x000000, 0x01ffff) AM_ROM AM_REGION("maincpu", 0)
	AM_RANGE(0x060000, 0x063fff) AM_RAM	AM_SHARE("vram") // character/attribute RAM
	AM_RANGE(0X080000, 0X087fff) AM_RAM	AM_SHARE("fontram") // font RAM
	AM_RANGE(0x090000, 0x0900ff) AM_READWRITE(tvi1111_r, tvi1111_w)
	AM_RANGE(0x0a0000, 0x0a000f) AM_DEVREADWRITE8(UART0_TAG, ns16450_device, ins8250_r, ins8250_w, 0x00ff)
	AM_RANGE(0x0a0010, 0x0a001f) AM_DEVREADWRITE8(UART1_TAG, ns16450_device, ins8250_r, ins8250_w, 0x00ff)
	AM_RANGE(0x0b0000, 0x0b0003) AM_READWRITE(keyb_r, keyb_w)
	AM_RANGE(0x0c0000, 0x0c7fff) AM_RAM	// work RAM
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( tv990 )
INPUT_PORTS_END


void tv990_state::machine_reset()
{
}

static MACHINE_CONFIG_START( tv990, tv990_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", M68000, XTAL_10MHz)	// real speed unclear, this is as fast as you can go without outrunning the AT keyboard controller, but real 990 uses an HLE reproduction
	MCFG_CPU_PROGRAM_MAP(tv990_mem)
	MCFG_CPU_VBLANK_INT_DRIVER("screen", tv990_state, irq6_line_hold)
	
	MCFG_SCREEN_ADD("screen", RASTER)
	MCFG_SCREEN_UPDATE_DRIVER(tv990_state, screen_update)
	MCFG_SCREEN_SIZE(132*16, 50*16)
	MCFG_SCREEN_VISIBLE_AREA(0, (80*16)-1, 0, (50*16)-1)
	MCFG_SCREEN_REFRESH_RATE(60)
	
	MCFG_DEVICE_ADD( UART0_TAG, NS16450, XTAL_1_8432MHz )
	MCFG_INS8250_OUT_TX_CB(DEVWRITELINE(RS232A_TAG, rs232_port_device, write_txd))
	MCFG_INS8250_OUT_INT_CB(WRITELINE(tv990_state, uart0_irq))

	MCFG_DEVICE_ADD( UART1_TAG, NS16450, XTAL_1_8432MHz )
	MCFG_INS8250_OUT_TX_CB(DEVWRITELINE(RS232B_TAG, rs232_port_device, write_txd))
	MCFG_INS8250_OUT_INT_CB(WRITELINE(tv990_state, uart0_irq))

	MCFG_RS232_PORT_ADD(RS232A_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, rx_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, dcd_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(UART0_TAG, ns16450_device, cts_w))

	MCFG_RS232_PORT_ADD(RS232B_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(UART1_TAG, ns16450_device, rx_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(UART1_TAG, ns16450_device, dcd_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(UART1_TAG, ns16450_device, cts_w))
	
	MCFG_DEVICE_ADD("keybc", AT_KEYBOARD_CONTROLLER, XTAL_12MHz)
	MCFG_AT_KEYBOARD_CONTROLLER_KEYBOARD_CLOCK_CB(DEVWRITELINE("pc_kbdc", pc_kbdc_device, clock_write_from_mb))
	MCFG_AT_KEYBOARD_CONTROLLER_KEYBOARD_DATA_CB(DEVWRITELINE("pc_kbdc", pc_kbdc_device, data_write_from_mb))
	MCFG_AT_KEYBOARD_CONTROLLER_INPUT_BUFFER_FULL_CB(WRITELINE(tv990_state, keybc_irq))

	MCFG_DEVICE_ADD("pc_kbdc", PC_KBDC, 0)
	MCFG_PC_KBDC_OUT_CLOCK_CB(DEVWRITELINE("keybc", at_keyboard_controller_device, keyboard_clock_w))
	MCFG_PC_KBDC_OUT_DATA_CB(DEVWRITELINE("keybc", at_keyboard_controller_device, keyboard_data_w))
	MCFG_PC_KBDC_SLOT_ADD("pc_kbdc", "kbd", pc_at_keyboards, STR_KBD_MICROSOFT_NATURAL)
MACHINE_CONFIG_END

/* ROM definition */
ROM_START( tv990 )
	ROM_REGION( 0x20000, "maincpu", 0 )
	ROM_LOAD16_BYTE( "180003-89_u3.bin", 0x000000, 0x010000, CRC(0465fc55) SHA1(b8874ce54bf2bf4f77664194d2f23c0e4e6ccbe9) ) 
	ROM_LOAD16_BYTE( "180003-90_u4.bin", 0x000001, 0x010000, CRC(fad7d77d) SHA1(f1114a4a07c8b4ffa0323a2e7ce03d82a386f7d3) ) 
ROM_END

/* Driver */
COMP( 1992, tv990, 0, 0, tv990, tv990, driver_device, 0, "TeleVideo", "TeleVideo 990", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
