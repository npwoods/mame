// license:BSD-3-Clause
// copyright-holders:Sandro Ronco
/*********************************************************************

    includes/x07.h

*********************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"
#include "sound/beep.h"
#include "machine/nvram.h"
#include "machine/ram.h"
#include "sound/wave.h"
#include "imagedev/cassette.h"
#include "imagedev/printer.h"
#include "formats/x07_cas.h"
#include "bus/generic/slot.h"
#include "bus/generic/carts.h"
#include "rendlay.h"

//default value for user defined keys, taken for official documentation
static const char *const udk_ini[12] = {
	"tim?TIME$\r",
	"cldCLOAD\"",
	"locLOCATE ",
	"lstLIST ",
	"runRUN\r",
	"",
	"dat?DATE$\r",
	"csvCSAVE\"",
	"prtPRINT ",
	"slpSLEEP",
	"cntCONT\r",
	""
};

static const uint16_t udk_offset[12] =
{
	0x0000, 0x002a, 0x0054, 0x007e, 0x00a8, 0x00d2,
	0x0100, 0x012a, 0x0154, 0x017e, 0x01a8, 0x01d2
};

static const uint8_t printer_charcode[256] =
{
	0xff, 0x7f, 0xbf, 0x3f, 0xdf, 0x5f, 0x9f, 0x1f,
	0xef, 0x6f, 0xaf, 0x2f, 0xcf, 0x4f, 0x8f, 0x0f,
	0xf7, 0x77, 0xb7, 0x37, 0xd7, 0x57, 0x97, 0x17,
	0xe7, 0x67, 0xa7, 0x27, 0xc7, 0x47, 0x87, 0x07,
	0xfb, 0x7b, 0xbb, 0x3b, 0xdb, 0x5b, 0x9b, 0x1b,
	0xeb, 0x6b, 0xab, 0x2b, 0xcb, 0x4b, 0x8b, 0x0b,
	0xf3, 0x73, 0xb3, 0x33, 0xd3, 0x53, 0x93, 0x13,
	0xe3, 0x63, 0xa3, 0x23, 0xc3, 0x43, 0x83, 0x03,
	0xfd, 0x7d, 0xbd, 0x3d, 0xdd, 0x5d, 0x9d, 0x1d,
	0xed, 0x6d, 0xad, 0x2d, 0xcd, 0x4d, 0x8d, 0x0d,
	0xf5, 0x75, 0xb5, 0x35, 0xd5, 0x55, 0x95, 0x15,
	0xe5, 0x65, 0xa5, 0x25, 0xc5, 0x45, 0x85, 0x05,
	0xf9, 0x79, 0xb9, 0x39, 0xd9, 0x59, 0x99, 0x19,
	0xe9, 0x69, 0xa9, 0x29, 0xc9, 0x49, 0x89, 0x09,
	0xf1, 0x71, 0xb1, 0x31, 0xd1, 0x51, 0x91, 0x11,
	0xe1, 0x61, 0xa1, 0x21, 0xc1, 0x41, 0x81, 0x01,
	0xfe, 0x7e, 0xbe, 0x3e, 0xde, 0x5e, 0x9e, 0x1e,
	0xee, 0x6e, 0xae, 0x2e, 0xce, 0x4e, 0x8e, 0x0e,
	0xf6, 0x76, 0xb6, 0x36, 0xd6, 0x56, 0x96, 0x16,
	0xe6, 0x66, 0xa6, 0x26, 0xc6, 0x46, 0x86, 0x06,
	0xfa, 0x7a, 0xba, 0x3a, 0xda, 0x5a, 0x9a, 0x1a,
	0xea, 0x6a, 0xaa, 0x2a, 0xca, 0x4a, 0x8a, 0x0a,
	0xf2, 0x72, 0xb2, 0x32, 0xd2, 0x52, 0x92, 0x12,
	0xe2, 0x62, 0xa2, 0x22, 0xc2, 0x42, 0x82, 0x02,
	0xfc, 0x7c, 0xbc, 0x3c, 0xdc, 0x5c, 0x9c, 0x1c,
	0xec, 0x6c, 0xac, 0x2c, 0xcc, 0x4c, 0x8c, 0x0c,
	0xf4, 0x74, 0xb4, 0x34, 0xd4, 0x54, 0x94, 0x14,
	0xe4, 0x64, 0xa4, 0x24, 0xc4, 0x44, 0x84, 0x04,
	0xf8, 0x78, 0xb8, 0x38, 0xd8, 0x58, 0x98, 0x18,
	0xe8, 0x68, 0xa8, 0x28, 0xc8, 0x48, 0x88, 0x08,
	0xf0, 0x70, 0xb0, 0x30, 0xd0, 0x50, 0x90, 0x10,
	0xe0, 0x60, 0xa0, 0x20, 0xc0, 0x40, 0x80, 0x00
};

static const uint8_t t6834_cmd_len[0x47] =
{
	0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x04, 0x03,
	0x01, 0x02, 0x09, 0x01, 0x09, 0x01, 0x01, 0x02,
	0x03, 0x03, 0x03, 0x03, 0x05, 0x04, 0x82, 0x02,
	0x01, 0x01, 0x0a, 0x02, 0x01, 0x81, 0x81, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x04, 0x01, 0x01, 0x03,
	0x02, 0x01, 0x02, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x82, 0x01, 0x01,
	0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
	0x01, 0x09, 0x01, 0x03, 0x03, 0x01, 0x01
};

struct x07_kb
{
	const char *tag;        //input port tag
	uint8_t       mask;       //bit mask
	uint8_t       codes[7];   //port codes
};

static const x07_kb x07_keycodes[56] =
{
	{"BZ", 0x01, {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b}},
	{"S1", 0x01, {0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12}},
	{"S1", 0x02, {0x16, 0x16, 0x16, 0x16, 0x16, 0x16, 0x16}},
	{"S1", 0x04, {0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c, 0x1c}},
	{"S1", 0x08, {0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d, 0x1d}},
	{"S1", 0x10, {0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e, 0x1e}},
	{"S1", 0x20, {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}},
	{"S1", 0x40, {0x20, 0x20, 0x20, 0x00, 0x20, 0x20, 0x20}},
	{"S2", 0x01, {0x5a, 0x7a, 0x1a, 0x00, 0xc2, 0xaf, 0xe1}},
	{"S2", 0x02, {0x58, 0x78, 0x18, 0x00, 0xbb, 0x00, 0x98}},
	{"S2", 0x04, {0x43, 0x63, 0x03, 0x00, 0xbf, 0x00, 0xe4}},
	{"S2", 0x08, {0x56, 0x76, 0x16, 0x00, 0xcb, 0x00, 0x95}},
	{"S2", 0x10, {0x42, 0x62, 0x02, 0x00, 0xba, 0x00, 0xed}},
	{"S2", 0x20, {0x4e, 0x6e, 0x0e, 0x00, 0xd0, 0x00, 0x89}},
	{"S2", 0x40, {0x4d, 0x6d, 0x0d, 0x30, 0xd3, 0x00, 0xf5}},
	{"S2", 0x80, {0x2c, 0x3c, 0x00, 0x00, 0xc8, 0xa4, 0x9c}},
	{"S3", 0x01, {0x41, 0x61, 0x01, 0x00, 0xc1, 0x00, 0x88}},
	{"S3", 0x02, {0x53, 0x73, 0x13, 0x00, 0xc4, 0x00, 0x9f}},
	{"S3", 0x04, {0x44, 0x64, 0x04, 0x00, 0xbc, 0x00, 0xef}},
	{"S3", 0x08, {0x46, 0x66, 0x06, 0x00, 0xca, 0x00, 0xfd}},
	{"S3", 0x10, {0x47, 0x67, 0x07, 0x00, 0xb7, 0x00, 0x9d}},
	{"S3", 0x20, {0x48, 0x68, 0x08, 0x00, 0xb8, 0x00, 0xfe}},
	{"S3", 0x40, {0x4a, 0x6a, 0x0a, 0x31, 0xcf, 0x00, 0xe5}},
	{"S3", 0x80, {0x4b, 0x6b, 0x0b, 0x32, 0xc9, 0x00, 0x9b}},
	{"S4", 0x01, {0x51, 0x71, 0x11, 0x00, 0xc0, 0x00, 0x8b}},
	{"S4", 0x02, {0x57, 0x77, 0x17, 0x00, 0xc3, 0x00, 0xfb}},
	{"S4", 0x04, {0x45, 0x65, 0x05, 0x00, 0xb2, 0xa8, 0x99}},
	{"S4", 0x08, {0x52, 0x72, 0x12, 0x00, 0xbd, 0x00, 0xf6}},
	{"S4", 0x10, {0x54, 0x74, 0x14, 0x00, 0xb6, 0x00, 0x97}},
	{"S4", 0x20, {0x59, 0x79, 0x19, 0x35, 0xdd, 0x00, 0x96}},
	{"S4", 0x40, {0x55, 0x75, 0x15, 0x34, 0xc5, 0x00, 0x94}},
	{"S4", 0x80, {0x49, 0x69, 0x09, 0x00, 0xc6, 0x00, 0xf9}},
	{"S5", 0x01, {0x31, 0x21, 0x00, 0x00, 0xc7, 0x00, 0xe9}},
	{"S5", 0x02, {0x32, 0x22, 0x00, 0x00, 0xcc, 0x00, 0x90}},
	{"S5", 0x04, {0x33, 0x23, 0x00, 0x00, 0xb1, 0xa7, 0x91}},
	{"S5", 0x08, {0x34, 0x24, 0x00, 0x00, 0xb3, 0xa9, 0x92}},
	{"S5", 0x10, {0x35, 0x25, 0x00, 0x00, 0xb4, 0xaa, 0x93}},
	{"S5", 0x20, {0x36, 0x26, 0x00, 0x00, 0xb5, 0xab, 0xec}},
	{"S5", 0x40, {0x37, 0x27, 0x37, 0x00, 0xd4, 0xac, 0xe0}},
	{"S5", 0x80, {0x38, 0x28, 0x37, 0x00, 0xd5, 0xad, 0xf2}},
	{"S7", 0x01, {0x2e, 0x3e, 0x00, 0x2e, 0xd9, 0xa1, 0x9a}},
	{"S7", 0x02, {0x2f, 0x3f, 0x00, 0x00, 0xd2, 0xa5, 0x80}},
	{"S7", 0x04, {0x3f, 0xa5, 0x00, 0x00, 0x00, 0x00, 0x00}},
	{"S7", 0x08, {0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d, 0x0d}},
	{"S7", 0x10, {0x4f, 0x6f, 0x0f, 0x36, 0xd7, 0x00, 0x9e}},
	{"S7", 0x20, {0x50, 0x70, 0x10, 0x00, 0xbe, 0x00, 0xf7}},
	{"S7", 0x40, {0x40, 0x60, 0x00, 0x00, 0xde, 0x00, 0xe7}},
	{"S7", 0x80, {0x5b, 0x7b, 0x00, 0x00, 0xdf, 0xa2, 0x84}},
	{"S8", 0x01, {0x4c, 0x6c, 0x0c, 0x33, 0xd8, 0x00, 0xf4}},
	{"S8", 0x02, {0x3b, 0x2b, 0x00, 0x00, 0xda, 0x00, 0x82}},
	{"S8", 0x04, {0x3a, 0x2a, 0x00, 0x00, 0xb9, 0x00, 0x81}},
	{"S8", 0x08, {0x5d, 0x7d, 0x00, 0x00, 0xd1, 0xa3, 0x85}},
	{"S8", 0x10, {0x39, 0x29, 0x39, 0x00, 0xd6, 0xae, 0xf1}},
	{"S8", 0x20, {0x30, 0x7c, 0x00, 0x00, 0xdc, 0xa6, 0x8a}},
	{"S8", 0x40, {0x2d, 0x3d, 0x00, 0x00, 0xce, 0x00, 0xf0}},
	{"S8", 0x80, {0x3d, 0x7e, 0x00, 0x00, 0xcd, 0x00, 0xfc}}
};

class x07_state : public driver_device
{
public:
	x07_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag),
			m_maincpu(*this, "maincpu"),
			m_printer(*this, "printer"),
			m_beep(*this, "beeper"),
			m_ram(*this, RAM_TAG),
			m_nvram1(*this, "nvram1"),
			m_nvram2(*this, "nvram2"),
			m_cassette(*this, "cassette"),
			m_card(*this, "cardslot"),
			m_warm_start(1)
	{ }

	required_device<cpu_device> m_maincpu;
	required_device<printer_image_device> m_printer;
	required_device<beep_device> m_beep;
	required_device<ram_device> m_ram;
	required_device<nvram_device> m_nvram1;
	required_device<nvram_device> m_nvram2;
	required_device<cassette_image_device> m_cassette;
	required_device<generic_slot_device> m_card;

	void machine_start() override;
	void machine_reset() override;
	uint32_t screen_update(screen_device &screen, bitmap_ind16 &bitmap, const rectangle &cliprect);
	DECLARE_READ8_MEMBER( x07_io_r );
	DECLARE_WRITE8_MEMBER( x07_io_w );

	DECLARE_INPUT_CHANGED_MEMBER( kb_keys );
	DECLARE_INPUT_CHANGED_MEMBER( kb_func_keys );
	DECLARE_INPUT_CHANGED_MEMBER( kb_break );
	DECLARE_INPUT_CHANGED_MEMBER( kb_update_udk );

	DECLARE_DRIVER_INIT(x07);
	void nvram_init(nvram_device &nvram, void *data, size_t size);

	void t6834_cmd(uint8_t cmd);
	void t6834_r();
	void t6834_w();
	void cassette_w();
	void cassette_r();
	void printer_w();
	void kb_irq();
	void cassette_load();
	void cassette_save();
	void receive_bit(int bit);

	inline uint8_t get_char(uint16_t pos);
	inline uint8_t kb_get_index(uint8_t char_code);
	inline void draw_char(uint8_t x, uint8_t y, uint8_t char_pos);
	inline void draw_point(uint8_t x, uint8_t y, uint8_t color);
	inline void draw_udk();

	DECLARE_DEVICE_IMAGE_LOAD_MEMBER( x07_card );

	/* general */
	uint8_t m_sleep;
	uint8_t m_warm_start;
	uint8_t m_t6834_ram[0x800];
	uint8_t m_regs_r[8];
	uint8_t m_regs_w[8];
	uint8_t m_alarm[8];

	struct fifo_buffer
	{
		uint8_t data[0x100];
		uint8_t read;
		uint8_t write;
	};
	fifo_buffer m_in;
	fifo_buffer m_out;

	uint8_t m_udk_on;
	uint8_t m_draw_udk;
	uint8_t m_sp_on;
	uint8_t m_font_code;
	emu_timer *m_rsta_clear;
	emu_timer *m_rstb_clear;
	emu_timer *m_beep_stop;

	/* LCD */
	uint8_t m_lcd_on;
	uint8_t m_lcd_map[32][120];
	uint8_t m_scroll_min;
	uint8_t m_scroll_max;
	uint8_t m_blink;

	struct lcd_position
	{
		uint8_t x;
		uint8_t y;
		uint8_t on;
	};
	lcd_position m_locate;
	lcd_position m_cursor;

	/* keyboard */
	uint8_t m_kb_on;
	uint8_t m_repeat_key;         //not supported
	uint8_t m_kb_size;

	/* cassette */
	uint8_t  m_cass_motor;
	uint8_t  m_cass_data;
	uint32_t m_cass_clk;
	uint8_t  m_bit_count;
	int    m_cass_state;
	emu_timer *m_cass_poll;
	emu_timer *m_cass_tick;

	/* printer */
	uint8_t m_prn_sendbit;
	uint8_t m_prn_char_code;
	uint8_t m_prn_buffer[0x100];
	uint8_t m_prn_size;
	DECLARE_PALETTE_INIT(x07);
	TIMER_CALLBACK_MEMBER(cassette_tick);
	TIMER_CALLBACK_MEMBER(cassette_poll);
	TIMER_CALLBACK_MEMBER(rsta_clear);
	TIMER_CALLBACK_MEMBER(rstb_clear);
	TIMER_CALLBACK_MEMBER(beep_stop);
	TIMER_DEVICE_CALLBACK_MEMBER(blink_timer);
};
