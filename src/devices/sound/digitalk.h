// license:BSD-3-Clause
// copyright-holders:Olivier Galibert
#ifndef _DIGITALKER_H_
#define _DIGITALKER_H_


//**************************************************************************
//  INTERFACE CONFIGURATION MACROS
//**************************************************************************

#define MCFG_DIGITALKER_ADD(_tag, _clock) \
	MCFG_DEVICE_ADD(_tag, DIGITALKER, _clock)
#define MCFG_DIGITALKER_REPLACE(_tag, _clock) \
	MCFG_DEVICE_REPLACE(_tag, DIGITALKER, _clock)


//**************************************************************************
//  TYPE DEFINITIONS
//**************************************************************************

// ======================> digitalker_device

class digitalker_device : public device_t,
							public device_sound_interface
{
public:
	digitalker_device(const machine_config &mconfig, const char *tag, device_t *owner, uint32_t clock);
	~digitalker_device() { }

	void digitalker_0_cs_w(int line);
	void digitalker_0_cms_w(int line);
	void digitalker_0_wr_w(int line);
	int digitalker_0_intr_r();

protected:
	// device-level overrides
	virtual void device_start() override;

	// sound stream update overrides
	virtual void sound_stream_update(sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) override;

public:
	DECLARE_WRITE8_MEMBER(digitalker_data_w);

private:
	void digitalker_write(uint8_t *adr, uint8_t vol, int8_t dac);
	uint8_t digitalker_pitch_next(uint8_t val, uint8_t prev, int step);
	void digitalker_set_intr(uint8_t intr);
	void digitalker_start_command(uint8_t cmd);
	void digitalker_step_mode_0();
	void digitalker_step_mode_1();
	void digitalker_step_mode_2();
	void digitalker_step_mode_3();
	void digitalker_step();
	void digitalker_cs_w(int line);
	void digitalker_cms_w(int line);
	void digitalker_wr_w(int line);
	int digitalker_intr_r();
	void digitalker_register_for_save();

private:
	required_region_ptr<uint8_t> m_rom;
	sound_stream *m_stream;

	// Port/lines state
	uint8_t m_data;
	uint8_t m_cs;
	uint8_t m_cms;
	uint8_t m_wr;
	uint8_t m_intr;

	// Current decoding state
	uint16_t m_bpos;
	uint16_t m_apos;

	uint8_t m_mode;
	uint8_t m_cur_segment;
	uint8_t m_cur_repeat;
	uint8_t m_segments;
	uint8_t m_repeats;

	uint8_t m_prev_pitch;
	uint8_t m_pitch;
	uint8_t m_pitch_pos;

	uint8_t m_stop_after;
	uint8_t m_cur_dac;
	uint8_t m_cur_bits;

	// Zero-range size
	uint32_t m_zero_count; // 0 for done

	// Waveform and current index in it
	uint8_t m_dac_index; // 128 for done
	int16_t m_dac[128];
};

extern const device_type DIGITALKER;


#endif
