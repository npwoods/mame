// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	65spi.h

	Code for emulating the 65SPI

***************************************************************************/

#ifndef MAME_MACHINE_65SPI_H
#define MAME_MACHINE_65SPI_H

#pragma once

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> spi65_device


class spi65_device : public device_t
{
public:
	// construction/destruction
	spi65_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	DECLARE_READ8_MEMBER(read);
	DECLARE_WRITE8_MEMBER(write);

protected:
	// device-level overrides
	virtual void device_start() override;
};

// device type definition
DECLARE_DEVICE_TYPE(SPI65, spi65_device)

#endif // MAME_MACHINE_65SPI_H
