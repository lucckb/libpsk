/*
 * diff_angle_calc.hpp
 *
 *  Created on: 03-05-2013
 *      Author: lucck
 */

#ifndef LIBPSK_DIFF_ANGLE_CALC_HPP_
#define LIBPSK_DIFF_ANGLE_CALC_HPP_
/* ------------------------------------------------------------------------- */
#include <complex>
#include <foundation/dsp/dsp_basic_ops.hpp>
#include <foundation/dsp/sqrt_int.hpp>
#include <foundation/dsp/atan2_int.hpp>

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
class diff_angle_calc
{
	//Make object noncopyable
	diff_angle_calc(const diff_angle_calc&) = delete;
	diff_angle_calc& operator=(const diff_angle_calc&) = delete;
public:
	//SCALE for the angle
	static constexpr auto SCALE = 1<<15;
	//Default constructor
	diff_angle_calc()
	{
	}
	//Reset difference angle decoder
	void reset()
	{
		m_iq_phase_array.fill(1);
		m_iq_phz_index = 0;
		m_I0 = 0;
		m_I1 = 0;
		m_Q0 = 0;
		m_Q1 = 0;
	}
	//Calculate difference angle
	int operator()( std::complex<int> newsamp, int agc_value, bool is_qpsk_lsb )
	{
		static constexpr auto PHZ_SCALE = 1<<9;
		constexpr int C_PI_S = 4.0 * std::atan(1.0) * SCALE;
		m_I1 = m_I0;		//form the multi delayed symbol samples
		m_Q1 = m_Q0;
		//m_I0 = newsamp.real();
		//m_Q0 = newsamp.imag();
		m_I0 = dsp::cpu::saturated_cast<short>(newsamp.real());
		m_Q0 = dsp::cpu::saturated_cast<short>(newsamp.imag());
		// calculate difference angle for QPSK, BPSK, and IQPSK decoding
		//create vector whose angle is the difference angle by multiplying the
		// current sample by the complex conjugate of the previous sample.
		//swap I and Q axis to keep away from  the +/-Pi discontinuity and
		//  add Pi to make make range from 0 to 2Pi.
		// 180 deg phase changes center at Pi/4
		// 0 deg phase changes center at 3Pi/2
		// +90 deg phase changes center at 2Pi or 0
		// -90 deg phase changes center at Pi
		//  if using lower sideband must flip sign of difference angle.
		//
		// first calculate normalized vectors for vector display
		auto vect = std::complex<int>( (m_I1*m_Q0)/SCALE - (m_I0*m_Q1)/SCALE, (m_I1*m_I0)/SCALE + (m_Q1*m_Q0)/SCALE );
		auto energy = dsp::integer::sqrt( unsigned(vect.real()*vect.real()) + unsigned(vect.imag()*vect.imag()));
		//TODO: fixme this
		if( agc_value > 10 && energy>0)
		{
			m_iq_phase_array[m_iq_phz_index++] = ((vect.real())/energy)/PHZ_SCALE;
			m_iq_phase_array[m_iq_phz_index++] = ((vect.imag())/energy)/PHZ_SCALE;
		}
		else
		{
			m_iq_phase_array[m_iq_phz_index++] = 2;
			m_iq_phase_array[m_iq_phz_index++] = 2;
		}
		m_iq_phz_index &= 0x000F;		//mod 16 index
		int angle;
		if(is_qpsk_lsb)
			angle = (C_PI_S) + dsp::integer::atan2<int,SCALE>( vect.imag(), -vect.real()); //QPSK lower sideband;
		else
			angle = (C_PI_S) + dsp::integer::atan2<int,SCALE>( vect.imag(), vect.real());  //QPSK upper sideband or BPSK
		return angle;
	}
	//Get last two energies
	int get_energy() const
	{
		return m_I1 * m_I0 +  m_Q1 * m_Q0;
	}
	//Get phase array
	const std::array<int , 16>& get_iq_phase_array() const
	{
		return m_iq_phase_array;
	}
private:
	int m_I0 {};		// 4 stage I/Q delay line variables
	int m_I1 {};
	int m_Q0 {};
	int m_Q1 {};
	std::array<int , 16> m_iq_phase_array;
	int m_iq_phz_index {};
};


}}}
/* ------------------------------------------------------------------------- */
#endif /* DIFF_ANGLE_CALC_HPP_ */
/* ------------------------------------------------------------------------- */
