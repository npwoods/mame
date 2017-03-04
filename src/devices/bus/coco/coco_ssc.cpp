// license:BSD-3-Clause
// copyright-holders:tim lindner
/***************************************************************************

    coco_ssc.c

    Code for emulating the CoCo Speech / Sound Cartridge

    The SSC was a complex sound cartridge. A TMS-7040 microcontroller,
    SPO256-AL2 speech processor, AY-3-8913 programable sound generator, and
    2K of RAM.

	All four ports of the microcontroller are under software control.

	Port A is input from the host CPU.
	Port B is A0-A7 for the 2k of RAM.
	Port C is the internal bus controller:
		bit 7 6 5 4 3 2 1 0
			| | | | | | | |
			| | | | | | | + A8 for RAM and BC1 of AY3-8913
			| | | | | | +-- A9 for RAM
			| | | | | +---- A10 for RAM
			| | | | +------ R/W* for RAM and BDIR for AY3-8913
			| | | +-------- CS* for RAM
			| | +---------- ALD* for SP0256
			| +------------ CS* for AY3-8913
			+-------------- BUSY* for host CPU (connects to a latch)
		* – Active low
	Port D is the 8-bit data bus.

***************************************************************************/

#include "emu.h"
#include "coco_ssc.h"
#include "cpu/tms7000/tms7000.h"
#include "speaker.h"

#define PIC_TAG "pic7040"
#define AY_TAG "cocossc_ay"
#define SP0256_TAG "sp0256"

#define C_A8   0x01
#define C_BC1  0x01
#define C_A9   0x02
#define C_A10  0x04
#define C_RRW  0x08
#define C_BDR  0x08
#define C_RCS  0x10
#define C_ALD  0x20
#define C_ACS  0x40
#define C_BSY  0x80

static ADDRESS_MAP_START( ssc_io_map, AS_IO, 8, coco_ssc_device )
	AM_RANGE(TMS7000_PORTA, TMS7000_PORTA) AM_READ(ssc_port_a_r)
	AM_RANGE(TMS7000_PORTB, TMS7000_PORTB) AM_WRITE(ssc_port_b_w)
	AM_RANGE(TMS7000_PORTC, TMS7000_PORTC) AM_READWRITE(ssc_port_c_r, ssc_port_c_w)
	AM_RANGE(TMS7000_PORTD, TMS7000_PORTD) AM_READWRITE(ssc_port_d_r, ssc_port_d_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssc_rom, AS_PROGRAM, 8, coco_ssc_device )
	AM_RANGE(0xf000, 0xffff) AM_REGION(PIC_TAG, 0)
ADDRESS_MAP_END

static MACHINE_CONFIG_FRAGMENT( coco_ssc )
	MCFG_CPU_ADD(PIC_TAG, TMS7040, XTAL_3_579545MHz / 2)
	MCFG_CPU_PROGRAM_MAP(ssc_rom)
	MCFG_CPU_IO_MAP(ssc_io_map)

	MCFG_RAM_ADD("staticram")
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_DEFAULT_VALUE(0x00)

	MCFG_SPEAKER_STANDARD_MONO("sscmono")
	MCFG_SOUND_ADD(AY_TAG, AY8913, XTAL_3_579545MHz / 4)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "sscmono", 0.5)

	MCFG_SOUND_ADD(SP0256_TAG, SP0256, XTAL_3_12MHz) // ???
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "sscmono", 0.5)
	MCFG_SP0256_DATA_REQUEST_CB(INPUTLINE(PIC_TAG, TMS7000_INT1_LINE))
//	MCFG_SP0256_DATA_REQUEST_CB(WRITELINE(coco_ssc_device, lrq_cb))

MACHINE_CONFIG_END

ROM_START( coco_ssc )
	ROM_REGION( 0x1000, PIC_TAG, 0 )
	ROM_LOAD( "cocossp.bin", 0x0000, 0x1000, CRC(a8e2eb98) SHA1(7c17dcbc21757535ce0b3a9e1ce5ca61319d3606) ) // ssc cpu rom
	ROM_REGION( 0x10000, SP0256_TAG, 0 )
	ROM_LOAD( "sp0256-al2.bin", 0x1000, 0x0800, CRC(b504ac15) SHA1(e60fcb5fa16ff3f3b69d36c7a6e955744d3feafc) )
ROM_END

//**************************************************************************
//  GLOBAL VARIABLES
//**************************************************************************

const device_type COCO_SSC = device_creator<coco_ssc_device>;

//**************************************************************************
//  LIVE DEVICE
//**************************************************************************

//-------------------------------------------------
//  coco_ssc_device - constructor
//-------------------------------------------------

coco_ssc_device::coco_ssc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
		: device_t(mconfig, COCO_SSC, "CoCo S/SC PAK", tag, owner, clock, "coco_ssc", __FILE__),
		device_cococart_interface(mconfig, *this ),
		m_tms7040(*this, PIC_TAG),
		m_staticram(*this, "staticram"),
		m_ay(*this, AY_TAG),
		m_spo(*this, SP0256_TAG)
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void coco_ssc_device::device_start()
{
	// install $FF7D-E handler
	write8_delegate wh = write8_delegate(FUNC(coco_ssc_device::ff7d_write), this);
	read8_delegate rh = read8_delegate(FUNC(coco_ssc_device::ff7d_read), this);
	machine().device(":maincpu")->memory().space(AS_PROGRAM).install_readwrite_handler(0xFF7D, 0xFF7E, rh, wh);

	save_item(NAME(reset_line));
	save_item(NAME(tms7000_porta));
	save_item(NAME(tms7000_portb));
	save_item(NAME(tms7000_portc));
	save_item(NAME(tms7000_portd));
}

//-------------------------------------------------
//  machine_config_additions - device-specific
//  machine configurations
//-------------------------------------------------

machine_config_constructor coco_ssc_device::device_mconfig_additions() const
{
	return MACHINE_CONFIG_NAME( coco_ssc );
}

//-------------------------------------------------
//  rom_region - device-specific ROM region
//-------------------------------------------------

const tiny_rom_entry *coco_ssc_device::device_rom_region() const
{
	return ROM_NAME( coco_ssc );
}

/*-------------------------------------------------
    read
-------------------------------------------------*/

READ8_MEMBER(coco_ssc_device::ff7d_read)
{
	uint8_t data = 0x00;

	switch(offset)
	{
		case 0x00:
			break;

		case 0x01:

			if( tms7000_portc & C_BSY )
			{
				data |= 0x80;
			}

			if( m_spo->sby_r() )
			{
				data |= 0x40;
			}

			break;
	}

	return data;
}

/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(coco_ssc_device::ff7d_write)
{
	switch(offset)
	{
		case 0x00:
			if( (reset_line & 1) == 1 )
			{
				if( (data & 1) == 0 )
				{
					m_tms7040->reset();
					m_ay->reset_w(space, 0, 0);
				}
			}

			reset_line = data;
			break;

		case 0x01:
			tms7000_porta = data;
			m_tms7040->set_input_line(TMS7000_INT3_LINE, ASSERT_LINE);
			m_tms7040->set_input_line(TMS7000_INT3_LINE, CLEAR_LINE);
			break;
	}
}

//-------------------------------------------------
//  Handlers for secondary CPU ports
//-------------------------------------------------

READ8_MEMBER(coco_ssc_device::ssc_port_a_r)
{
	return tms7000_porta;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_b_w)
{
// 	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
// 	{
// 		if( (tms7000_portc & C_RRW) == 0 ) /* static ram chip write (low) */
// 		{
// 			m_staticram->write(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + data, tms7000_portd);
// 		}
// 	}

	tms7000_portb = data;
}

READ8_MEMBER(coco_ssc_device::ssc_port_c_r)
{
	return tms7000_portc;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_c_w)
{
	int ramr_flag = 0,ramw_flag = 0, psg_flag = 0, spo_flag = 0;
	static uint16_t address;

// 	if( (tms7000_portc & C_RCS) == C_RCS) /* edge of ram chip select */
// 	{
// 		if( (data & C_RCS) == 0 && (data & C_RRW) == 0) /* static RAM write */
// 		{
// 			m_staticram->write(((data & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, tms7000_portd);
// 			ram_flag = 1;
// 		}
// 	}

	if( (tms7000_portc & C_RCS) == C_RCS )
	{
		if( (data & C_RCS) == 0 )
		{
			address = ((data & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb;
			logerror( "ram address: %04.4x\n", address );

			if( (data & C_RRW) == 0 )
			{
				m_staticram->write(address, tms7000_portd);
				ramw_flag = 1;
			}
			else
			{
				tms7000_portd = m_staticram->read(address);
				ramr_flag = 1;
			}
		}
	}

	if( (tms7000_portc & C_RRW) == 0) /* edge of ram read */
	{
		if( (data & C_RCS) == 0 && (data & C_RRW) == C_RRW) /* static RAM read */
		{
			tms7000_portd = m_staticram->read(address);
			ramr_flag = 1;
		}
	}

	if( (tms7000_portc & C_RRW) == C_RRW) /* edge of ram write */
	{
		if( (data & C_RCS) == 0 && (data & C_RRW) == 0) /* static RAM write */
		{
			m_staticram->write(address, tms7000_portd);
			ramw_flag = 1;
		}
	}

	if( (tms7000_portc & C_ACS) == C_ACS )
	{
		if( (data & C_ACS) == 0 ) /* chip select for AY-3-8913 */
		{
			if( (data & (C_BDR|C_BC1)) == (C_BDR|C_BC1) ) /* BDIR = 1, BC1 = 1: latch address */
			{
				m_ay->address_w(space, 0, tms7000_portd);
				psg_flag = 1;
			}

			if( ((data & C_BDR) == C_BDR) && ((data & C_BC1) == 0) ) /* BDIR = 1, BC1 = 0: write data */
			{
				m_ay->data_w(space, 0, tms7000_portd);
				psg_flag = 1;
			}
		}
	}

	if( (tms7000_portc & C_ALD) == C_ALD )
	{
		if( (data & C_ALD) == 0 )
		{
			m_spo->ald_w(space, 0, tms7000_portd);
			spo_flag = 1;
		}
	}

 	if( ramr_flag + ramw_flag + psg_flag + spo_flag > 1 )
 	{
		logerror( "ramr:%d, ramw:%d, psg:%d, spo:%d (%02.2x -> %02.2x)!\n", ramr_flag, ramw_flag, psg_flag, spo_flag, tms7000_portc, data );
 	}
 	else
 	{
		logerror( "ramr:%d, ramw:%d, psg:%d, spo:%d (%02.2x -> %02.2x)\n", ramr_flag, ramw_flag, psg_flag, spo_flag, tms7000_portc, data );
 	}

	tms7000_portc = data;
}

READ8_MEMBER(coco_ssc_device::ssc_port_d_r)
{
 	uint8_t data = 0x00;
//
// 	if( ((tms7000_portc & C_RCS) == 0) && ((tms7000_portc & C_ACS) == 0))
// 		logerror( "Reading RAM and PSG at the same time!\n" );
//
// 	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
// 	{
// 		if( (tms7000_portc & C_RRW) == C_RRW ) /* static ram chip read (high) */
// 		{
// 			data = m_staticram->read(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb);
// 		}
// 	}
//
// 	if( (tms7000_portc & C_ACS) == 0 ) /* chip select for AY-3-8913 */
// 	{
// 		if( ((tms7000_portc & C_BDR) == 0) && ((tms7000_portc & C_BC1) == C_BC1) ) /* read data */
// 		{
// 			data = m_ay->data_r(space, 0);
// 		}
// 	}

	data = tms7000_portd;
	logerror( " port d read: %02.2x\n", data );

	return data;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_d_w)
{
// 	if( ((tms7000_portc & C_RCS) == 0) && ((tms7000_portc & C_ACS) == 0))
// 		logerror( "writing RAM and PSG at the same time!\n" );
//
// 	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
// 	{
// 		if( (tms7000_portc & C_RRW) == 0 ) /* static ram chip write (low) */
// 		{
// 			m_staticram->write(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, data);
// 		}
// 	}
//
// 	if( (tms7000_portc & C_ACS) == 0 ) /* chip select for the AY-3-8913 */
// 	{
// 		if( (tms7000_portc & (C_BDR|C_BC1)) == (C_BDR|C_BC1) ) /* BDIR = 1, BC1 = 1: latch address */
// 		{
// 			m_ay->address_w(space, 0, data);
// 		}
//
// 		if( ((tms7000_portc & C_BDR) == C_BDR) && ((tms7000_portc & C_BC1) == 0) ) /* BDIR = 1, BC1 = 0: write data */
// 		{
// 			m_ay->data_w(space, 0, data);
// 		}
// 	}
//
// 	if( (tms7000_portc & C_ALD) == 0 )
// 	{
// 		m_spo->ald_w(space, 0, data);
// 	}

	logerror( "port d write: %02.2x\n", data );
	tms7000_portd = data;
}

WRITE_LINE_MEMBER(coco_ssc_device::lrq_cb)
{
	if( state == 0 )
	{
		m_tms7040->set_input_line(TMS7000_INT1_LINE, ASSERT_LINE);
		m_tms7040->set_input_line(TMS7000_INT1_LINE, CLEAR_LINE);
	}
}
