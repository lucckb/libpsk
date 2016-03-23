/*
 * agc_control.hpp
 *
 *  Created on: 25-04-2013
 *      Author: lucck
 */

#ifndef AGC_CONTROL_HPP_
#define AGC_CONTROL_HPP_
/* ------------------------------------------------------------------------- */
#include <complex>
#include <limits>
#include <foundation/dsp/sqrt_int.hpp>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
class agc
{
	static constexpr double SCALE = 1<<15;
	//Make object noncopyable
	agc(const agc&) = delete;
	agc& operator=(const agc&) = delete;
public:
	//Default constructor
	agc() {}
	//Operator for do AGC
	void operator()( std::complex<int> sample )
	{
		constexpr int K1_F = (1.0-1.0/200.0) * SCALE + 0.5;
		constexpr int K2_F = (1.0/200.0) * SCALE + 0.5;
		constexpr int K1_S = (1.0-1.0/500.0) * SCALE + 0.5;
		constexpr int K2_S = (1.0/500.0) * SCALE + 0.5;
		int mag = dsp::integer::sqrt(sample.real()*sample.real() + sample.imag()*sample.imag() );
		if( mag > m_agc_ave )
		{
			m_agc_ave = (K1_F*m_agc_ave + K2_F*mag)/SCALE;
		}
		else
		{
			m_agc_ave = (K1_S*m_agc_ave + K2_S*mag)/SCALE;
		}
	}
	//Get AGC value
	int operator()() const
	{
		return m_agc_ave;
	}
	//Scale to the AGC value
	std::complex<int> scale( std::complex<int> v )
	{
		if( m_agc_ave>0 && (SCALE/m_agc_ave) > 0)
		{
			return std::complex<int>( (v.real()*SCALE)/m_agc_ave, (v.imag()*SCALE)/m_agc_ave );
		}
		else
		{
			return v;
		}
	}
private:
	int m_agc_ave {};
};

/* ------------------------------------------------------------------------- */
}}}



#endif /* AGC_CONTROL_HPP_ */
