// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    coco_multi.h

    Multi-Pak interface emulation

***************************************************************************/

#pragma once

#ifndef __COCO_MULTI_H__
#define __COCO_MULTI_H__

#include "cococart.h"



//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_multipak_device

class coco_multipak_device :
	public device_t,
	public device_cococart_interface
{
public:
	// construction/destruction
	coco_multipak_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// optional information overrides
	virtual machine_config_constructor device_mconfig_additions() const override;

	virtual uint8_t* get_cart_base() override;

	DECLARE_WRITE_LINE_MEMBER(multi_cart_w);
	DECLARE_WRITE_LINE_MEMBER(multi_nmi_w);
	DECLARE_WRITE_LINE_MEMBER(multi_halt_w);

	void cart_install_read_handler(int slot, uint16_t addrstart, uint16_t addrend, read8_delegate rhandler);
	void cart_install_write_handler(int slot, uint16_t addrstart, uint16_t addrend, write8_delegate whandler);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_reset() override;
	virtual DECLARE_READ8_MEMBER(scs_read) override;
	virtual DECLARE_WRITE8_MEMBER(scs_write) override;
	virtual void set_sound_enable(bool sound_enable) override;

private:
	// device references
	cococart_slot_device *m_owner;
	std::array<cococart_slot_device *, 4> m_slots;

	// internal state
	uint8_t m_select;

	// methods
	DECLARE_WRITE8_MEMBER(ff7f_write);
	cococart_slot_device *active_scs_slot(void);
	cococart_slot_device *active_cts_slot(void);
	void set_select(uint8_t new_select);
};


// device type definition
extern const device_type COCO_MULTIPAK;

#endif  /* __COCO_MULTI_H__ */
