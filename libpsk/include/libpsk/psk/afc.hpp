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
#include <cstdlib>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
/*Automatic frequency control class */
class afc
{
private:
	static auto constexpr  AFC_TIMELIMIT =  10;
	static auto constexpr  AFC_FTIMELIMIT = 2;
	static auto constexpr ERR_SCALE = 1<<15;
	static constexpr int PI2I = 1<<15;

public:
	afc( int phz_inc, int afc_limit )
		: m_afc_max(  phz_inc + afc_limit ),
		  m_afc_min( phz_inc - afc_limit ),
		  m_afc_limit( afc_limit )
	{
	}
	//Set the frequency
	void reset( int phz_inc )
	{
		m_fferr_ave = 0;
		if(m_fast_afc_mode) m_afc_timer = AFC_FTIMELIMIT;
		else m_afc_timer = AFC_TIMELIMIT;
		// calculate new limits around new receive frequency
		m_afc_max =  phz_inc + m_afc_limit;
		m_afc_min =  phz_inc - m_afc_limit;
		if(m_afc_min<=0) m_afc_min = 0;
		m_fperr_ave = 0;
	}
	//Set AFC limit
	void set_afc_limit( int limit, int freq, int phz_inc )
	{
		m_hz_limit = limit;
		//No frequency control
		if( limit == 0 ) m_fperr_ave = 0;
		if( limit==0 ) m_afc_on = false;
		else m_afc_on = true;
		if(limit==3000) m_fast_afc_mode = true;
		else m_fast_afc_mode = false;
		// calculate new limits around current receive frequency
		m_afc_limit =(limit*PI2I)/freq;
		m_afc_max =  phz_inc + m_freq_error + m_afc_limit;
		m_afc_min =  phz_inc + m_freq_error - m_afc_limit;
		if(m_afc_min<=0) m_afc_min = 0;
	}
	int get_afc_limit() const {
		return m_hz_limit;
	}
	//Handle sample timer
	void handle_sample_timer( int phz_inc )
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
				if(m_afc_min<=0)
					m_afc_min = 0;
			}
			else
			{
				m_afc_capture_on = true;
			}
		}
	}
	//TODO: Remove double when next stage will use integers
	//Phase error update
	void update_angle_error( int aerr )
	{
		constexpr auto PHZDERIVED_GN =  1.0/.2;		//gain to make error in Hz
		//narrow phase derived afc constans
		static constexpr auto NLP_K = 100.0;
		static constexpr auto FNLP_K = 30.0;
		constexpr auto ASCALE = 1<<15;
		static constexpr int F_K1  = (1.0-1.0/FNLP_K) * double(ASCALE) + 0.5;
		static constexpr int F_K2 =  ((1.0*PHZDERIVED_GN)/FNLP_K) * double(ASCALE) + 0.5;
		static constexpr int S_K1 = (1.0-1.0/NLP_K) * double(ASCALE) + 0.5;
		static constexpr int S_K2 = ((1.0*PHZDERIVED_GN)/NLP_K) * double(ASCALE) + 0.5;
		if( is_enabled() )
		{
			if(is_fast())
			{
				m_fperr_ave = F_K1*(m_fperr_ave/ASCALE) +  F_K2*aerr;
			}
			else
			{
				m_fperr_ave = S_K1*(m_fperr_ave/ASCALE) + S_K2*aerr;
			}
		}
		else
		{
			m_fperr_ave = 0;
		}
	}

	/** Return PHZINC and freq error */
	int operator()( std::complex<int> IQ, int phz_inc )
	{
		if(!m_afc_on)
		{
			m_fferr_ave = 0;
			m_freq_error = 0;
			return phz_inc;
		}
		int ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
		m_z2 = m_z1;
		m_z1 = IQ;
		// Calculate frequency error
		if(m_fast_afc_mode)
			phz_inc = calc_ffreq_error(ferror/ERR_SCALE, phz_inc );
		else
			phz_inc = calc_freq_error(ferror/ERR_SCALE, phz_inc );
		//clamp frequency within range
		if( (phz_inc+m_freq_error) > m_afc_max )
		{
			phz_inc = m_afc_max;
			m_freq_error = 0;
		}
		else if( (phz_inc+m_freq_error) < m_afc_min )
		{
			phz_inc = m_afc_min;
			m_freq_error = 0;
		}
		return phz_inc;
	}
	//Get frequency error
	int get_freq_error() const
	{
		return m_freq_error;
	}
private:
	bool is_fast() const
	{
		return m_fast_afc_mode;
	}
	bool is_enabled() const
	{
		return m_afc_on;
	}
	//FAST AFC mode
	int calc_ffreq_error( int ferror, int phz_inc  )
	{

		constexpr auto FWIDE_GN = 1.0/.02;		//gain to make error in Hz
		constexpr auto FWLP_K = 300.0;
		constexpr int K1 = (1.0-1.0/FWLP_K) * double(ERR_SCALE) + 0.5;
		constexpr int K2 = ((1.0*FWIDE_GN)/FWLP_K) * double(ERR_SCALE) + 0.5;


		constexpr auto FP_GN = 0.008;
		constexpr auto FI_GN = 3.0E-5;
		constexpr auto FP_CGN = 0.002;
		constexpr auto FI_CGN = 1.50E-5;

		constexpr int I_FI_CGN = 1.0 /FI_CGN + 0.5;
		constexpr int I_FP_CGN = 1.0 / FP_CGN + 0.5;
		constexpr int I_FI_GN = 1.0 / FI_GN + 0.5;
		constexpr int I_FP_GN = 1.0 / FP_GN + 0.5;

		constexpr int FERR_LIMIT = .30 * double(ERR_SCALE) + 0.5;
		constexpr int FFERR_LIMIT = 6.0 * double(ERR_SCALE) + 0.5;

		constexpr auto SCALE = ERR_SCALE;

		// error at this point is abt .02 per Hz error
		if( ferror > FERR_LIMIT ) ferror = FERR_LIMIT;
		if( ferror < -FERR_LIMIT ) ferror = -FERR_LIMIT;

		m_fferr_ave = (K1*(m_fferr_ave/(SCALE*2)) + K2*ferror);
		ferror=m_fferr_ave;

		if( (ferror > FFERR_LIMIT/SCALE) || (ferror < -FFERR_LIMIT/SCALE) )
		{
			{
				const std::div_t d = std::div( phz_inc*SCALE + ferror/I_FI_CGN, SCALE );
				m_phz_inc_fract += d.rem;
				const int fdiv = m_phz_inc_fract/SCALE;
				phz_inc = d.quot + fdiv;
				if( fdiv ) m_phz_inc_fract %= SCALE;
			}
			{
				const std::div_t d = std::div( ferror/I_FP_CGN, SCALE );
				m_freq_error_fract += d.rem;
				const int fdiv = m_freq_error_fract/SCALE;
				m_freq_error = d.quot + fdiv;
				if( fdiv ) m_freq_error_fract %= SCALE;
			}
		}
		else
		{
			//make sure both errors agree
			if( (m_fferr_ave>0&&m_fperr_ave>0) || (m_fferr_ave<0&&m_fperr_ave<0) )
				ferror = m_fperr_ave;
			else
				ferror = 0;
			if( (ferror/SCALE > FERR_LIMIT) || (ferror/SCALE < -FERR_LIMIT ) )
			{
				const std::div_t d = std::div( phz_inc*SCALE + ferror/I_FI_GN, SCALE );
				m_phz_inc_fract += d.rem;
				const int fdiv = m_phz_inc_fract/SCALE;
				phz_inc = d.quot + fdiv;
				if( fdiv ) m_phz_inc_fract %= SCALE;
			}
			//block
			{
				const std::div_t d = std::div( ferror/I_FP_GN, SCALE );
				m_freq_error_fract += d.rem;
				const int fdiv = m_freq_error_fract/SCALE;
				m_freq_error = d.quot + fdiv;
				if( fdiv ) m_freq_error_fract %= SCALE;
			}
		}
		return phz_inc;
	}
	//Normal AFC mode
	int calc_freq_error( int ferror, int phz_inc )
	{

		constexpr auto WIDE_GN = 1.0/.02;			//gain to make error in Hz
		constexpr auto WLP_K = 200.0;

		constexpr int K1 = (1.0-1.0/WLP_K) * double(ERR_SCALE) + 0.5;
		constexpr int K2 = ((1.0*WIDE_GN)/WLP_K) * double(ERR_SCALE) + 0.5;
		constexpr int FERR_LIMIT = .30 * double(ERR_SCALE) + 0.5;


		constexpr auto I_CGN = 3.0E-6;
		constexpr auto P_GN =  0.001;			//AFC constants
		constexpr auto I_GN = 1.5E-6;
		constexpr auto P_CGN = 0.0004;

		constexpr int I_I_CGN = 1.0 / I_CGN + 0.5;
		constexpr int I_P_CGN = 1.0 / P_CGN + 0.5;
		constexpr int I_I_GN =  1.0 / I_GN  + 0.5;
		constexpr int I_P_GN = 1.0 / P_GN + 0.5;

		// error at this point is abt .02 per Hz error
		if( ferror > FERR_LIMIT ) ferror = FERR_LIMIT;
		if( ferror < -FERR_LIMIT ) ferror = -FERR_LIMIT;

		constexpr auto SCALE = ERR_SCALE;

		m_fferr_ave = (K1*(m_fferr_ave/(SCALE*2)) + K2*ferror);

		if( m_afc_capture_on )
		{
			ferror=m_fferr_ave;
			if( ((ferror/SCALE) > FERR_LIMIT) || ((ferror/SCALE) < FERR_LIMIT ) )
			{
				const std::div_t d = std::div( (phz_inc*SCALE + (ferror/I_I_CGN)), SCALE );
				m_phz_inc_fract += d.rem;
				const int fdiv = m_phz_inc_fract/SCALE;
				phz_inc = d.quot + fdiv;
				if( fdiv ) m_phz_inc_fract %= SCALE;
			}
			const std::div_t d =  std::div( (ferror/I_P_CGN), SCALE );
			m_freq_error_fract += d.rem;
			const int fdiv = m_freq_error_fract/SCALE;
			m_freq_error = d.quot + fdiv;
			if( fdiv ) m_freq_error_fract %= SCALE;
		}
		else
		{
			if( (m_fferr_ave>0&&m_fperr_ave>0) || (m_fferr_ave<0&&m_fperr_ave<0) )
				ferror = m_fperr_ave;
			else
				ferror = 0;
			if( (ferror/SCALE > FERR_LIMIT) || (ferror/SCALE < FERR_LIMIT ) )
			{
				const std::div_t d = std::div( phz_inc*SCALE+(ferror/I_I_GN), SCALE );
				m_phz_inc_fract += d.rem;
				const int fdiv = m_phz_inc_fract/SCALE;
				phz_inc = d.quot + fdiv;
				if( fdiv ) m_phz_inc_fract %= SCALE;
			}
			const std::div_t d = std::div( ferror/I_P_GN , SCALE );
			m_freq_error_fract += d.rem;
			const int fdiv = m_freq_error_fract/SCALE;
			m_freq_error = d.quot + fdiv;
			if( fdiv ) m_freq_error_fract %= SCALE;
		}
		return phz_inc;
	}
private:
	//Make object noncopyable
	afc(const afc&) = delete;
	afc& operator=(const afc&) = delete;
private:
	int m_afc_timer {};
	bool m_afc_capture_on {};
	int m_afc_max;
	int m_afc_min;
	int m_afc_limit;
	bool m_afc_on {};
	int m_fferr_ave {};
	std::complex<int> m_z1 {};
	std::complex<int> m_z2 {};
	int m_freq_error {};
	bool m_fast_afc_mode {};
	//Fractial part of values
	int m_phz_inc_fract {};
	int m_freq_error_fract {};
	int m_fperr_ave {};
	int m_hz_limit {};
};

/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* AFC_CONTROL_HPP_ */
/* ------------------------------------------------------------------------- */
