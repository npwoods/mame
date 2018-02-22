// license:BSD-3-Clause
// copyright-holders:David Haywood, Miodrag Milanovic
/*********************************************************************

    pgm2_memcard.cpp

    PGM2 Memory card functions.
    Presumably Siemens SLE 4442 or compatible.

*********************************************************************/

#include "emu.h"
#include "emuopts.h"

#include "pgm2_memcard.h"


// device type definition
DEFINE_DEVICE_TYPE(PGM2_MEMCARD, pgm2_memcard_device, "pgm2_memcard", "PGM2 Memory Card")

//-------------------------------------------------
//  pgm2_memcard_device - constructor
//-------------------------------------------------

pgm2_memcard_device::pgm2_memcard_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock)
	: device_t(mconfig, PGM2_MEMCARD, tag, owner, clock)
	, device_image_interface(mconfig, *this)
{
}


//-------------------------------------------------
//  device_start - device-specific startup
//-------------------------------------------------

void pgm2_memcard_device::device_start()
{
	save_item(NAME(m_memcard_data));
}

/*-------------------------------------------------
    memcard_insert - insert an existing memory card
    with the given index
-------------------------------------------------*/

image_init_result pgm2_memcard_device::call_load()
{
	authenticated = false;
	if(length() != 0x108)
		return image_init_result::FAIL;

	fseek(0, SEEK_SET);
	size_t ret = fread(m_memcard_data, 0x100);
	if(ret != 0x100)
		return image_init_result::FAIL;
	ret = fread(m_protection_data, 4);
	if (ret != 4)
		return image_init_result::FAIL;
	ret = fread(m_security_data, 4);
	if (ret != 4)
		return image_init_result::FAIL;

	return image_init_result::PASS;
}

void pgm2_memcard_device::call_unload()
{
	authenticated = false;
	fseek(0, SEEK_SET);
	fwrite(m_memcard_data, 0x100);
	fwrite(m_protection_data, 4);
	fwrite(m_security_data, 4);
}

image_init_result pgm2_memcard_device::call_create(int format_type, util::option_resolution *format_options)
{
	authenticated = false;
	// cards must contain valid defaults for each game / region or they don't work?
	memory_region *rgn = memregion("^default_card");

	if (!rgn)
		return image_init_result::FAIL;

	memcpy(m_memcard_data, rgn->base(), 0x100);
	memcpy(m_protection_data, rgn->base() + 0x100, 4);
	memcpy(m_security_data, rgn->base() + 0x104, 4);

	size_t ret = fwrite(rgn->base(), 0x108);
	if(ret != 0x108)
		return image_init_result::FAIL;

	return image_init_result::PASS;
}

void pgm2_memcard_device::auth(uint8_t p1, uint8_t p2, uint8_t p3)
{
	if (m_security_data[0] & 7)
	{
		if (m_security_data[1] == p1 && m_security_data[2] == p2 && m_security_data[3] == p3) {
			authenticated = true;
			m_security_data[0] = 7;
		}
		else {
			authenticated = false;
			m_security_data[0] >>= 1; // hacky
			if (m_security_data[0] & 7)
				popmessage("Wrong IC Card password !!!\n");
			else
				popmessage("Wrong IC Card password, card was locked!!!\n");
		}
	}
}

READ8_MEMBER(pgm2_memcard_device::read)
{
	return m_memcard_data[offset];
}

WRITE8_MEMBER(pgm2_memcard_device::write)
{
	if (authenticated && (offset >= 0x20 || (m_protection_data[offset>>3] & (1 <<(offset & 7)))))
	{
		m_memcard_data[offset] = data;
	}
}

READ8_MEMBER(pgm2_memcard_device::read_prot)
{
	return m_protection_data[offset];
}

WRITE8_MEMBER(pgm2_memcard_device::write_prot)
{
	if (authenticated)
		m_protection_data[offset] &= data;
}

READ8_MEMBER(pgm2_memcard_device::read_sec)
{
	if (!authenticated)
		return 0xff; // guess
	return m_security_data[offset];
}

WRITE8_MEMBER(pgm2_memcard_device::write_sec)
{
	if (authenticated)
		m_security_data[offset] = data;
}
