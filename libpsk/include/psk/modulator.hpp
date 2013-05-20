/*
 * modulator.hpp
 *
 *  Created on: 08-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_MODULATOR_HPP_
#define LIBPSK_MODULATOR_HPP_
/* ------------------------------------------------------------------------- */
#include <cstddef>
#include <cstdint>
#include <array>
#include "psk/dyn_queue.hpp"
#include "psk/symbol_encoder.hpp"
#include "dsp/nco_mixer.hpp"
#include "codec/codec_types.hpp"
#include "codec/trx_device_base.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
/* PSK modulator class */
class modulator : public tx_codec {
	//Make object noncopyable
	modulator(const modulator&) = delete;
	modulator& operator=(const modulator&) = delete;
public:
	//Enum special chars
	enum class mode
	{
		bpsk,
		qpsku,
		qpskl,
		tune,
	};

	enum class state
	{
		off,				//Decoder is off
		sending,			//Decoder sending data
		paused,				//Decoder is paused
		preamble,			//Decoder sending preamble
		postamble,			//Decoder sending postamble
		tune				//Decoder in tune state
	};
	enum class baudrate
	{
		b31,				//Baudrate 31
		b63,				//Baudrate 63
		b125				//Baudrate 125
	};
	//Constructor
	explicit modulator( int sample_freq, int tx_freq, std::size_t char_que_len );
	//Operator on new samples
	virtual unsigned operator()( sample_type* sample, size_t len );
	//Set char into the modulator
	virtual void put_tx( short c );
	//Clear queue
	virtual void clear_tx();
	//Set frequency
	virtual void set_freqency( int frequency );
	//Set mode
	void set_mode( mode mmode, baudrate baud );
	//Get number of chars remaining
	virtual size_t get_count() const
	{
		return m_chqueue.size();
	}
	//Set auto shut off
	void set_auto_shutoff( bool en = true )
	{
		m_need_shutoff = en;
	}
	//Get tx state
	state get_state() const
	{
		return m_state;
	}
	//TODO: Add reset support
	virtual void reset()
	{}
private:
	//Vector lookup table
	static constexpr int m_vect_lookup[6][2] =
		{{0, 1000}, {1000, 0}, {0, -1000}, {-1000, 0}, {0, 0}, {0, 1000}};
	//Preamble and postamble char defs and len
	static constexpr short C_preamble_chr =  ctrl_chars::TXTOG_CODE;
	static constexpr short C_postamble_chr = ctrl_chars::TXTOG_CODE;
	static constexpr auto  C_amble_size = 32;
	//Rate scale for the symbol rate
	static constexpr unsigned RATE_SCALE = 100;
	//PI2I wrap arround
	static constexpr int PI2I = 1<<15;
private:
	//Get tx char from queue
	int get_tx_char();
	//Update state return char
	int update_state_chr();
private:
	_internal::symbol_encoder m_encoder;
	const int m_sample_freq;
	mode m_mode { mode::bpsk };
	int m_psk_phase_inc;
	int (*m_p_psk_tx_i)(int, int) {};
	int (*m_p_psk_tx_q)(int, int) {};
	int m_ramp {};
	dsp::nco_mixer<short, int ,512, PI2I> m_nco_mix;
	size_t m_psk_sample_cnt {};
	short m_present_phase {};
	//Change this to array
	std::array<int, 16> m_iq_phase_array {{}};
	state m_state;
	bool m_need_shutoff { true };
	int m_amble_ptr {};
	bool m_no_squelch_tail {};
	fnd::dyn_queue<short> m_chqueue;
	bool m_temp_need_shutoff {};
	bool m_temp_no_squelch_tail {};
	short m_symbol_rate { 3125 };
	unsigned m_psk_period_update;
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */

/* ------------------------------------------------------------------------- */
#endif /* MODULATOR_HPP_ */

/* ------------------------------------------------------------------------- */
