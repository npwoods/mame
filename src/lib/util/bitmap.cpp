// license:BSD-3-Clause
// copyright-holders:Aaron Giles
/***************************************************************************

    bitmap.c

    Core bitmap routines.

***************************************************************************/

#include <assert.h>

#include "bitmap.h"

#include <new>



//**************************************************************************
//  CONSTANTS
//**************************************************************************

/** @brief  alignment values; 128 bytes is the largest cache line on typical architectures today. */
const uint32_t BITMAP_OVERALL_ALIGN = 128;
/** @brief  The bitmap rowbytes align. */
const uint32_t BITMAP_ROWBYTES_ALIGN = 128;



//**************************************************************************
//  INLINE HELPERS
//**************************************************************************

//-------------------------------------------------
//  compute_rowpixels - compute an aligned
//  rowpixels value
//-------------------------------------------------

inline int32_t bitmap_t::compute_rowpixels(int width, int xslop)
{
	int rowpixels_align = BITMAP_ROWBYTES_ALIGN / (m_bpp / 8);
	return ((width + 2 * xslop + (rowpixels_align - 1)) / rowpixels_align) * rowpixels_align;
}


//-------------------------------------------------
//  compute_base - compute an aligned bitmap base
//  address with the given slop values
//-------------------------------------------------

inline void bitmap_t::compute_base(int xslop, int yslop)
{
	m_base = m_alloc + (m_rowpixels * yslop + xslop) * (m_bpp / 8);
	uint64_t aligned_base = ((reinterpret_cast<uint64_t>(m_base) + (BITMAP_OVERALL_ALIGN - 1)) / BITMAP_OVERALL_ALIGN) * BITMAP_OVERALL_ALIGN;
	m_base = reinterpret_cast<void *>(aligned_base);
}



//**************************************************************************
//  BITMAP ALLOCATION/CONFIGURATION
//**************************************************************************

/**
 * @fn  bitmap_t::bitmap_t(bitmap_format format, int bpp, int width, int height, int xslop, int yslop)
 *
 * @brief   -------------------------------------------------
 *            bitmap_t - basic constructor
 *          -------------------------------------------------.
 *
 * @param   format  Describes the format to use.
 * @param   bpp     The bits per pixel.
 * @param   width   The width.
 * @param   height  The height.
 * @param   xslop   The xslop.
 * @param   yslop   The yslop.
 */

bitmap_t::bitmap_t(bitmap_format format, int bpp, int width, int height, int xslop, int yslop)
	: m_alloc(nullptr),
		m_allocbytes(0),
		m_format(format),
		m_bpp(bpp),
		m_palette(nullptr)
{
	// allocate intializes all other fields
	allocate(width, height, xslop, yslop);
}

/**
 * @fn  bitmap_t::bitmap_t(bitmap_format format, int bpp, void *base, int width, int height, int rowpixels)
 *
 * @brief   Constructor.
 *
 * @param   format          Describes the format to use.
 * @param   bpp             The bits per pixel.
 * @param [in,out]  base    If non-null, the base.
 * @param   width           The width.
 * @param   height          The height.
 * @param   rowpixels       The rowpixels.
 */

bitmap_t::bitmap_t(bitmap_format format, int bpp, void *base, int width, int height, int rowpixels)
	: m_alloc(nullptr),
		m_allocbytes(0),
		m_base(base),
		m_rowpixels(rowpixels),
		m_width(width),
		m_height(height),
		m_format(format),
		m_bpp(bpp),
		m_palette(nullptr),
		m_cliprect(0, width - 1, 0, height - 1)
{
}

/**
 * @fn  bitmap_t::bitmap_t(bitmap_format format, int bpp, bitmap_t &source, const rectangle &subrect)
 *
 * @brief   Constructor.
 *
 * @param   format          Describes the format to use.
 * @param   bpp             The bits per pixel.
 * @param [in,out]  source  Source for the.
 * @param   subrect         The subrect.
 */

bitmap_t::bitmap_t(bitmap_format format, int bpp, bitmap_t &source, const rectangle &subrect)
	: m_alloc(nullptr),
		m_allocbytes(0),
		m_base(source.raw_pixptr(subrect.min_y, subrect.min_x)),
		m_rowpixels(source.m_rowpixels),
		m_width(subrect.width()),
		m_height(subrect.height()),
		m_format(format),
		m_bpp(bpp),
		m_palette(nullptr),
		m_cliprect(0, subrect.width() - 1, 0, subrect.height() - 1)
{
	assert(format == source.m_format);
	assert(bpp == source.m_bpp);
	assert(source.cliprect().contains(subrect));
}

/**
 * @fn  bitmap_t::~bitmap_t()
 *
 * @brief   -------------------------------------------------
 *            ~bitmap_t - basic destructor
 *          -------------------------------------------------.
 */

bitmap_t::~bitmap_t()
{
	// delete any existing stuff
	reset();
}

/**
 * @fn  void bitmap_t::allocate(int width, int height, int xslop, int yslop)
 *
 * @brief   -------------------------------------------------
 *            allocate -- (re)allocate memory for the bitmap at the given size, destroying
 *            anything that already exists
 *          -------------------------------------------------.
 *
 * @param   width   The width.
 * @param   height  The height.
 * @param   xslop   The xslop.
 * @param   yslop   The yslop.
 */

void bitmap_t::allocate(int width, int height, int xslop, int yslop)
{
	assert(m_format != BITMAP_FORMAT_INVALID);
	assert(m_bpp == 8 || m_bpp == 16 || m_bpp == 32 || m_bpp == 64);

	// delete any existing stuff
	reset();

	// handle empty requests cleanly
	if (width <= 0 || height <= 0)
		return;

	// initialize fields
	m_rowpixels = compute_rowpixels(width, xslop);
	m_width = width;
	m_height = height;
	m_cliprect.set(0, width - 1, 0, height - 1);

	// allocate memory for the bitmap itself
	m_allocbytes = m_rowpixels * (m_height + 2 * yslop) * m_bpp / 8;
	m_allocbytes += BITMAP_OVERALL_ALIGN - 1;
	m_alloc = new uint8_t[m_allocbytes];

	// clear to 0 by default
	memset(m_alloc, 0, m_allocbytes);

	// compute the base
	compute_base(xslop, yslop);
}

/**
 * @fn  void bitmap_t::resize(int width, int height, int xslop, int yslop)
 *
 * @brief   -------------------------------------------------
 *            resize -- resize a bitmap, reusing existing memory if the new size is smaller than
 *            the current size
 *          -------------------------------------------------.
 *
 * @param   width   The width.
 * @param   height  The height.
 * @param   xslop   The xslop.
 * @param   yslop   The yslop.
 */

void bitmap_t::resize(int width, int height, int xslop, int yslop)
{
	assert(m_format != BITMAP_FORMAT_INVALID);
	assert(m_bpp == 8 || m_bpp == 16 || m_bpp == 32 || m_bpp == 64);

	// handle empty requests cleanly
	if (width <= 0 || height <= 0)
		width = height = 0;

	// determine how much memory we need for the new bitmap
	int new_rowpixels = compute_rowpixels(width, xslop);
	uint32_t new_allocbytes = new_rowpixels * (height + 2 * yslop) * m_bpp / 8;
	new_allocbytes += BITMAP_OVERALL_ALIGN - 1;

	// if we need more memory, just realloc
	if (new_allocbytes > m_allocbytes)
	{
		palette_t *palette = m_palette;
		allocate(width, height, xslop, yslop);
		set_palette(palette);
		return;
	}

	// otherwise, reconfigure
	m_rowpixels = new_rowpixels;
	m_width = width;
	m_height = height;
	m_cliprect.set(0, width - 1, 0, height - 1);

	// re-compute the base
	compute_base(xslop, yslop);
}

/**
 * @fn  void bitmap_t::reset()
 *
 * @brief   -------------------------------------------------
 *            reset -- reset to an invalid bitmap, deleting all allocated stuff
 *          -------------------------------------------------.
 */

void bitmap_t::reset()
{
	// delete any existing stuff
	set_palette(nullptr);
	delete[] m_alloc;
	m_alloc = nullptr;
	m_base = nullptr;

	// reset all fields
	m_rowpixels = 0;
	m_width = 0;
	m_height = 0;
	m_cliprect.set(0, -1, 0, -1);
}

/**
 * @fn  void bitmap_t::wrap(void *base, int width, int height, int rowpixels)
 *
 * @brief   -------------------------------------------------
 *            wrap -- wrap an array of memory; the target bitmap does not own the memory
 *          -------------------------------------------------.
 *
 * @param [in,out]  base    If non-null, the base.
 * @param   width           The width.
 * @param   height          The height.
 * @param   rowpixels       The rowpixels.
 */

void bitmap_t::wrap(void *base, int width, int height, int rowpixels)
{
	// delete any existing stuff
	reset();

	// initialize relevant fields
	m_base = base;
	m_rowpixels = rowpixels;
	m_width = width;
	m_height = height;
	m_cliprect.set(0, m_width - 1, 0, m_height - 1);
}

/**
 * @fn  void bitmap_t::wrap(const bitmap_t &source, const rectangle &subrect)
 *
 * @brief   -------------------------------------------------
 *            wrap -- wrap a subrectangle of an existing bitmap by copying its fields; the target
 *            bitmap does not own the memory
 *          -------------------------------------------------.
 *
 * @param   source  Source for the.
 * @param   subrect The subrect.
 */

void bitmap_t::wrap(const bitmap_t &source, const rectangle &subrect)
{
	assert(m_format == source.m_format);
	assert(m_bpp == source.m_bpp);
	assert(source.cliprect().contains(subrect));

	// delete any existing stuff
	reset();

	// copy relevant fields
	m_base = source.raw_pixptr(subrect.min_y, subrect.min_x);
	m_rowpixels = source.m_rowpixels;
	m_width = subrect.width();
	m_height = subrect.height();
	set_palette(source.m_palette);
	m_cliprect.set(0, m_width - 1, 0, m_height - 1);
}

/**
 * @fn  void bitmap_t::set_palette(palette_t *palette)
 *
 * @brief   -------------------------------------------------
 *            set_palette -- associate a palette with a bitmap
 *          -------------------------------------------------.
 *
 * @param [in,out]  palette If non-null, the palette.
 */

void bitmap_t::set_palette(palette_t *palette)
{
	// first dereference any existing palette
	if (m_palette != nullptr)
	{
		m_palette->deref();
		m_palette = nullptr;
	}

	// then reference any new palette
	if (palette != nullptr)
	{
		palette->ref();
		m_palette = palette;
	}
}

/**
 * @fn  void bitmap_t::fill(uint32_t color, const rectangle &cliprect)
 *
 * @brief   -------------------------------------------------
 *            fill -- fill a bitmap with a solid color
 *          -------------------------------------------------.
 *
 * @param   color       The color.
 * @param   cliprect    The cliprect.
 */

void bitmap_t::fill(uint32_t color, const rectangle &cliprect)
{
	// if we have a cliprect, intersect with that
	rectangle fill = cliprect;
	fill &= m_cliprect;
	if (fill.empty())
		return;

	// based on the bpp go from there
	switch (m_bpp)
	{
		case 8:
			// 8bpp always uses memset
			for (int32_t y = fill.min_y; y <= fill.max_y; y++)
				memset(raw_pixptr(y, fill.min_x), (uint8_t)color, fill.width());
			break;

		case 16:
			// 16bpp can use memset if the bytes are equal
			if ((uint8_t)(color >> 8) == (uint8_t)color)
			{
				for (int32_t y = fill.min_y; y <= fill.max_y; y++)
					memset(raw_pixptr(y, fill.min_x), (uint8_t)color, fill.width() * 2);
			}
			else
			{
				// Fill the first line the hard way
				uint16_t *destrow = &pixt<uint16_t>(fill.min_y);
				for (int32_t x = fill.min_x; x <= fill.max_x; x++)
					destrow[x] = (uint16_t)color;

				// For the other lines, just copy the first one
				void *destrow0 = &pixt<uint16_t>(fill.min_y, fill.min_x);
				for (int32_t y = fill.min_y + 1; y <= fill.max_y; y++)
				{
					destrow = &pixt<uint16_t>(y, fill.min_x);
					memcpy(destrow, destrow0, fill.width() * 2);
				}
			}
			break;

		case 32:
			// 32bpp can use memset if the bytes are equal
			if ((uint8_t)(color >> 8) == (uint8_t)color && (uint16_t)(color >> 16) == (uint16_t)color)
			{
				for (int32_t y = fill.min_y; y <= fill.max_y; y++)
					memset(&pixt<uint32_t>(y, fill.min_x), (uint8_t)color, fill.width() * 4);
			}
			else
			{
				// Fill the first line the hard way
				uint32_t *destrow  = &pixt<uint32_t>(fill.min_y);
				for (int32_t x = fill.min_x; x <= fill.max_x; x++)
					destrow[x] = (uint32_t)color;

				// For the other lines, just copy the first one
				uint32_t *destrow0 = &pixt<uint32_t>(fill.min_y, fill.min_x);
				for (int32_t y = fill.min_y + 1; y <= fill.max_y; y++)
				{
					destrow = &pixt<uint32_t>(y, fill.min_x);
					memcpy(destrow, destrow0, fill.width() * 4);
				}
			}
			break;

		case 64:
			// 64bpp can use memset if the bytes are equal
			if ((uint8_t)(color >> 8) == (uint8_t)color && (uint16_t)(color >> 16) == (uint16_t)color)
			{
				for (int32_t y = fill.min_y; y <= fill.max_y; y++)
					memset(&pixt<uint64_t>(y, fill.min_x), (uint8_t)color, fill.width() * 8);
			}
			else
			{
				// Fill the first line the hard way
				uint64_t *destrow  = &pixt<uint64_t>(fill.min_y);
				for (int32_t x = fill.min_x; x <= fill.max_x; x++)
					destrow[x] = (uint64_t)color;

				// For the other lines, just copy the first one
				uint64_t *destrow0 = &pixt<uint64_t>(fill.min_y, fill.min_x);
				for (int32_t y = fill.min_y + 1; y <= fill.max_y; y++)
				{
					destrow = &pixt<uint64_t>(y, fill.min_x);
					memcpy(destrow, destrow0, fill.width() * 8);
				}
			}
			break;
	}
}
