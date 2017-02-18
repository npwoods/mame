// license:BSD-3-Clause
// copyright-holders:Nathan Woods, Miodrag Milanovic
/*********************************************************************

    bitbngr.h

    TRS style "bitbanger" serial port

*********************************************************************/

#ifndef MAME_DEVICES_IMAGEDEV_BITBNGR_H
#define MAME_DEVICES_IMAGEDEV_BITBNGR_H


enum
{
	BITBANGER_PRINTER           = 0,
	BITBANGER_MODEM,
	BITBANGER_MODE_MAX,

	BITBANGER_150               = 0,
	BITBANGER_300,
	BITBANGER_600,
	BITBANGER_1200,
	BITBANGER_2400,
	BITBANGER_4800,
	BITBANGER_9600,
	BITBANGER_14400,
	BITBANGER_19200,
	BITBANGER_28800,
	BITBANGER_38400,
	BITBANGER_57600,
	BITBANGER_115200,
	BITBANGER_BAUD_MAX,

	BITBANGER_NEG40PERCENT  = 0,
	BITBANGER_NEG35PERCENT,
	BITBANGER_NEG30PERCENT,
	BITBANGER_NEG25PERCENT,
	BITBANGER_NEG20PERCENT,
	BITBANGER_NEG15PERCENT,
	BITBANGER_NEG10PERCENT,
	BITBANGER_NEG5PERCENT,
	BITBANGER_0PERCENT,
	BITBANGER_POS5PERCENT,
	BITBANGER_POS10PERCENT,
	BITBANGER_POS15PERCENT,
	BITBANGER_POS20PERCENT,
	BITBANGER_POS25PERCENT,
	BITBANGER_POS30PERCENT,
	BITBANGER_POS35PERCENT,
	BITBANGER_POS40PERCENT,
	BITBANGER_TUNE_MAX
};

/***************************************************************************
    CONSTANTS
***************************************************************************/

#define MCFG_BITBANGER_ADD(_tag, _intrf) \
	MCFG_DEVICE_ADD(_tag, BITBANGER, 0) \
	MCFG_DEVICE_CONFIG(_intrf)


/***************************************************************************
    TYPE DEFINITIONS
***************************************************************************/

#if 0 // TODO
struct bitbanger_config
{
	// callback to driver
	devcb_write_line        m_input_callback;

	// emulating a printer or modem
	int                     m_default_mode;

	// output bits per second
	int                     m_default_baud;

	// fine tune adjustment to the baud
	int                     m_default_tune;
};
#endif


class bitbanger_device : public device_t,
	public device_image_interface
{
public:
	// construction/destruction
	bitbanger_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);

	// image-level overrides
	virtual image_init_result call_load() override;
	virtual image_init_result call_create(const image_device_format *create_format, util::option_resolution *format_options) override;
	virtual void call_unload() override;

	// image device
	virtual iodevice_t image_type() const override { return IO_SERIAL; }
	virtual bool is_readable()  const override { return 1; }
	virtual bool is_writeable() const override { return 1; }
	virtual bool is_creatable() const override { return 1; }
	virtual bool must_be_loaded() const override { return 0; }
	virtual bool is_reset_on_load() const override { return 0; }
	virtual const char *file_extensions() const override { return ""; }

	uint32_t input(void *buffer, uint32_t length)
	{
		return 0;
	}

	// outputs data to a bitbanger port
	void output(int value);

	// reads the current input value
	int input() const { return m_current_input; }

	// ui functions
	const char *mode_string();
	const char *baud_string();
	const char *tune_string();
	bool inc_mode(bool test);
	bool dec_mode(bool test);
	bool inc_tune(bool test);
	bool dec_tune(bool test);
	bool inc_baud(bool test);
	bool dec_baud(bool test);

protected:
	// device-level overrides
	virtual void device_start() override;
	virtual void device_config_complete() override;
	virtual void device_timer(emu_timer &timer, device_timer_id id, int param, void *ptr) override;

private:
	// constants
	static const device_timer_id TIMER_OUTPUT = 0;
	static const device_timer_id TIMER_INPUT = 1;

	// methods
	void native_output(uint8_t data);
	uint32_t native_input(void *buffer, uint32_t length);
	void bytes_to_bits_81N(void);
	void timer_input(void);
	void timer_output(void);
	float tune_value(void);
	uint32_t baud_value(void);
	void set_input_line(uint8_t line);

	
	// configuration
	devcb_write_line        m_input_callback;	// callback to driver
	int                     m_default_mode;		// emulating a printer or modem
	int                     m_default_baud;		// output bits per second
	int                     m_default_tune;		// fine tune adjustment to the baud

	// variables
	emu_timer *             m_output_timer;
	emu_timer *             m_input_timer;
	devcb_write_line		m_input_func;
	int                     m_output_value;
	int                     m_build_count;
	int                     m_build_byte;
	attotime                m_idle_delay;
	attotime                m_current_baud;
	uint32_t                m_input_buffer_size;
	uint32_t                m_input_buffer_cursor;
	int                     m_mode;
	int                     m_baud;
	int                     m_tune;
	uint8_t                 m_current_input;
	uint8_t                 m_input_buffer[1000];
};

#define MCFG_BITBANGER_INTERFACE(_interface) \
	bitbanger_image_device::static_set_interface(*device, _interface);

// device type definition
extern const device_type BITBANGER;

#endif // MAME_DEVICES_IMAGEDEV_BITBNGR_H
