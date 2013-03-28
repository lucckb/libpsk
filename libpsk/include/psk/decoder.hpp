/*
 * decoder.h
 *
 *  Created on: 24-03-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_DECODER_HPP_
#define LIBPSK_DECODER_HPP_

/* ------------------------------------------------------------------------- */
#include <cstddef>
#include <array>
#include <complex>
#include <functional>
#include "imd_calculator.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Sample type variable
typedef int16_t sample_type;
//Vector data
typedef std::array<long, 16> signal_vector_type;
//Sync vector data
typedef std::array<long, 16> sync_array_type;
//Squelch tresh type
typedef int sqelch_value_type;
//Sample rate type
typedef short samplerate_type;

//Event calback type
typedef std::function< void( int event, int param1, int param2 ) > event_callback_type;
/* ------------------------------------------------------------------------- */
class decoder {
	//Make object noncopyable
	decoder(const decoder&) = delete;
	decoder& operator=(const decoder&) = delete;
	static constexpr auto DEC4_LPFIR_LENGTH = 35;
	static constexpr auto BITFIR_LENGTH = 65;
	//Survivor states in decoder
	struct survivor_states
	{
		survivor_states( double _path_distance = double(), long _bit_estimates = long() )
			: path_distance(_path_distance),  bit_estimates(_bit_estimates ) {}
		double path_distance;	// sum of all metrics for a given survivor path
		long bit_estimates;		// the bit pattern estimate associated with given survivor path
	};

public:
	enum cbevent
	{
		cb_rxchar,
		cb_clkerror,
		cb_imdrdy
	};
	enum class mode
	{
		bpsk,			//BPSK
		qpsku,			//QPSK USB
		qpskl			//QPSK LSB
	};
	enum class baudrate
	{
		b31,
		b63,
		b125
	};
	enum class squelch_mode
	{
		slow,
		fast
	};
	//Construct the decoder object
	explicit decoder( samplerate_type sample_rate, event_callback_type  callback );
	//Process input sample buffer
	void operator()( const sample_type* samples, std::size_t sample_size );
	//Get signal vector
	signal_vector_type get_vector_data( ) const
	{
		return m_iq_phase_array;
	}
	//Get sync array type
	sync_array_type get_sync_data() const
	{
		return m_sync_array;
	}
	//Reset decoder
	void reset();
	//Set receive frequency
	void set_frequency( int freq );
	//Set detector mode
	void set_mode( mode mode, baudrate rate )
	{
		m_rx_mode = mode;
		m_baudrate = rate;
	}
	//Set AFC limit
	void set_afc_limit( int limit );
	//Get current frequency
	int get_frequency() const
	{
		return m_rx_frequency;
	}
	//Get signal level
	int get_signal_level() const
	{
		return m_sql_level>0?m_sql_level:0;
	}
	//Set squelch tresh
	void set_squelch_tresh( sqelch_value_type tresh, squelch_mode mode );
private:
	void calc_quality( double angle );
	bool viterbi_decode( double newangle );
	void calc_bit_filter( std::complex<double> samp );
	void calc_agc( std::complex<double> samp );
	void calc_freq_error( std::complex<double> IQ );
	void calc_ffreq_error( std::complex<double> IQ );
	void decode_symb( std::complex<double> newsamp );
	bool symb_sync(std::complex<double> sample);
	bool is_qpsk() const
	{
		return m_rx_mode != mode::bpsk;
	}
private:
	//Event handler
	event_callback_type m_callback;
	baudrate m_baudrate {  baudrate::b63 };
	double m_vco_phz {};
	int m_afc_timer {};
	bool m_afc_capture_on {};
	int m_rx_frequency { 1500 };
	double m_nco_phzinc;
	double m_afc_limit;
	double m_afc_max { m_nco_phzinc + m_afc_limit };
	double m_afc_min { m_nco_phzinc - m_afc_limit };
	std::array<std::complex<double>, DEC4_LPFIR_LENGTH> m_que1;
	std::array<std::complex<double>, DEC4_LPFIR_LENGTH> m_que2;
	std::array<std::complex<double>, BITFIR_LENGTH> m_que3;
	int m_fir1_state;
	int m_fir2_state;
	int m_fir3_state;
	std::array<survivor_states, 16> m_survivor_states;
	std::array<long , 16> m_iq_phase_array;
	std::array<double, 21> m_sync_ave;
	int m_sql_level;
	int m_clk_err_counter;
	int m_clk_err_timer;
	double m_dev_ave;
	double m_freq_error {};
	int m_sample_cnt {};
	std::complex<double> m_freq_signal;
	bool m_fast_afc_mode {};
	std::complex<double> m_bit_signal;
	bool m_imd_valid {};
	_internal::imd_calculator m_calc_imd;
	double m_sample_freq;
	double m_agc_ave {};
	bool m_afc_on {};
	double m_fperr_ave {};
	double m_fferr_ave {};
	std::complex<double> m_z1;
	std::complex<double> m_z2;
	double m_I0 {};		// 4 stage I/Q delay line variables
	double m_I1 {};
	double m_Q0 {};
	double m_Q1 {};
	int m_iq_phz_index {};
	mode m_rx_mode { mode::bpsk };
	bool m_last_bit_zero {};
	uint16_t m_bit_acc {};
	bool m_sq_open {};
	int m_squelch_speed { 75 };
	double m_q_freq_error {};
	int m_on_count {};
	int m_off_count {};
	int m_pcnt {};
	int m_ncnt {};
	int m_sq_thresh { 50 };
	double m_nlp_k;
	int m_bit_pos {};
	int m_pk_pos {};
	int m_new_pk_pos { 5 };
	std::array<long, 16> m_sync_array;
	double m_bit_phase_pos {};
	double m_bit_phase_inc { 16.0 / m_sample_freq };
	int m_last_pk_pos {};
	int m_clk_error {};
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */
#endif /* DECODER_H_ */
