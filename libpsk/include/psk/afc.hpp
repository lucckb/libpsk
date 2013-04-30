/*
 * afc.hpp
 *
 *  Created on: 29-04-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef AFC_CONTROL_HPP_
#define AFC_CONTROL_HPP_
/* ------------------------------------------------------------------------- */
#include <complex>
#include <utility>

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
template<int SCALE, typename T>
class afc
{
public:
	afc( double phz_inc, double afc_limit )
		: m_afc_max(  phz_inc + afc_limit ),
		  m_afc_min( phz_inc - afc_limit ),
		  m_afc_limit( afc_limit )
	{}
	//Handle sample timer
	void handle_sample_timer( double phz_inc )
	{
		if(	m_afc_timer )
		{
			if(--m_afc_timer <= 0)
			{
				m_afc_timer = 0;
				m_afc_capture_on = false;
				// calculate new limits around latest receive frequency
				m_afc_max =  phz_inc + m_afc_limit;
				m_afc_min =  phz_inc  - m_afc_limit;
				if(m_afc_min<=0.0)
					m_afc_min = 0.0;
			}
			else
			{
				m_afc_capture_on = true;
			}
		}
	}
	void operator()( std::complex<T> sample, T angle_error )
	{
		if(!m_afc_on)
		{
			m_fferr_ave = 0.0;
			m_freq_error = 0.0;
			return;
		}
	}
private:
	//FAST AFC mode
	void calc_ffreq_error( double ferror )
	{
		constexpr auto FP_GN = 0.008;
		constexpr auto FI_GN = 3.0E-5;
		constexpr auto FP_CGN = 0.002;
		constexpr auto FI_CGN = 1.50E-5;
		constexpr auto FWIDE_GN = 1.0/.02;		//gain to make error in Hz
		constexpr auto FWLP_K = 300.0;

		double ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
		ferror /= double(SCALE*SCALE);
		m_z2 = m_z1;
		m_z1 = IQ;
		// error at this point is abt .02 per Hz error
		if( ferror > .30 )		//clamp range
			ferror = .30;
		if( ferror < -.30 )
			ferror = -.30;
		m_fferr_ave = (1.0-1.0/FWLP_K)*m_fferr_ave + ((1.0*FWIDE_GN)/FWLP_K)*ferror;
		ferror=m_fferr_ave;
		if( (ferror > 6.0) || (ferror < -6.0 ) )
		{
			m_nco_phzinc = m_nco_phzinc + (ferror*FI_CGN);
			m_freq_error = ferror*FP_CGN;
		}
		else
		{
			if( (m_fferr_ave*m_fperr_ave)>0.0)	//make sure both errors agree
				ferror = m_fperr_ave;
			else
				ferror = 0.0;
			if( (ferror > 0.3) || (ferror < -0.3 ) )
				m_nco_phzinc = m_nco_phzinc + (ferror*FI_GN);
			m_freq_error = ferror*FP_GN;
		}
	}
	void calc_freq_error( std::complex<T> IQ,  T angle_error )
	{
		constexpr auto P_GN =  0.001;			//AFC constants
		constexpr auto I_GN = 1.5E-6;
		constexpr auto P_CGN = 0.0004;
		constexpr auto I_CGN = 3.0E-6;
		constexpr auto WIDE_GN = 1.0/.02;			//gain to make error in Hz
		constexpr auto WLP_K = 200.0;
		double ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
		ferror /= double(SCALE*SCALE);
		m_z2 = m_z1;
		m_z1 = IQ;
		// error at this point is abt .02 per Hz error
		if( ferror > .30 )		//clamp range
			ferror = .30;
		if( ferror < -.30 )
			ferror = -.30;
		m_fferr_ave = (1.0-1.0/WLP_K)*m_fferr_ave + ((1.0*WIDE_GN)/WLP_K)*ferror;
		if( m_afc_capture_on)
		{
			ferror=m_fferr_ave;
			if( (ferror > 0.3) || (ferror < -0.3 ) )
				m_nco_phzinc = m_nco_phzinc + (ferror*I_CGN);
			m_freq_error = ferror*P_CGN;
		}
		else
		{
			if( (m_fferr_ave*m_fperr_ave)>0.0)	//make sure both errors agree
				ferror = m_fperr_ave;
			else
				ferror = 0.0;
			if( (ferror > 0.3) || (ferror < -0.3 ) )
				m_nco_phzinc = m_nco_phzinc + (ferror*I_GN);
			m_freq_error = ferror*P_GN;
		}
	}
private:
	int m_afc_timer {};
	bool m_afc_capture_on {};
	double m_afc_max;
	double m_afc_min;
	double m_afc_limit;
	bool m_afc_on {};
	double m_fferr_ave {};
	std::complex<long> m_z1 {};
	std::complex<long> m_z2 {};
	double m_freq_error {};
};

/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* AFC_CONTROL_HPP_ */
/* ------------------------------------------------------------------------- */
