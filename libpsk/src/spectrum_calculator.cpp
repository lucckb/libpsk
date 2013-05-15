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
	//TODO: FInaly calculate only to WITDTH/2
	for( int i=0; i<WIDTH; i++ )
	{
		m_real[i] = dint::sqrt( pow2(m_cplx[i].real()) + pow2(m_cplx[i].imag()) );
	}
}
/*----------------------------------------------------------*/

} /* namespace psk */
} /* namespace ham */
/*----------------------------------------------------------*/
