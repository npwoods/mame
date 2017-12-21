// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

	coco_spinx512.cpp

	Code for emulating the Spinx512

***************************************************************************/

#include "emu.h"
#include "cococart.h"
#include "coco_spinx512.h"
#include "machine/65spi.h"


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
			: device_t(mconfig, COCO_SPINX512, tag, owner, clock)
			, device_cococart_interface(mconfig, *this)
			, m_spi(*this, "spi")
		{
		}

		// optional information overrides
		virtual void device_add_mconfig(machine_config &config) override;

	protected:
		// device-level overrides
		virtual void device_start() override
		{
			install_readwrite_handler(0xFF6C, 0xFF6F, read8_delegate(FUNC(spi65_device::read), &*m_spi), write8_delegate(FUNC(spi65_device::write), &*m_spi));
			install_write_handler(0xFFA8, 0xFFAF, write8_delegate(FUNC(coco_spinx512_device::ffax_w), this));
			install_write_handler(0xFFBE, 0xFFBF, write8_delegate(FUNC(coco_spinx512_device::ffbx_w), this));
			install_write_handler(0xFFDE, 0xFFDF, write8_delegate(FUNC(coco_spinx512_device::ty_w), this));

			save_item(NAME(m_map));
			save_item(NAME(m_ty));
			save_item(NAME(m_bank));
			save_item(NAME(m_memory));
		}

		virtual void device_reset() override
		{
			for (int i = 0; i < ARRAY_LENGTH(m_map); i++)
				m_map[i] = false;
			m_ty = false;
			m_bank = 0;
			update();
		}

		virtual void device_post_load() override
		{
			update();
		}

	private:
		WRITE8_MEMBER(ffax_w)	// FFA8-F
		{
			m_map[offset / 2 % ARRAY_LENGTH(m_map)] = (offset & 1) ? true : false;
			update();
		}

		WRITE8_MEMBER(ffbx_w)	// FFBE-F
		{
			for (int i = 0; i < ARRAY_LENGTH(m_map); i++)
				m_map[i] = (offset & 1) ? true : false;
			m_bank = data;
			update();
		}

		WRITE8_MEMBER(ty_w)
		{
			m_ty = (offset & 1) ? true : false;
			update();
		}

		void update()
		{
			// The Spinx-512 uses the SLENB signal to disable normal CoCo memory
			// operations.  This is presumed by the use of install_ram() into
			// normally handled areas.
			for (int i = 0; i < ARRAY_LENGTH(m_map); i++)
			{
				uint16_t addrstart = 0x8000 + i * 0x2000;
				uint16_t addrend = std::min(addrstart + 0x1FFF, 0xFEFF);
				if (!m_ty && m_map[i])
					install_ram(addrstart, addrend, &m_memory[m_bank % ARRAY_LENGTH(m_memory)][i * 0x2000]);
				else
					unmap(addrstart, addrend);
			}
		}

		// devices
		required_device<spi65_device> m_spi;

		// internal state
		bool			m_map[4];
		bool			m_ty;
		uint8_t			m_bank;
		uint8_t			m_memory[16][0x8000];
	};
};


//**************************************************************************
//  MACHINE AND ROM DECLARATIONS
//**************************************************************************

MACHINE_CONFIG_MEMBER(coco_spinx512_device::device_add_mconfig)
	MCFG_DEVICE_ADD("spi", SPI65, 0)
MACHINE_CONFIG_END


//**************************************************************************
//  DEVICE DECLARATION
//**************************************************************************

DEFINE_DEVICE_TYPE(COCO_SPINX512, coco_spinx512_device, "coco_spinx512", "CoCo Spinx-512")
