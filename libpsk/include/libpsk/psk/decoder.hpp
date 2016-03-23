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
#include <foundation/dsp/nco_mixer.hpp>
#include <foundation/dsp/fir.hpp>
#include "agc.hpp"
#include "afc.hpp"
#include "synchronizer.hpp"
#include "diff_angle_calc.hpp"
#include "viterbi_decoder.hpp"
#include "squelch.hpp"
#include "symbol_decoder.hpp"
#include "libpsk/codec/codec_types.hpp"
#include "libpsk/codec/trx_device_base.hpp"
#include "libpsk/psk/dyn_queue.hpp"
#include "libpsk/codec/psk_config.hpp"

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
class decoder : public rx_codec {
	//Make object noncopyable
	decoder(const decoder&) = delete;
	decoder& operator=(const decoder&) = delete;
	static constexpr auto DEC4_LPFIR_LENGTH = 35;
	static constexpr auto BITFIR_LENGTH = 65;
	static constexpr int SCALE = 1<<15;
	static constexpr int PI2I = 1<<15;
public:
	static constexpr auto RCODE_ERR = -1;
	static constexpr auto RCODE_OK = 0;
	enum cbevent {
		cb_rxchar,
		cb_clkerror,
		cb_imdrdy
	};
	//Construct the decoder object
	explicit decoder( samplerate_type sample_rate, handler_t callback );
	//Process input sample buffer
	virtual void operator()( const sample_type* samples, std::size_t sample_size );
	//Get signal vector
	virtual bool get_vector_data( signal_vector_type &vector ) const
	{
		vector = m_angle_calc.get_iq_phase_array();
		return false;
	}
	//Reset decoder
	virtual void reset();
	//Set receive frequency
	virtual void set_frequency( int freq );
	//Set detector mode
	virtual int set_mode( const modulation_config_base& cfg );
	//Get detector mode
	virtual int get_mode( modulation_config_base& cfg );
	//Set AFC limit
	virtual bool set_afc_limit( int limit ) {
		m_afc.set_afc_limit( limit, m_sample_freq,  m_nco_phzinc );
		return false;
	}
	//Get afc limit
	virtual int get_afc_limit() const {
		return m_afc.get_afc_limit();
	}
	//Get current frequency
	virtual int get_frequency() const {
		return m_rx_frequency;
	}
	//Get signal level
	virtual sqelch_value_type get_signal_level() const {
		return m_squelch.get_level();
	}
	virtual bool get_sync_data(sync_array_type& sync_data) const {
		sync_data =  m_sync.get_sync_data();
		return false;
	}
	//Set squelch tresh
	virtual void set_squelch( sqelch_value_type tresh, squelch_mode mode ) {
		m_squelch.set_thresh( tresh, mode );
	}
	//! Get squelch
	virtual sqelch_value_type get_squelch() const {
		return m_squelch.get_thresh();
	}
private:
	typedef mod_psk_config::mode mode;
	typedef mod_psk_config::baud baudrate;
	int decode_symb( std::complex<int> newsamp );
	bool is_qpsk() const {
		return m_rx_mode != mode::bpsk;
	}
	//Numeric controlled oscillator and mixer
	dsp::nco_mixer<short, int ,512, PI2I> m_nco_mix;
	baudrate m_baudrate {  baudrate::b63 };
	int m_rx_frequency { 1000 };
	int m_nco_phzinc;
	_internal::symbol_synchronizer m_sync;
	//TODO: FIXME: Temporary viterbi decoder is here
	_internal::viterbi_decoder m_viterbi_decoder;
	int m_sample_cnt {};
	_internal::imd_calculator m_calc_imd;
	const int m_sample_freq;
	_internal::agc m_agc;
	_internal::afc m_afc;
    _internal::diff_angle_calc m_angle_calc;
    _internal::squelch m_squelch;
    _internal::symbol_decoder m_symb_decoder;
	mode m_rx_mode { mode::bpsk };
    //FIR1 BITFIR_LENGTH
    dsp::fir< std::complex<short>,  short, DEC4_LPFIR_LENGTH, std::complex<long long> > m_fir1_dec;
    dsp::fir< std::complex<short>,  short, DEC4_LPFIR_LENGTH,  std::complex<long long> > m_fir2_dec;
    dsp::fir< std::complex<short>,  short, BITFIR_LENGTH, std::complex<long long> > m_bit_fir;
    dsp::fir< std::complex<short>,  short, BITFIR_LENGTH, std::complex<long long> > m_freq_fir;
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */
#endif /* DECODER_H_ */
