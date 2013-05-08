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
	explicit modulator( int sample_freq, int tx_freq );
	//Operator on new samples
	void operator()( int16_t* sample, size_t len );
	//Set char into the modulator
	void put_tx( char c );
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
private:
	enum sym {
		SYM_NOCHANGE,	//Stay the same phase
		SYM_P90,		//Plus 90  deg
		SYM_P180,		//Plus 180 deg
		SYM_M90,		//Minus 90 deg
		SYM_OFF,		//No output
		SYM_ON			//constant output
	};

	static constexpr int m_vect_lookup[6][2] =
		{{0, 1000}, {1000, 0}, {0, -1000}, {-1000, 0}, {0, 0}, {0, 1000}};

private:
	int get_tx_char();
	int get_char();
	modulator::sym get_next_bpsk_symbol();
	modulator::sym get_next_qpsk_symbol();
	modulator::sym get_next_tune_symbol();
private:
	const int m_sample_freq;
	mode m_mode { mode::bpsk };
	double m_t {};
	double m_psk_phase_inc;
	double* m_p_psk_tx_i {};
	double* m_p_psk_tx_q {};
	int m_ramp {};
	double m_psk_sec_per_samp {};
	double m_psk_time {};
	double m_psk_period_update {};
	short m_present_phase {};
	int m_iq_phase_array[20] {};
	sym m_last_symb {};
	bool  m_add_ending_zero {};
	short m_tx_shift_reg {};
	state m_state;
	bool m_need_shutoff { true };
	int m_amble_ptr {};
	int m_postamble[33] {};
	int m_preamble[33] {};
	bool m_no_squelch_tail {};
	int m_p_tail {};
	int m_p_head {};
	static constexpr auto  TX_BUF_SIZE = 256;
	short m_p_xmit_que[TX_BUF_SIZE];
	bool m_temp_need_shutoff {};
	bool m_temp_no_squelch_tail {};
	double m_symbol_rate { 31.25 };
	short m_tx_code_word {};
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */

/* ------------------------------------------------------------------------- */
#endif /* MODULATOR_HPP_ */

/* ------------------------------------------------------------------------- */
