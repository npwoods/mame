// license:BSD-3-Clause
// copyright-holders:tim lindner
#pragma once

#ifndef __COCO_SSC_H__
#define __COCO_SSC_H__

#include "machine/ram.h"
#include "cococart.h"

//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_ssc_device

class coco_ssc_device :
		public device_t,
		public device_cococart_interface
{
public:
		// construction/destruction
		coco_ssc_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

		// optional information overrides
		virtual const tiny_rom_entry *device_rom_region() const override;
		virtual machine_config_constructor device_mconfig_additions() const override;

		DECLARE_READ8_MEMBER(ssc_port_a_r);
		DECLARE_WRITE8_MEMBER(ssc_port_b_w);
		DECLARE_WRITE8_MEMBER(ssc_port_c_w);
		DECLARE_READ8_MEMBER(ssc_port_d_r);
		DECLARE_WRITE8_MEMBER(ssc_port_d_w);

protected:
		// device-level overrides
		virtual void device_start() override;
		virtual DECLARE_READ8_MEMBER(read) override;
		virtual DECLARE_WRITE8_MEMBER(write) override;
private:
		uint8_t reset_line;
		uint8_t tms7000_porta;
		uint8_t tms7000_portb;
		uint8_t tms7000_portc;
		required_device<cpu_device> m_tms7040;
		required_device<ram_device> m_staticram;
};


// device type definition
extern const device_type COCO_SSC;

#endif  /* __COCO_SSC_H__ */
