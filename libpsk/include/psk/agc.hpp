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
#include "dsp/sqrt_int.hpp"

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
template< int SCALE, typename T>
class agc
{
public:
	void operator()( std::complex<T> sample )
	{
		constexpr T K1_F = (1.0-1.0/200.0) * double(SCALE);
		constexpr T K2_F = (1.0/200.0) * double(SCALE);
		constexpr T K1_S = (1.0-1.0/500.0) * double(SCALE);
		constexpr T K2_S = (1.0/500.0) * double(SCALE);
		T mag = dsp::integer::sqrt(sample.real()*sample.real() + sample.imag()*sample.imag() );
		if( mag > m_agc_ave )
		{
			//std::cout << "A";
			m_agc_ave = (K1_F*m_agc_ave + K2_F*mag)/SCALE;
		}
		else
		{
			//std::cout << "B";
			m_agc_ave = (K1_S*m_agc_ave + K2_S*mag)/SCALE;
		}
	}
	T operator()() const
	{
		return m_agc_ave;
	}
	std::complex<T> scale( std::complex<T> v )
	{
		if( m_agc_ave>0 && (SCALE/m_agc_ave) > 0)
		{
			const auto scale = SCALE/m_agc_ave;
			return std::complex<T>( v.real()*scale, v.imag()* scale );
		}
		else
		{
			return v;
		}
	}
private:
	T m_agc_ave {};
};

/* ------------------------------------------------------------------------- */
}}}



#endif /* AGC_CONTROL_HPP_ */
