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
#include "psk/dyn_queue.hpp"
#include "psk/symbol_encoder.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
/* PSK modulator class */
class modulator {
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
		off,
		sending,
		paused,
		preamble,
		postamble,
		tune
	};
	enum class baudrate
	{
		b31,
		b63,
		b125
	};
	//Constructor
	explicit modulator( int sample_freq, int tx_freq, std::size_t char_que_len );
	//Operator on new samples
	void operator()( int16_t* sample, size_t len );
	//Set char into the modulator
	void put_tx( short c );
	//Clear queue
	void clear_tx();
	//Set frequency
	void set_freqency( int frequency );
	//Set mode
	void set_mode( mode mmode, baudrate baud );
	//Get number of chars remaining
	size_t size_tx() const;
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

	static constexpr int m_vect_lookup[6][2] =
		{{0, 1000}, {1000, 0}, {0, -1000}, {-1000, 0}, {0, 0}, {0, 1000}};
	static constexpr short C_preamble_chr =  ctrl_chars::TXTOG_CODE;
	static constexpr short C_postamble_chr = ctrl_chars::TXTOG_CODE;
	static constexpr auto  C_amble_size = 32;

private:
	int get_tx_char();
	int get_char();

private:
	_internal::symbol_encoder m_encoder;
	const int m_sample_freq;
	mode m_mode { mode::bpsk };
	double m_t {};
	double m_psk_phase_inc;
	double (* m_p_psk_tx_i)( double, double) {};
	double (*m_p_psk_tx_q )( double, double ) {};
	double *tmp_i;
	double *tmp_q;
	int m_ramp {};
	double m_psk_sec_per_samp {};
	double m_psk_time {};
	double m_psk_period_update {};
	short m_present_phase {};
	int m_iq_phase_array[20] {};
	state m_state;
	bool m_need_shutoff { true };
	int m_amble_ptr {};
	bool m_no_squelch_tail {};
	fnd::dyn_queue<short> m_chqueue;
	bool m_temp_need_shutoff {};
	bool m_temp_no_squelch_tail {};
	double m_symbol_rate { 31.25 };
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */

/* ------------------------------------------------------------------------- */
#endif /* MODULATOR_HPP_ */

/* ------------------------------------------------------------------------- */
