/*
 * imd_calculator.cpp
 *
 *  Created on: 24-03-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#include "psk/imd_calculator.hpp"

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {

/* ------------------------------------------------------------------------- */
namespace
{
	constexpr auto PI2 = 8.0 * std::atan(1.0);
	constexpr auto N = 288;			//96 x 2 = Goertzel length(must be an integer value)
	constexpr auto FS = 500.0;		// sample frequency
	constexpr auto F0 = 15.625;	// bin frequencies
	constexpr auto F1 = 31.25;
	constexpr auto F2 = 46.875;
	constexpr auto K0 = double(N)*F0/FS;
	constexpr auto K1 = double(N)*F1/FS;
	constexpr auto K2 = double(N)*F2/FS;
	static constexpr double COEF[] =
	{
		2.0*std::cos(PI2*K0/double(N)),
		2.0*std::cos(PI2*K1/double(N)),
		2.0*std::cos(PI2*K2/double(N))
	};
}

/* ------------------------------------------------------------------------- */
void imd_calculator::reset()
{
	I1.fill(0);
	I2.fill(0);
	Q1.fill(0);
	Q2.fill(0);
	m_ncount = 0;
}
/* ------------------------------------------------------------------------- */
bool imd_calculator::calc_energies( std::complex<int> samp )
{
	std::complex<double> temp;
	for(int i=0; i<NUM_FILTERS;i++)
	{
		temp = std::complex<double>(I1[i], Q1[i]);
		I1[i] = I1[i]*COEF[i]-I2[i]+samp.real();
		Q1[i] = Q1[i]*COEF[i]-Q2[i]+samp.imag();
		I2[i] = temp.real(); Q2[i] = temp.imag();
	}
	if( ++m_ncount >= N )
	{
		m_ncount = 0;
		for(int i=0; i<NUM_FILTERS;i++)
		{
			m_energy[i] = I1[i]*I1[i] + I2[i]*I2[i] - I1[i]*I2[i]*COEF[i] +
						Q1[i]*Q1[i] + Q2[i]*Q2[i] - Q1[i]*Q2[i]*COEF[i];
			I1[i] = I2[i] = Q1[i] = Q2[i] = 0.0;
		}
		return true;
	}
	else
		return false;
}
/* ------------------------------------------------------------------------- */
bool imd_calculator::calc_value( int &imd_val )
{
	m_snr = 10.0*std::log10(m_energy[0]/m_energy[1]);
	m_imd = 10.0*std::log10(m_energy[2]/m_energy[0]);
	imd_val = int(m_imd);
	return m_snr > (-m_imd+6);
}

/* ------------------------------------------------------------------------- */

} /* namespace _internal */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
