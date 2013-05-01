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
#include <stdio.h>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
template<long SCALE, typename T>
class afc
{
private:
	static auto constexpr  AFC_TIMELIMIT =  10;
	static auto constexpr  AFC_FTIMELIMIT = 2;
public:
	afc( double phz_inc, double afc_limit )
		: m_afc_max(  phz_inc + afc_limit ),
		  m_afc_min( phz_inc - afc_limit ),
		  m_afc_limit( afc_limit )
	{
		printf("CONSTRUCT m_afc_max %f m_afc_min %f, m_afc_limit %f m_afc_on %i m_afc_fast %i\n", m_afc_max, m_afc_min, m_afc_limit, m_afc_on, m_fast_afc_mode );
	}
	//Set the frequency
	void reset( double phz_inc )
	{
		m_fferr_ave = 0;
		if(m_fast_afc_mode) m_afc_timer = AFC_FTIMELIMIT;
		else m_afc_timer = AFC_TIMELIMIT;
		// calculate new limits around new receive frequency
		m_afc_max =  phz_inc + m_afc_limit;
		m_afc_min =  phz_inc - m_afc_limit;
		if(m_afc_min<=0.0) m_afc_min = 0.0;
		printf("RESET m_afc_max %f m_afc_min %f, m_afc_limit %f m_afc_on %i m_afc_fast %i\n", m_afc_max, m_afc_min, m_afc_limit, m_afc_on, m_fast_afc_mode );
	}
	//Set AFC limit
	void set_afc_limit( double limit, double freq,double phz_inc )
	{
		constexpr auto PI2 = 8.0 * std::atan(1.0);
		if(limit==0) m_afc_on = false;
		else m_afc_on = true;
		if(limit==3000) m_fast_afc_mode = true;
		else m_fast_afc_mode = false;
		// calculate new limits around current receive frequency
		m_afc_limit = (double)limit*PI2/freq;
		m_afc_max =  phz_inc + m_freq_error + m_afc_limit;
		m_afc_min =  phz_inc + m_freq_error - m_afc_limit;
		if(m_afc_min<=0.0) m_afc_min = 0.0;
		printf("LIMIT m_afc_max %f m_afc_min %f, m_afc_limit %f m_afc_on %i m_afc_fast %i\n", m_afc_max, m_afc_min, m_afc_limit, m_afc_on, m_fast_afc_mode );
	}
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
	/** Return PHZINC and freq error */
	double operator()( std::complex<T> IQ, double angle_error, double phz_inc )
	{
		if(!m_afc_on)
		{
			m_fferr_ave = 0.0;
			m_freq_error = 0.0;
			return phz_inc;
		}
		double ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
		ferror /= double(SCALE*SCALE);
		//printf("FERRORS: %f ANGLE %f ", ferror, angle_error);
		m_z2 = m_z1;
		m_z1 = IQ;
		// Calculate frequency error
		if(m_fast_afc_mode)
			phz_inc = calc_ffreq_error(ferror, angle_error, phz_inc );
		else
			phz_inc = calc_freq_error(ferror, angle_error, phz_inc );
		//clamp frequency within range
		if( (phz_inc+m_freq_error) > m_afc_max )
		{
			phz_inc = m_afc_max;
			m_freq_error = 0.0;
		}
		else if( (phz_inc+m_freq_error) < m_afc_min )
		{
			phz_inc = m_afc_min;
			m_freq_error = 0.0;
		}
		if( 0 && m_freq_error != 0)
			printf("MAIN: phz_inc: %f  m_freq_error: %f fferave %f\n", phz_inc, m_freq_error, m_fferr_ave);
		return phz_inc;
	}
	//Get frequency error
	double get_freq_error() const
	{
		return m_freq_error;
	}
	bool is_fast() const
	{
		return m_fast_afc_mode;
	}
	bool is_enabled() const
	{
		return m_afc_on;
	}
private:
	//FAST AFC mode
	double calc_ffreq_error( double ferror, double angle_error, double phz_inc  )
	{
		constexpr auto FP_GN = 0.008;
		constexpr auto FI_GN = 3.0E-5;
		constexpr auto FP_CGN = 0.002;
		constexpr auto FI_CGN = 1.50E-5;
		constexpr auto FWIDE_GN = 1.0/.02;		//gain to make error in Hz
		constexpr auto FWLP_K = 300.0;


		// error at this point is abt .02 per Hz error
		if( ferror > .30 )		//clamp range
			ferror = .30;
		if( ferror < -.30 )
			ferror = -.30;
		m_fferr_ave = (1.0-1.0/FWLP_K)*m_fferr_ave + ((1.0*FWIDE_GN)/FWLP_K)*ferror;
		ferror=m_fferr_ave;
		if( (ferror > 6.0) || (ferror < -6.0 ) )
		{
			phz_inc = phz_inc + (ferror*FI_CGN);
			m_freq_error = ferror*FP_CGN;
		}
		else
		{
			if( (m_fferr_ave*angle_error)>0.0)	//make sure both errors agree
				ferror = angle_error;
			else
				ferror = 0.0;
			if( (ferror > 0.3) || (ferror < -0.3 ) )
				phz_inc = phz_inc + (ferror*FI_GN);
			m_freq_error = ferror*FP_GN;
		}
		return phz_inc;
	}
	//Normal AFC mode
	double calc_freq_error( double ferror, double angle_error, double phz_inc  )
	{
		constexpr auto P_GN =  0.001;			//AFC constants
		constexpr auto I_GN = 1.5E-6;
		constexpr auto P_CGN = 0.0004;
		constexpr auto I_CGN = 3.0E-6;
		constexpr auto WIDE_GN = 1.0/.02;			//gain to make error in Hz
		constexpr auto WLP_K = 200.0;

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
				phz_inc = phz_inc + (ferror*I_CGN);
			m_freq_error = ferror*P_CGN;
		}
		else
		{
			if( (m_fferr_ave*angle_error)>0.0)	//make sure both errors agree
				ferror = angle_error;
			else
				ferror = 0.0;
			if( (ferror > 0.3) || (ferror < -0.3 ) )
				phz_inc = phz_inc + (ferror*I_GN);
			m_freq_error = ferror*P_GN;
		}
		return phz_inc;
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
	bool m_fast_afc_mode {};
};

/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* AFC_CONTROL_HPP_ */
/* ------------------------------------------------------------------------- */
