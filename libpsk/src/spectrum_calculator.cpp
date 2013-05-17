/*
 * spectrum_calculator.cpp
 *
 *  Created on: 15-05-2013
 *      Author: lucck
 */

/*----------------------------------------------------------*/
#include "psk/spectrum_calculator.hpp"
#include "dsp/fft.h"
#include "dsp/sqrt_int.hpp"
#include "dsp/log2_int.hpp"
#include <algorithm>
#include <limits>
/*----------------------------------------------------------*/
namespace ham {
namespace psk {
/*----------------------------------------------------------*/
namespace
{
	//Inline POW2
	constexpr inline unsigned pow2( unsigned u )
	{
		return u * u;
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
	for( int i=0; i<WIDTH/2; i++ )
	{
		m_real[i] = dint::sqrt( pow2(m_cplx[i].real()) + pow2(m_cplx[i].imag()) );
	}
	//Normalize the graph and do log
	const int maxe = *std::max_element( m_real, m_real + WIDTH/2 );
	for( int i=0; i<WIDTH/2; i++ )
	{
		m_real[i] = (int(m_real[i])*  std::numeric_limits<short>::max()) / maxe;
		if(m_scale == scale::log )
			m_real[i] = dint::log2_0_1<unsigned,LOG_SCALE>( m_real[i])>>LOGVAL_SCALE;
	}
}
/*----------------------------------------------------------*/

} /* namespace psk */
} /* namespace ham */
/*----------------------------------------------------------*/
