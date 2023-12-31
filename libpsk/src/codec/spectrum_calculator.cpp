/*
 * spectrum_calculator.cpp
 *
 *  Created on: 15-05-2013
 *      Author: lucck
 */

/*----------------------------------------------------------*/
#include "libpsk/codec/spectrum_calculator.hpp"
#include <foundation/dsp/fft.hpp>
#include <foundation/dsp/sqrt_int.hpp>
#include <foundation/dsp/log2_int.hpp>
#include <algorithm>
#include <limits>
/*----------------------------------------------------------*/
namespace ham {
namespace psk {
/*----------------------------------------------------------*/
namespace {
	//Inline POW2
	constexpr inline unsigned pow2( unsigned u ) {
		return u * u;
	}
}
/*----------------------------------------------------------*/
//! Copy samples into internal buffer
bool spectrum_calculator::copy_samples( const pow_t* samples, size_t len ) 
{
	size_t cpyn = WIDTH - m_sample_buf_cnt;
	if( len < cpyn ) cpyn = len;
	std::memcpy( &m_real[m_sample_buf_cnt], samples, sizeof(pow_t) * cpyn );
	m_sample_buf_cnt += cpyn;
	m_energy_calculated = false;
	if( m_sample_buf_cnt >= WIDTH ) {
		m_sample_buf_cnt = 0;
		return true;
	}
	else {
		return false;
	}
}
/*----------------------------------------------------------*/
//Calculate samplees
void spectrum_calculator::calculate_samples()
{
	namespace fft = dsp::refft;
	namespace dint = dsp::integer;
	//Calculate FFT real transform
	fft::fft_real( m_cplx, m_real, MLOG2 );
	//Calculate energies
	for( unsigned i=0; i<WIDTH/2; i++ ) {
		m_real[i] = dint::sqrt( pow2(m_cplx[i].real()) + pow2(m_cplx[i].imag()) );
	}
	//Normalize the graph and do log
	const int max_sample = *std::max_element( m_real, m_real + WIDTH/2 );
	const int max_value =  (m_scale==scale::log)?(std::numeric_limits<pow_t>::max()):(m_factor);
	for( unsigned i=0; i<WIDTH/2; i++ ) {
		m_real[i] = (int(m_real[i])* max_value) / max_sample;
		if(m_scale == scale::log ) {
			const int lsample = dint::log2_0_1<unsigned,LOG_SCALE>( m_real[i])>>LOGVAL_SCALE;
			if( m_factor == std::numeric_limits<pow_t>::max() )
				m_real[i] = lsample;
			else
				m_real[i] = (lsample*m_factor)/std::numeric_limits<pow_t>::max();
		}
	}
}
/*----------------------------------------------------------*/

} /* namespace psk */
} /* namespace ham */
/*----------------------------------------------------------*/
