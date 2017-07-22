// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	65spi.cpp

	Code for emulating the 65SPI

***************************************************************************/

#include "emu.h"
#include "65spi.h"


//-------------------------------------------------
//  ctor
//-------------------------------------------------

spi65_device::spi65_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, SPI65, tag, owner, clock)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void spi65_device::device_start()
{
}


//-------------------------------------------------
//  read
//-------------------------------------------------

READ8_MEMBER(spi65_device::read)
{
	return 0;
}


//-------------------------------------------------
//  write
//-------------------------------------------------

WRITE8_MEMBER(spi65_device::write)
{
}


//**************************************************************************
//  DEVICE DECLARATION
//**************************************************************************

DEFINE_DEVICE_TYPE(SPI65, spi65_device, "spi65", "65SPI")
