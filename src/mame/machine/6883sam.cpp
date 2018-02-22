// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    6883sam.cpp

    Motorola 6883 Synchronous Address Multiplexer

    The Motorola 6883 SAM has 16 bits worth of state, but the state is changed
    by writing into a 32 byte address space; odd bytes set bits and even bytes
    clear bits.  Here is the layout:

        31  Set     TY  Map Type            0: RAM/ROM  1: All RAM
        30  Clear   TY  Map Type
        29  Set     M1  Memory Size         00: 4K      10: 64K Dynamic
        28  Clear   M1  Memory Size         01: 16K     11: 64K Static
        27  Set     M0  Memory Size
        26  Clear   M0  Memory Size
        25  Set     R1  MPU Rate            00: Slow    10: Fast
        24  Clear   R1  MPU Rate            01: Dual    11: Fast
        23  Set     R0  MPU Rate
        22  Clear   R0  MPU Rate
        21  Set     P1  Page #1             0: Low      1: High
        20  Clear   P1  Page #1
        19  Set     F6  Display Offset
        18  Clear   F6  Display Offset
        17  Set     F5  Display Offset
        16  Clear   F5  Display Offset
        15  Set     F4  Display Offset
        14  Clear   F4  Display Offset
        13  Set     F3  Display Offset
        12  Clear   F3  Display Offset
        11  Set     F2  Display Offset
        10  Clear   F2  Display Offset
         9  Set     F1  Display Offset
         8  Clear   F1  Display Offset
         7  Set     F0  Display Offset
         6  Clear   F0  Display Offset
         5  Set     V2  VDG Mode
         4  Clear   V2  VDG Mode
         3  Set     V1  VDG Mode
         2  Clear   V1  VDG Mode
         1  Set     V0  VDG Mode
         0  Clear   V0  VDG Mode

    All parts of the SAM are fully emulated except R1/R0 (the changes in the
    MPU rate are approximated) and M1/M0

	This code goes through sommersaults to get the MAME memory manager to
	do its bidding

***************************************************************************/


#include "emu.h"
#include "machine/6883sam.h"


//**************************************************************************
//  CONSTANTS
//**************************************************************************

#define LOG_SAM     0

DEFINE_DEVICE_TYPE(SAM6883, sam6883_device, "sam6883", "MC6883 SAM")



//**************************************************************************
//  DEVICE SETUP
//**************************************************************************

//-------------------------------------------------
//  ctor
//-------------------------------------------------

sam6883_device::sam6883_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, SAM6883, tag, owner, clock)
	, sam6883_friend_device_interface(mconfig, *this, 4)
	, m_cpu_tag(nullptr)
	, m_cpu_space_ref(AS_PROGRAM)
	, m_cpu_space(nullptr)
	, m_shadow_space(nullptr)
	, m_read_res(*this)
	, m_space_0000(*this)
	, m_space_8000(*this)
	, m_space_A000(*this)
	, m_space_C000(*this)
	, m_space_FF00(*this)
	, m_space_FF20(*this)
	, m_space_FF40(*this)
	, m_space_FF60(*this)
	, m_space_FFE0(*this)
{
}

sam6883_friend_device_interface::sam6883_friend_device_interface(const machine_config &mconfig, device_t &device, int divider)
	: device_interface(device, "sam6883")
	, m_cpu(nullptr)
	, m_sam_state(0x0000)
	, m_divider(divider)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void sam6883_device::device_start()
{
	// find the CPU
	m_cpu = machine().device<cpu_device>(m_cpu_tag);
	m_cpu_space = &m_cpu->space(m_cpu_space_ref);

	// resolve callbacks
	m_read_res.resolve_safe(0);

	// install SAM handlers
	m_cpu_space->install_read_handler(0xFFC0, 0xFFDF, read8_delegate(FUNC(sam6883_device::read), this));
	m_cpu_space->install_write_handler(0xFFC0, 0xFFDF, write8_delegate(FUNC(sam6883_device::write), this));

	// save state support
	save_item(NAME(m_sam_state));
	save_item(NAME(m_counter));
	save_item(NAME(m_counter_xdiv));
	save_item(NAME(m_counter_ydiv));
}


//-------------------------------------------------
//  configure_bank - bank configuration
//-------------------------------------------------

void sam6883_device::configure_bank(int bank, uint8_t *memory, uint32_t memory_size, bool is_read_only)
{
	configure_bank(bank, memory, memory_size, is_read_only, read8_delegate(), write8_delegate());
}


//-------------------------------------------------
//  configure_bank - bank configuration
//-------------------------------------------------

void sam6883_device::configure_bank(int bank, read8_delegate rhandler, write8_delegate whandler)
{
	configure_bank(bank, nullptr, 0, false, rhandler, whandler);
}


//-------------------------------------------------
//  configure_bank - bank configuration
//-------------------------------------------------

void sam6883_device::configure_bank(int bank, uint8_t *memory, uint32_t memory_size, bool is_read_only, read8_delegate rhandler, write8_delegate whandler)
{
	assert((bank >= 0) && (bank < ARRAY_LENGTH(m_banks)));
	m_banks[bank].m_memory = memory;
	m_banks[bank].m_memory_size = memory_size;
	m_banks[bank].m_memory_read_only = is_read_only;
	m_banks[bank].m_rhandler = rhandler;
	m_banks[bank].m_whandler = whandler;

	/* if we're configuring a bank that never changes, update it now */
	switch(bank)
	{
		case 4:
			m_space_FF00.point(m_banks[4], 0x0000);
			break;
		case 5:
			m_space_FF20.point(m_banks[5], 0x0000);
			break;
		case 6:
			m_space_FF40.point(m_banks[6], 0x0000);
			break;
		case 7:
			m_space_FF60.point(m_banks[7], 0x0000);
			break;
		case 2:
			m_space_FFE0.point(m_banks[2], 0x1FE0);
			break;
	}
}



//-------------------------------------------------
//  device_reset - device-specific reset
//-------------------------------------------------

void sam6883_device::device_reset()
{
	m_counter = 0;
	m_counter_xdiv = 0;
	m_counter_ydiv = 0;
	m_sam_state = 0x0000;
	update_state();
}



//-------------------------------------------------
//  device_post_load - device-specific post load
//-------------------------------------------------

void sam6883_device::device_post_load()
{
	device_t::device_post_load();
	update_state();
}



//-------------------------------------------------
//  update_state
//-------------------------------------------------

void sam6883_device::update_state(void)
{
	update_memory();
	update_cpu_clock();
}



//-------------------------------------------------
//  update_memory
//-------------------------------------------------

void sam6883_device::update_memory(void)
{
	// Memory size - allowed restricting memory accesses to something less than
	// 32k
	//
	// This was a SAM switch that occupied 4 addresses:
	//
	//      $FFDD   (set)   R1
	//      $FFDC   (clear) R1
	//      $FFDB   (set)   R0
	//      $FFDA   (clear) R0
	//
	// R1:R0 formed the following states:
	//      00  - 4k
	//      01  - 16k
	//      10  - 64k
	//      11  - static RAM (??)
	//
	// If something less than 64k was set, the low RAM would be smaller and
	// mirror the other parts of the RAM
	//
	// TODO:  Find out what "static RAM" is
	// TODO:  This should affect _all_ memory accesses, not just video ram
	// TODO:  Verify that the CoCo 3 ignored this

	// switch depending on the M1/M0 variables
	bool setup_rom = true;
	switch(m_sam_state & (SAM_STATE_M1|SAM_STATE_M0))
	{
		case 0:
			// 4K mode
			m_space_0000.point(m_banks[0], 0x0000, m_banks[0].m_memory_size);
			m_counter_mask = 0x0FFF;
			break;

		case SAM_STATE_M0:
			// 16K mode
			m_space_0000.point(m_banks[0], 0x0000, m_banks[0].m_memory_size);
			m_counter_mask = 0x3FFF;
			break;

		case SAM_STATE_M1:
		case SAM_STATE_M1|SAM_STATE_M0:
			// 64k mode
			if (m_sam_state & SAM_STATE_TY)
			{
				// full 64k RAM
				m_space_0000.point(m_banks[0], 0x0000, m_banks[0].m_memory_size);
				m_space_8000.point(m_banks[0], 0x8000);
				m_space_A000.point(m_banks[0], 0xA000);
				m_space_C000.point(m_banks[0], 0xC000);
				m_counter_mask = 0xFFFF;
				setup_rom = false;
			}
			else
			{
				// ROM/RAM
				uint16_t ram_base = (m_sam_state & SAM_STATE_P1) ? 0x8000 : 0x0000;
				m_space_0000.point(m_banks[0], ram_base, m_banks[0].m_memory_size);
				m_counter_mask = 0x7FFF;
			}
			break;
	}

	if (setup_rom)
	{
		m_space_8000.point(m_banks[1], m_banks[1].m_memory_offset);
		m_space_A000.point(m_banks[2], m_banks[2].m_memory_offset);
		m_space_C000.point(m_banks[3], m_banks[3].m_memory_offset);
	}

	// update $FFE0-$FFFF
	m_space_FFE0.point(m_banks[2], m_banks[2].m_memory_offset + 0x1FE0);
}



//-------------------------------------------------
//  update_cpu_clock - adjusts the speed of the CPU
//  clock
//-------------------------------------------------

void sam6883_friend_device_interface::update_cpu_clock(void)
{
	// The infamous speed up poke.
	//
	// This was a SAM switch that occupied 4 addresses:
	//
	//      $FFD9   (set)   R1
	//      $FFD8   (clear) R1
	//      $FFD7   (set)   R0
	//      $FFD6   (clear) R0
	//
	// R1:R0 formed the following states:
	//      00  - slow          0.89 MHz
	//      01  - dual speed    ???
	//      1x  - fast          1.78 MHz
	//
	// R1 controlled whether the video addressing was speeded up and R0
	// did the same for the CPU.  On pre-CoCo 3 machines, setting R1 caused
	// the screen to display garbage because the M6847 could not display
	// fast enough.
	//
	// TODO:  Make the overclock more accurate.  In dual speed, ROM was a fast
	// access but RAM was not.  I don't know how to simulate this.

	int speed = (m_sam_state & (SAM_STATE_R1|SAM_STATE_R0)) / SAM_STATE_R0;
	m_cpu->owner()->set_unscaled_clock(device().clock() / (m_divider * (speed ? 2 : 4)));
}



//-------------------------------------------------
//  set_bank_offset
//-------------------------------------------------

void sam6883_device::set_bank_offset(int bank, offs_t offset)
{
	if (m_banks[bank].m_memory_offset != offset)
	{
		m_banks[bank].m_memory_offset = offset;
		update_memory();
	}
}


//-------------------------------------------------
//  read
//-------------------------------------------------

READ8_MEMBER( sam6883_device::read )
{
	return read_shadow(0xFFC0 + offset);
}


//-------------------------------------------------
//  write
//-------------------------------------------------

WRITE8_MEMBER( sam6883_device::write )
{
	// alter the SAM state
	uint16_t xorval = alter_sam_state(offset);

	// based on the mask, apply effects
	if (xorval & (SAM_STATE_TY|SAM_STATE_M1|SAM_STATE_M0|SAM_STATE_P1))
		update_memory();
	if (xorval & (SAM_STATE_R1|SAM_STATE_R0))
		update_cpu_clock();

	// also write to the shadow space
	write_shadow(0xFFC0 + offset, data);
}


//-------------------------------------------------
//  horizontal_sync
//-------------------------------------------------

void sam6883_device::horizontal_sync(void)
{
	bool carry;

	// When horizontal sync occurs, bits B1-B3 or B1-B4 may be cleared (except in DMA mode).  The catch
	// is that the SAM's counter is a chain of flip-flops.  Clearing the counter can cause carries to
	// occur just as they can when the counter is bumped.
	//
	// This is critical in getting certain semigraphics modes to work correctly.  Guardian uses this
	// mode (see bug #1153).  Special thanks to Ciaran Anscomb and Phill Harvey-Smith for figuring this
	// out
	switch((m_sam_state & (SAM_STATE_V2|SAM_STATE_V1|SAM_STATE_V0)) / SAM_STATE_V0)
	{
		case 0x01:
		case 0x03:
		case 0x05:
			/* these SAM modes clear bits B1-B3 */
			carry = (m_counter & 0x0008) ? true : false;
			m_counter &= ~0x000F;
			if (carry)
				counter_carry_bit3();
			break;

		case 0x00:
		case 0x02:
		case 0x04:
		case 0x06:
			/* clear bits B1-B4 */
			carry = (m_counter & 0x0010) ? true : false;
			m_counter &= ~0x001F;
			if (carry)
				counter_carry_bit4();
			break;

		case 0x07:
			/* DMA mode - do nothing */
			break;

		default:
			fatalerror("Should not get here\n");
	}
}



//-------------------------------------------------
//  hs_w
//-------------------------------------------------

WRITE_LINE_MEMBER( sam6883_device::hs_w )
{
	if (state)
	{
		horizontal_sync();
	}
}


//-------------------------------------------------
//  shadow_changed
//-------------------------------------------------

void sam6883_device::shadow_changed(uint16_t addrstart, uint16_t addrend, bool read_changed, bool write_changed)
{
	// invalidate ranges
	m_space_0000.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_8000.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_A000.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_C000.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_FF00.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_FF20.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_FF60.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_FF40.invalidate_range(addrstart, addrend, read_changed, write_changed);
	m_space_FFE0.invalidate_range(addrstart, addrend, read_changed, write_changed);

	// if we're at runtime, update memory
	if (machine().phase() == machine_phase::RUNNING)
		update_memory();
}


//-------------------------------------------------
//  read_shadow
//-------------------------------------------------

uint8_t sam6883_device::read_shadow(uint16_t address)
{
	return m_shadow_space->read_byte(address);
}


//-------------------------------------------------
//  write_shadow
//-------------------------------------------------

void sam6883_device::write_shadow(uint16_t address, uint8_t data)
{
	m_shadow_space->write_byte(address, data);
}


//-------------------------------------------------
//  shadow_space_read_delegate
//-------------------------------------------------

read8_delegate sam6883_device::shadow_space_read_delegate(uint16_t addrstart)
{
	return read8_delegate(
		[this, addrstart](address_space &, offs_t offset, u8) -> u8
		{
			return read_shadow(offset + addrstart);
		},
		"shadow-read-delegate"
	);
}


//-------------------------------------------------
//  shadow_space_write_delegate
//-------------------------------------------------

write8_delegate sam6883_device::shadow_space_write_delegate(uint16_t addrstart)
{
	return write8_delegate(
		[this, addrstart](address_space &, offs_t offset, u8 data, u8)
		{
			write_shadow(offset + addrstart, data);
		},
		"shadow-write-delegate"
	);
}


//-------------------------------------------------
//  cpu_space
//-------------------------------------------------

address_space &sam6883_device::cpu_space()
{
	assert(m_cpu_space);
	return *m_cpu_space;
}


//-------------------------------------------------
//  install_read_memory
//-------------------------------------------------

void sam6883_device::install_read_memory(uint16_t addrstart, uint16_t addrend, void *memory)
{
	if (memory)
		cpu_space().install_rom(addrstart, addrend, memory);
	else
		cpu_space().nop_read(addrstart, addrend);
}


//-------------------------------------------------
//  install_write_memory
//-------------------------------------------------

void sam6883_device::install_write_memory(uint16_t addrstart, uint16_t addrend, void *memory)
{
	if (memory)
		cpu_space().install_writeonly(addrstart, addrend, memory);
	else
		cpu_space().nop_write(addrstart, addrend);
}


//-------------------------------------------------
//  install_read_handler
//-------------------------------------------------

void sam6883_device::install_read_handler(uint16_t addrstart, uint16_t addrend, read8_delegate rhandler)
{
	cpu_space().install_read_handler(addrstart, addrend, rhandler);
}


//-------------------------------------------------
//  install_write_handler
//-------------------------------------------------

void sam6883_device::install_write_handler(uint16_t addrstart, uint16_t addrend, write8_delegate whandler)
{
	cpu_space().install_write_handler(addrstart, addrend, whandler);
}


//-------------------------------------------------
//  point_specific_bank
//-------------------------------------------------

void sam6883_device::point_specific_bank(const sam_bank &bank, uint32_t offset, uint32_t length, bank_state &state, uint32_t addrstart, uint32_t addrend, bool is_write)
{
	if (bank.m_memory != nullptr)
	{
		// this bank is a memory bank - first lets adjust the length as per the offset; as
		// passed to this method, the length is from offset zero
		if (length != ~0)
			length -= std::min(offset, length);

		// determine "nop_addrstart" - where the bank ends, and above which is AM_NOP
		uint32_t nop_addrstart = (length != ~0)
			? std::min(addrend + 1, addrstart + length)
			: addrend + 1;

		// for writes, we may have to honor read only memory
		if (is_write && bank.m_memory_read_only)
			nop_addrstart = addrstart;

		// did the state change?
		bank_state new_state(addrstart, nop_addrstart, addrend, length);
		if (new_state != state)
		{
			// install the bank
			if (is_write)
			{
				if (addrstart < nop_addrstart)
					install_write_memory(addrstart, nop_addrstart - 1, bank.m_memory + offset);
				if (nop_addrstart <= addrend)
					install_write_memory(nop_addrstart, addrend, nullptr);
			}
			else
			{
				if (addrstart < nop_addrstart)
					install_read_memory(addrstart, nop_addrstart - 1, bank.m_memory + offset);
				if (nop_addrstart <= addrend)
					install_read_memory(nop_addrstart, addrend, nullptr);
			}

			state = new_state;
		}
	}
	else
	{
		// this bank uses handlers - first thing's first, assert that we are not doing
		// any weird stuff with offfsets and lengths - that isn't supported in this path
		assert((offset == 0) && (length == (uint32_t)~0));

		if (is_write)
		{
			write8_delegate wh = !bank.m_whandler.isnull()
				? bank.m_whandler
				: shadow_space_write_delegate(addrstart);
			install_write_handler(addrstart, addrend, wh);
		}
		else
		{
			read8_delegate rh = !bank.m_rhandler.isnull()
				? bank.m_rhandler
				: shadow_space_read_delegate(addrstart);
			install_read_handler(addrstart, addrend, rh);
		}
	}
}


//-------------------------------------------------
//  update_shadow
//-------------------------------------------------

void sam6883_device::update_shadow(uint16_t addrstart, uint16_t addrend, read_or_write row)
{
	if (m_update_shadow)
		m_update_shadow(addrstart, addrend, row);
}


//-------------------------------------------------
//  bank_state::ctor
//-------------------------------------------------

sam6883_device::bank_state::bank_state()
	: bank_state(0, 0, 0, 0)
{
}


//-------------------------------------------------
//  bank_state::ctor
//-------------------------------------------------

sam6883_device::bank_state::bank_state(uint32_t addrstart, uint32_t nop_addr_start, uint32_t addrend, uint32_t length)
	: m_addrstart(addrstart)
	, m_nop_addrstart(nop_addr_start)
	, m_addrend(addrend)
	, m_length(length)
{
}


//-------------------------------------------------
//  bank_state::operator!=
//-------------------------------------------------

bool sam6883_device::bank_state::operator!=(const sam6883_device::bank_state &that) const
{
	return m_addrstart != that.m_addrstart
		|| m_nop_addrstart != that.m_nop_addrstart
		|| m_addrend != that.m_addrend
		|| m_length != that.m_length;
}


//-------------------------------------------------
//  sam_space::ctor
//-------------------------------------------------

template<uint16_t _addrstart, uint16_t _addrend>
sam6883_device::sam_space<_addrstart, _addrend>::sam_space(sam6883_device &owner)
	: m_owner(owner)
	, m_length(0)
	, m_shadow_addrstart(_addrstart)
	, m_shadow_length(_addrend - _addrstart + 1)
{
}


//-------------------------------------------------
//  sam_space::point
//-------------------------------------------------

template<uint16_t _addrstart, uint16_t _addrend>
void sam6883_device::sam_space<_addrstart, _addrend>::point(const sam_bank &bank, uint16_t offset, uint32_t length)
{
	if (LOG_SAM)
	{
		m_owner.logerror("sam6883_device::sam_space::point():  addrstart=0x%04X addrend=0x%04X offset=0x%04X length=0x%04X bank.m_memory=0x%p bank.m_memory_read_only=%s\n",
			(unsigned) _addrstart,
			(unsigned) _addrend,
			(unsigned) offset,
			(unsigned)length,
			bank.m_memory,
			bank.m_memory_read_only ? "true" : "false");
	}

	m_owner.point_specific_bank(bank, offset, length, m_read_bank, _addrstart, _addrend, false);
	m_owner.point_specific_bank(bank, offset, length, m_write_bank, _addrstart, _addrend, true);
	m_owner.update_shadow(_addrstart, _addrend, read_or_write::READ);
	m_owner.update_shadow(_addrstart, _addrend, read_or_write::WRITE);
}


//------------------------------------------------
//  sam_space::invalidate_range
//-------------------------------------------------

template<uint16_t _addrstart, uint16_t _addrend>
void sam6883_device::sam_space<_addrstart, _addrend>::invalidate_range(uint16_t addrstart, uint16_t addrend, bool read_changed, bool write_changed)
{
	if (addrstart <= _addrend && addrend >= _addrstart)
	{
		if (read_changed)
			m_read_bank = bank_state();
		if (write_changed)
			m_write_bank = bank_state();
	}
}
