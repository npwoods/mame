// license:BSD-3-Clause
// copyright-holders:Nathan Woods
#ifndef MAME_BUS_COCO_COCO_232_H
#define MAME_BUS_COCO_COCO_232_H

#pragma once

#include "cococart.h"
#include "machine/mos6551.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_232_device

class coco_232_device :
		public device_t,
		public device_cococart_interface
{
public:
	// construction/destruction
	coco_232_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const override;
protected:
	// device-level overrides
	virtual void device_start() override;
	virtual DECLARE_READ8_MEMBER(read) override;
	virtual DECLARE_WRITE8_MEMBER(write) override;
private:
		// internal state
	required_device<mos6551_device> m_uart;
};


// device type definition
DECLARE_DEVICE_TYPE(COCO_232, coco_232_device)

#endif // MAME_BUS_COCO_COCO_232_H
