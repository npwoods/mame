// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	coco_spinx512.cpp

	Code for emulating the Spinx512

***************************************************************************/

#include "emu.h"
#include "cococart.h"


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> coco_spinx512_device

namespace
{
	class coco_spinx512_device :
		public device_t,
		public device_cococart_interface
	{
	public:
		// construction/destruction
		coco_spinx512_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
			: device_t(mconfig, COCO_DCMODEM, tag, owner, clock)
			, device_cococart_interface(mconfig, *this)
		{
		}

	protected:
		// device-level overrides
		virtual void device_start() override
		{
			install_write_handler(0xFFBE, 0xFFBF, write8_delegate(FUNC(coco_spinx512_device::control_w), this));

			save_item(NAME(m_map));
			save_item(NAME(m_bank));
			save_item(NAME(m_memory));
		}

	private:
		WRITE8_MEMBER(control_w)
		{
			m_map = (offset & 1) ? true : false;
			m_bank = data;
		}

		void update()
		{
			set_line_value(line::SLENB, m_map ? line_value::ASSERT : line_value::CLEAR);
		}

		// internal state
		bool			m_map;
		uint8_t			m_bank;
		uint8_t			m_memory[0x8000][16];
	};
};


//**************************************************************************
//  DEVICE DECLARATION
//**************************************************************************

DEFINE_DEVICE_TYPE(COCO_SPINX512, coco_spinx512_device, "coco_spinx512", "CoCo Spinx-512")
