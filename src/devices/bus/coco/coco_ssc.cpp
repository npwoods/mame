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

#define AY_TAG "cocossc_ay"

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
	AM_RANGE(0xf000, 0xffff) AM_REGION("pic7040", 0)
ADDRESS_MAP_END

static MACHINE_CONFIG_FRAGMENT( coco_ssc )
	MCFG_CPU_ADD("pic7040", TMS7040, XTAL_3_579545MHz / 2)
	MCFG_CPU_PROGRAM_MAP(ssc_rom)
	MCFG_CPU_IO_MAP(ssc_io_map)

	MCFG_RAM_ADD("staticram")
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_DEFAULT_VALUE(0x00)

	MCFG_SPEAKER_STANDARD_MONO("aymono")
	MCFG_SOUND_ADD(AY_TAG, AY8913, XTAL_3_579545MHz / 4)
	MCFG_AY8910_OUTPUT_TYPE(AY8910_SINGLE_OUTPUT)
	MCFG_SOUND_ROUTE(ALL_OUTPUTS, "aymono", 0.5)


MACHINE_CONFIG_END

ROM_START( coco_ssc )
	ROM_REGION( 0x1000, "pic7040", 0 )
	ROM_LOAD( "cocossp.bin", 0x0000, 0x1000, CRC(a8e2eb98) SHA1(7c17dcbc21757535ce0b3a9e1ce5ca61319d3606) ) // ssc cpu rom
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
		m_tms7040(*this, "pic7040"),
		m_staticram(*this, "staticram"),
		m_ay(*this, AY_TAG)
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
	logerror("coco_ssc_device::read offset: %4.4X\n", offset);

	switch(offset)
	{
		case 0x00:
			break;

		case 0x01:
			return tms7000_portc & C_BSY;
			break;
	}

	return 0;
}

/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(coco_ssc_device::ff7d_write)
{
	logerror("coco_ssc_device::write offset: %4.4X, data: %2.2X\n", offset, data);

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
	logerror("port_a_r: read offset: %4.4X\n", offset);
	return tms7000_porta;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_b_w)
{
	logerror("port_b_w: write offset: %4.4X, data: %2.2X\n", offset, data);

	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
	{
		if( (tms7000_portc & C_RRW) == 0 ) /* static ram chip write (low) */
		{
			m_staticram->write(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + data, tms7000_portd);
			logerror("write static ram offset: %u, data: %u\n", ((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + data, tms7000_portd );
		}
	}

	tms7000_portb = data;
}

READ8_MEMBER(coco_ssc_device::ssc_port_c_r)
{
	logerror("port_c_r: read offset: %4.4X\n", offset);
	return tms7000_portc;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_c_w)
{
	logerror("port_c_w: write offset: %4.4X, data: %2.2X\n", offset, data);

	if( (data & C_RCS) == 0 && (data & C_RRW) == C_RRW) logerror( "RAM selected for reading\n" );
	if( (data & C_RCS) == 0 && (data & C_RRW) == 0)
	{
		logerror( "RAM selected for writing\n" );
		m_staticram->write(((data & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, tms7000_portd);
		logerror("write static ram offset: %u, data: %u\n", ((data & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, tms7000_portd );
	}

	if( (data & C_ALD) == 0 ) logerror( "SPO256 selected\n" );
	if( (data & C_ACS) == 0 ) /* chip select for AY-3-8913 */
	{
		logerror( "AY3-8913 selected\n" );

		if( (data & (C_BDR|C_BC1)) == (C_BDR|C_BC1) ) /* BDIR = 1, BC1 = 1: latch address */
		{
			m_ay->address_w(space, 0, tms7000_portd);
			logerror( "AY3-8913 writing address: %u\n", tms7000_portd );
		}

		if( ((data & C_BDR) == C_BDR) && ((data & C_BC1) == 0) ) /* BDIR = 1, BC1 = 0: write data */
		{
			m_ay->data_w(space, 0, tms7000_portd);
			logerror( "AY3-8913 writing data: %u\n", tms7000_portd );
		}
	}

	tms7000_portc = data;
}

READ8_MEMBER(coco_ssc_device::ssc_port_d_r)
{
	logerror("port_d_r: read offset: %4.4X\n", offset);

	uint8_t data = 0x00;

	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
	{
		if( (tms7000_portc & C_RRW) == C_RRW ) /* static ram chip read (high) */
		{
			data |= m_staticram->read(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb);
			logerror("read static ram: address: %u, data: %u\n", ((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, data );
		}
	}

	if( (tms7000_portc & C_ACS) == 0 ) /* chip select for AY-3-8913 */
	{
		if( ((tms7000_portc & C_BDR) == 0) && ((tms7000_portc & C_BC1) == C_BC1) ) /* read data */
		{
			data |= m_ay->data_r(space, 0);
			logerror( "AY3-8913 reading data: %u\n", data );
		}
	}

	return data;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_d_w)
{
	logerror("port_d_w: write offset: %4.4X, data: %2.2X\n", offset, data);

	if( (tms7000_portc & C_RCS) == 0 ) /* static ram chip select (low) */
	{
		if( (tms7000_portc & C_RRW) == 0 ) /* static ram chip write (low) */
		{
			m_staticram->write(((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, data);
			logerror("write static ram offset: %u, data: %u\n", ((tms7000_portc & (C_A10|C_A9|C_A8)) << 8) + tms7000_portb, data );
		}
	}

	if( (tms7000_portc & C_ACS) == 0 ) /* chip select for the AY-3-8913 */
	{
		if( (tms7000_portc & (C_BDR|C_BC1)) == (C_BDR|C_BC1) ) /* BDIR = 1, BC1 = 1: latch address */
		{
			m_ay->address_w(space, 0, data);
			logerror( "AY3-8913 writing address: %u\n", data );
		}

		if( ((tms7000_portc & C_BDR) == C_BDR) && ((tms7000_portc & C_BC1) == 0) ) /* BDIR = 1, BC1 = 0: write data */
		{
			m_ay->data_w(space, 0, data);
			logerror( "AY3-8913 writing data: %u\n", data );
		}
	}

	tms7000_portd = data;
}

