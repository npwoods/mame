// license:BSD-3-Clause
// copyright-holders:tim lindner
/***************************************************************************

    coco_ssc.c

    Code for emulating the CoCo Speech / Sound cartridge

    The SSC was a complex sound cartridge. A TMS-7040 microcontroller,
    SPO256-AL2 speech processor, AY3-8913 programable sound generator, and
    2K of RAM.

***************************************************************************/

#include "emu.h"
#include "coco_ssc.h"
#include "cpu/tms7000/tms7000.h"
#include "sound/volt_reg.h"


static ADDRESS_MAP_START( ssc_io_map, AS_IO, 8, coco_ssc_device )
	AM_RANGE(TMS7000_PORTA, TMS7000_PORTA) AM_READ(ssc_port_a_r)
	AM_RANGE(TMS7000_PORTB, TMS7000_PORTB) AM_WRITE(ssc_port_b_w)
	AM_RANGE(TMS7000_PORTC, TMS7000_PORTC) AM_WRITE(ssc_port_c_w)
	AM_RANGE(TMS7000_PORTD, TMS7000_PORTD) AM_READWRITE(ssc_port_d_r, ssc_port_d_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( ssc_rom, AS_PROGRAM, 8, coco_ssc_device )
	AM_RANGE(0xf000, 0xffff) AM_REGION("ssc_tms7040", 0)
ADDRESS_MAP_END

static MACHINE_CONFIG_FRAGMENT( coco_ssc )
	MCFG_CPU_ADD("ssc_tms7040", TMS7040, XTAL_3_579545MHz / 2)
	MCFG_CPU_PROGRAM_MAP(ssc_rom)
	MCFG_CPU_IO_MAP(ssc_io_map)

	MCFG_RAM_ADD("staticram")
	MCFG_RAM_DEFAULT_SIZE("2K")
	MCFG_RAM_DEFAULT_VALUE(0x00)

MACHINE_CONFIG_END

ROM_START( coco_ssc )
	ROM_REGION( 0x1000, "ssc_tms7040", 0 )
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
		m_tms7040(*this, "ssc_tms7040"),
		m_staticram(*this, "staticram")
{
}

//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void coco_ssc_device::device_start()
{
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

READ8_MEMBER(coco_ssc_device::read)
{
	logerror("coco_ssc_device::read offset: %4.4X\n", offset);

	switch(offset)
	{
		case 0x3D:
			break;

		case 0x3E:
			return tms7000_portc & 0x80;
			break;
	}

	return 0;
}

/*-------------------------------------------------
    write
-------------------------------------------------*/

WRITE8_MEMBER(coco_ssc_device::write)
{
	logerror("coco_ssc_device::write offset: %4.4X, data: %2.2X\n", offset, data);

	switch(offset)
	{
		case 0x3D:
			if( (reset_line & 1) == 1 )
			{
				if( (data & 1) == 0 )
				{
					m_tms7040->reset();
				}
			}

			reset_line = data;
			break;

		case 0x3E:
			tms7000_porta = data;
			m_tms7040->set_input_line(TMS7000_INT3_LINE, PULSE_LINE);
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

	tms7000_portb = data;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_c_w)
{
	logerror("port_c_w: write offset: %4.4X, data: %2.2X\n", offset, data);

	if( (data & 0x10) == 0 && (data & 0x08) == 1) logerror( "RAM selected for reading\n" );
	if( (data & 0x10) == 0 && (data & 0x08) == 0) logerror( "RAM selected for writing\n" );
	if( (data & 0x20) == 0 ) logerror( "SPO256 selected\n" );
	if( (data & 0x40) == 0 ) logerror( "AY3-8913 selected\n" );

	tms7000_portc = data;
}

READ8_MEMBER(coco_ssc_device::ssc_port_d_r)
{
	logerror("port_d_r: read offset: %4.4X\n", offset);

	uint8_t data = 0x00;

	if( (tms7000_portc & 0x10) == 0 ) /* static ram chip select (low) */
	{
		if( tms7000_portc & 0x08 ) /* static ram chip read (high) */
		{
			data |= m_staticram->read(((tms7000_portc & 0x08) << 8) + tms7000_portb);
			logerror("read static ram: %d\n", data );
		}
	}

	return data;
}

WRITE8_MEMBER(coco_ssc_device::ssc_port_d_w)
{
	logerror("port_d_w: write offset: %4.4X, data: %2.2X\n", offset, data);

	if( (tms7000_portc & 0x10) == 0 ) /* static ram chip select (low) */
	{
		if( (tms7000_portc & 0x08) == 0 ) /* static ram chip write (low) */
		{
			m_staticram->write(((tms7000_portc & 0x08) << 8) + tms7000_portb, data);
			logerror("write static ram offset: %d, data: %d\n", ((tms7000_portc & 0x08) << 8) + tms7000_portb, data );
		}
	}
}

