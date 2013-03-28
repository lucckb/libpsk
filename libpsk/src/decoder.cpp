/* ------------------------------------------------------------------------- */
/*
 * decoder.cpp
 *
 *  Created on: 24-03-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "psk/decoder.hpp"
#include "psk/varicode.hpp"
#include <complex>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iomanip>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Unnamed anon namespace
namespace
{
	constexpr auto PI2 = 8.0 * std::atan(1.0);
	constexpr auto PHZ_180_QMIN	= PI2/8.0;		// Pi/4
	constexpr auto PHZ_180_QMAX	= 3.0*PI2/8.0;	// 3Pi/4
	constexpr auto PHZ_0_QMIN	= 5.0*PI2/8.0;		// 5Pi/4
	constexpr auto PHZ_0_QMAX	= 7.0*PI2/8.0;		// 7Pi/4
	constexpr auto PHZ_180_BMIN = 0.0;			// 0
	constexpr auto PHZ_180_BMAX = PI2/2.0;		// Pi
	constexpr auto PHZ_0_BMIN	= PI2/2.0;			// Pi
	constexpr auto PHZ_0_BMAX	= PI2;
	// phase wraparound correction tables for viterbi decoder
	constexpr double ANGLE_TBL1[] = { 3.0*PI2/4.0, 0.0, PI2/4.0, PI2/2.0 };
	constexpr double ANGLE_TBL2[] = { 3.0*PI2/4.0, PI2, PI2/4.0, PI2/2.0 };
	constexpr auto NLP_K = 100.0;		//narrow phase derived afc constans
	constexpr auto FNLP_K = 30.0;
	constexpr auto Ts = .032+.00000;			// Ts == symbol period
	// Lookup table to get symbol from non-inverted data stream
	static constexpr unsigned char ConvolutionCodeTable[] =
	{
		2, 1, 3, 0, 3, 0, 2, 1,
		0, 3, 1, 2, 1, 2, 0, 3,
		1, 2, 0, 3, 0, 3, 1, 2,
		3, 0, 2, 1, 2, 1, 3, 0
	};
	constexpr double Dec4LPCoef[ ] = {
	 -0.00021203644,
	 -0.00070252426,
	  -0.0016680526,
	  -0.0031934799,
	  -0.0051899752,
	  -0.0072862086,
	  -0.0087714235,
	  -0.0086272102,
	  -0.0056735648,
	   0.0011784719,
	     0.01261353,
	    0.028615709,
	    0.048280707,
	    0.069812051,
	    0.090735013,
	     0.10830381,
	     0.12001897,
	     0.12413413,
	     0.12001897,
	     0.10830381,
	    0.090735013,
	    0.069812051,
	    0.048280707,
	    0.028615709,
	     0.01261353,
	   0.0011784719,
	  -0.0056735648,
	  -0.0086272102,
	  -0.0087714235,
	  -0.0072862086,
	  -0.0051899752,
	  -0.0031934799,
	  -0.0016680526,
	 -0.00070252426,
	 -0.00021203644,
	//
	 -0.00021203644,
	 -0.00070252426,
	  -0.0016680526,
	  -0.0031934799,
	  -0.0051899752,
	  -0.0072862086,
	  -0.0087714235,
	  -0.0086272102,
	  -0.0056735648,
	   0.0011784719,
	     0.01261353,
	    0.028615709,
	    0.048280707,
	    0.069812051,
	    0.090735013,
	     0.10830381,
	     0.12001897,
	     0.12413413,
	     0.12001897,
	     0.10830381,
	    0.090735013,
	    0.069812051,
	    0.048280707,
	    0.028615709,
	     0.01261353,
	   0.0011784719,
	  -0.0056735648,
	  -0.0086272102,
	  -0.0087714235,
	  -0.0072862086,
	  -0.0051899752,
	  -0.0031934799,
	  -0.0016680526,
	 -0.00070252426,
	 -0.00021203644
	};

	constexpr double FreqFirCoef[] = { // 31.25 Hz bw LP filter for frequency error
	 -0.00088229088,
	  -0.0011374389,
	  -0.0016412868,
	  -0.0020520989,
	  -0.0022175618,
	  -0.0019791161,
	  -0.0012057952,
	  0.00016527251,
	   0.0020943472,
	   0.0044122668,
	   0.0068189371,
	   0.0089045933,
	    0.010203678,
	    0.010266316,
	   0.0087507451,
	   0.0055136683,
	  0.00068616242,
	   -0.005288313,
	   -0.011653249,
	   -0.017399749,
	   -0.021376642,
	   -0.022435377,
	   -0.019606873,
	   -0.012263665,
	 -0.00026078671,
	    0.015986088,
	    0.035491391,
	    0.056773305,
	    0.078002392,
	    0.097207364,
	     0.11251413,
	     0.12237776,
	     0.12580366,
	     0.12237776,
	     0.11251413,
	    0.097207364,
	    0.078002392,
	    0.056773305,
	    0.035491391,
	    0.015986088,
	 -0.00026078671,
	   -0.012263665,
	   -0.019606873,
	   -0.022435377,
	   -0.021376642,
	   -0.017399749,
	   -0.011653249,
	   -0.005288313,
	  0.00068616242,
	   0.0055136683,
	   0.0087507451,
	    0.010266316,
	    0.010203678,
	   0.0089045933,
	   0.0068189371,
	   0.0044122668,
	   0.0020943472,
	  0.00016527251,
	  -0.0012057952,
	  -0.0019791161,
	  -0.0022175618,
	  -0.0020520989,
	  -0.0016412868,
	  -0.0011374389,
	 -0.00088229088,
	//
	 -0.00088229088,
	  -0.0011374389,
	  -0.0016412868,
	  -0.0020520989,
	  -0.0022175618,
	  -0.0019791161,
	  -0.0012057952,
	  0.00016527251,
	   0.0020943472,
	   0.0044122668,
	   0.0068189371,
	   0.0089045933,
	    0.010203678,
	    0.010266316,
	   0.0087507451,
	   0.0055136683,
	  0.00068616242,
	   -0.005288313,
	   -0.011653249,
	   -0.017399749,
	   -0.021376642,
	   -0.022435377,
	   -0.019606873,
	   -0.012263665,
	 -0.00026078671,
	    0.015986088,
	    0.035491391,
	    0.056773305,
	    0.078002392,
	    0.097207364,
	     0.11251413,
	     0.12237776,
	     0.12580366,
	     0.12237776,
	     0.11251413,
	    0.097207364,
	    0.078002392,
	    0.056773305,
	    0.035491391,
	    0.015986088,
	 -0.00026078671,
	   -0.012263665,
	   -0.019606873,
	   -0.022435377,
	   -0.021376642,
	   -0.017399749,
	   -0.011653249,
	   -0.005288313,
	  0.00068616242,
	   0.0055136683,
	   0.0087507451,
	    0.010266316,
	    0.010203678,
	   0.0089045933,
	   0.0068189371,
	   0.0044122668,
	   0.0020943472,
	  0.00016527251,
	  -0.0012057952,
	  -0.0019791161,
	  -0.0022175618,
	  -0.0020520989,
	  -0.0016412868,
	  -0.0011374389,
	 -0.00088229088
	};
	constexpr double BitFirCoef[] = { // 16 Hz bw LP filter for data recovery
	 4.3453566e-005,
	 -0.00049122414,
	 -0.00078771292,
	  -0.0013507826,
	  -0.0021287814,
	   -0.003133466,
	   -0.004366817,
	  -0.0058112187,
	  -0.0074249976,
	  -0.0091398882,
	   -0.010860157,
	   -0.012464086,
	   -0.013807772,
	   -0.014731191,
	   -0.015067057,
	   -0.014650894,
	   -0.013333425,
	    -0.01099166,
	  -0.0075431246,
	  -0.0029527849,
	   0.0027546292,
	   0.0094932775,
	    0.017113308,
	    0.025403511,
	    0.034099681,
	    0.042895839,
	    0.051458575,
	    0.059444853,
	    0.066521003,
	    0.072381617,
	    0.076767694,
	    0.079481619,
	    0.080420311,
	    0.079481619,
	    0.076767694,
	    0.072381617,
	    0.066521003,
	    0.059444853,
	    0.051458575,
	    0.042895839,
	    0.034099681,
	    0.025403511,
	    0.017113308,
	   0.0094932775,
	   0.0027546292,
	  -0.0029527849,
	  -0.0075431246,
	    -0.01099166,
	   -0.013333425,
	   -0.014650894,
	   -0.015067057,
	   -0.014731191,
	   -0.013807772,
	   -0.012464086,
	   -0.010860157,
	  -0.0091398882,
	  -0.0074249976,
	  -0.0058112187,
	   -0.004366817,
	   -0.003133466,
	  -0.0021287814,
	  -0.0013507826,
	 -0.00078771292,
	 -0.00049122414,
	 4.3453566e-005,
	//
	 4.3453566e-005,
	 -0.00049122414,
	 -0.00078771292,
	  -0.0013507826,
	  -0.0021287814,
	   -0.003133466,
	   -0.004366817,
	  -0.0058112187,
	  -0.0074249976,
	  -0.0091398882,
	   -0.010860157,
	   -0.012464086,
	   -0.013807772,
	   -0.014731191,
	   -0.015067057,
	   -0.014650894,
	   -0.013333425,
	    -0.01099166,
	  -0.0075431246,
	  -0.0029527849,
	   0.0027546292,
	   0.0094932775,
	    0.017113308,
	    0.025403511,
	    0.034099681,
	    0.042895839,
	    0.051458575,
	    0.059444853,
	    0.066521003,
	    0.072381617,
	    0.076767694,
	    0.079481619,
	    0.080420311,
	    0.079481619,
	    0.076767694,
	    0.072381617,
	    0.066521003,
	    0.059444853,
	    0.051458575,
	    0.042895839,
	    0.034099681,
	    0.025403511,
	    0.017113308,
	   0.0094932775,
	   0.0027546292,
	  -0.0029527849,
	  -0.0075431246,
	    -0.01099166,
	   -0.013333425,
	   -0.014650894,
	   -0.015067057,
	   -0.014731191,
	   -0.013807772,
	   -0.012464086,
	   -0.010860157,
	  -0.0091398882,
	  -0.0074249976,
	  -0.0058112187,
	   -0.004366817,
	   -0.003133466,
	  -0.0021287814,
	  -0.0013507826,
	 -0.00078771292,
	 -0.00049122414,
	 4.3453566e-005
	};


	constexpr int HALF_TBL[] = {
	    7,	// 0
	   8,	// 1
	   9,	// 2
	   10,	// 3
	   11,	// 4
	   12,	// 5
	   13,	// 6
	   14,	// 7
	    0,	// 8
	    1,	// 9
	    2,	// 10
	    3,	// 11
	    4,	// 12
	    5,	// 13
	    6,	// 14
	    7,	// 15
	};

	auto constexpr  AFC_TIMELIMIT =  10;
	auto constexpr  AFC_FTIMELIMIT = 2;
}
/* ------------------------------------------------------------------------- */
//Construct the decoder object
decoder::decoder( samplerate_type sample_rate, event_callback_type callback ) :
	  m_callback(callback ),
	  m_nco_phzinc( PI2*double(m_rx_frequency)/double(sample_rate) ),
	  m_afc_limit(50.0*PI2/double(sample_rate) ),
	  m_sample_freq( sample_rate ), m_nlp_k( NLP_K )

{
	//Initialy reset the decoder
	reset();
}

/* ------------------------------------------------------------------------- */
//Set squelch tresh
void decoder::set_squelch_tresh( sqelch_value_type thresh, squelch_mode mode )
{
	m_sq_thresh = thresh;
	if( mode == squelch_mode::fast ) m_squelch_speed = 20;
	else if( mode == squelch_mode::slow ) m_squelch_speed = 75;
}
/* ------------------------------------------------------------------------- */
void decoder::set_frequency( int freq )
{
	if( freq != m_rx_frequency)
	{
		m_rx_frequency = freq;
		m_nco_phzinc = PI2*(double)freq/m_sample_freq;
		m_fferr_ave = 0.0;
		m_fperr_ave = 0.0;
		if(m_fast_afc_mode)
			m_afc_timer = AFC_FTIMELIMIT;
		else
			m_afc_timer = AFC_TIMELIMIT;
	// calculate new limits around new receive frequency
		m_afc_max =  m_nco_phzinc + m_afc_limit;
		m_afc_min =  m_nco_phzinc - m_afc_limit;
		if(m_afc_min<=0.0)
			m_afc_min = 0.0;
		m_pcnt = 0;
		m_ncnt = 0;
	}
}
/* ------------------------------------------------------------------------- */
//Set AFC limit
void decoder::set_afc_limit( int limit )
{
	if(limit==0) m_afc_on = false;
	else m_afc_on = true;
	if(limit==3000) m_fast_afc_mode = true;
	else m_fast_afc_mode = false;
	m_afc_limit = (double)limit*PI2/m_sample_freq;
	// calculate new limits around current receive frequency
	m_afc_max =  m_nco_phzinc + m_freq_error + m_afc_limit;
	m_afc_min =  m_nco_phzinc + m_freq_error - m_afc_limit;
	if(m_afc_min<=0.0) m_afc_min = 0.0;
	if(m_fast_afc_mode)  m_nlp_k = FNLP_K;
	else m_nlp_k = NLP_K;
}
/* ------------------------------------------------------------------------- */
//Viterbi decode
bool decoder::viterbi_decode( double newangle )
{
	double pathdist[32];
	double min;
	int bitestimates[32];
	int ones;
	const double* pAngleTbl;
	min = 1.0e100;		// make sure can find a minimum value
	if( newangle >= PI2/2 )		//deal with ambiguity at +/- 2PI
		pAngleTbl = ANGLE_TBL2;	// by using two different tables
	else
		pAngleTbl = ANGLE_TBL1;
	for(int i = 0; i < 32; i++)		// calculate all possible distances
	{							//lsb of 'i' is newest bit estimate
		pathdist[i] = m_survivor_states[i / 2].path_distance +
				std::abs(newangle - pAngleTbl[ ConvolutionCodeTable[i] ]);
		if(pathdist[i] < min)	// keep track of minimum distance
			min = pathdist[i];
		// shift in newest bit estimates
		bitestimates[i] = ((m_survivor_states[i / 2].bit_estimates) << 1) + (i & 1);
	}
	for(int i = 0; i < 16; i++)	//compare path lengths with the same end state
							// and keep only the smallest path in m_SurvivorStates[].
	{
		if(pathdist[i] < pathdist[16 + i])
		{
			m_survivor_states[i].path_distance = pathdist[i] - min;
			m_survivor_states[i].bit_estimates = bitestimates[i];
		}
		else
		{
			m_survivor_states[i].path_distance = pathdist[16 + i] - min;
			m_survivor_states[i].bit_estimates = bitestimates[16 + i];
		}
	}
	ones = 0;
	for(int i = 0; i < 16; i++)		// find if more ones than zeros at bit 20 position
		ones += (m_survivor_states[i].bit_estimates&(1L << 20));
	if( ones == (8L << 20 ) )
		return ( rand() & 0x1000 );	//if a tie then guess
	else
		return(ones > (8L << 20) );	//else return most likely bit value
}
/* ------------------------------------------------------------------------- */
//Reset decoder
void decoder::reset()
{
	for( auto &v : m_que1 )
	{
		v = std::complex<double>();
	}
	for( auto &v : m_que2 )
	{
		v = std::complex<double>();
	}
	for(auto &v : m_que3)
	{
		v = std::complex<double>();
	}
	for( auto &v : m_survivor_states )
	{
		v = survivor_states();
	}
	for( auto &v : m_iq_phase_array )
	{
		v = 1;
	}
	for( auto &v : m_sync_ave )
	{
		v = 0.0;						// initialize the array
		viterbi_decode( 3.0*PI2/4.0 );	// init the Viterbi decoder
	}
	m_fir1_state = DEC4_LPFIR_LENGTH-1;	//initialize FIR states
	m_fir2_state = DEC4_LPFIR_LENGTH-1;
	m_fir3_state = BITFIR_LENGTH-1;
	m_sql_level = 10;
	m_clk_err_counter = 0;
	m_clk_err_timer = 0;
	m_dev_ave = 90.0;
	m_clk_error = 0;
}

/* ------------------------------------------------------------------------- */
void decoder::calc_bit_filter( std::complex<double> samp )
{
	std::complex<double> acc1 {};
	std::complex<double> acc2 {};
	m_que3[m_fir3_state] = samp;
	std::complex<double>* Firptr = m_que3.data();
	const double* Kptr1 = FreqFirCoef + BITFIR_LENGTH - m_fir3_state;	//frequency error filter
	const double* Kptr2 = BitFirCoef + BITFIR_LENGTH - m_fir3_state;	//bit data filter
	for(int j=0; j<	BITFIR_LENGTH;j++)	//do the MACs
	{
		//acc1 += std::complex<double>((Firptr->real())*(*Kptr1),(Firptr->imag())*(*Kptr1++)  );
		//acc2 += std::complex<double>((Firptr->real())*(*Kptr2), (Firptr++->imag())*(*Kptr2++) );
		acc1 += (*Firptr) * (*Kptr1++);
		acc2 += (*Firptr++) * (*Kptr2++);
	}
	if( --m_fir3_state < 0)
		m_fir3_state = BITFIR_LENGTH-1;
	m_freq_signal = acc1;
	m_bit_signal = acc2;
}
/* ------------------------------------------------------------------------- */
void decoder::calc_agc( std::complex<double> samp )
{
	double mag = std::sqrt(samp.real()*samp.real() + samp.imag()*samp.imag() );
	if( mag > m_agc_ave )
		m_agc_ave = (1.0-1.0/200.0)*m_agc_ave + (1.0/200.0)*mag;
	else
		m_agc_ave = (1.0-1.0/500.0)*m_agc_ave + (1.0/500.0)*mag;
	if( m_agc_ave >= 1.0 )	// divide signal by ave if not almost zero
	{
		m_bit_signal =  std::complex<double>( m_bit_signal.real()/ m_agc_ave , m_bit_signal.imag()/ m_agc_ave);
		m_freq_signal = std::complex<double>( m_freq_signal.real()/ m_agc_ave , m_freq_signal.imag()/ m_agc_ave);
		//m_BitSignal.x /= m_AGCave;
		//m_BitSignal.y /= m_AGCave;
		//m_FreqSignal.x /= m_AGCave;
		//m_FreqSignal.y /= m_AGCave;
	}
}
/* ------------------------------------------------------------------------- */
void decoder::calc_freq_error( std::complex<double> IQ )
{
	constexpr auto P_GN =  0.001;			//AFC constants
	constexpr auto I_GN = 1.5E-6;
	constexpr auto P_CGN = 0.0004;
	constexpr auto I_CGN = 3.0E-6;
	constexpr auto WIDE_GN = 1.0/.02;			//gain to make error in Hz
	constexpr auto WLP_K = 200.0;
	if( !m_afc_on )
	{
		m_fferr_ave = 0.0;
		m_fperr_ave = 0.0;
		m_freq_error = 0.0;
		return;
	}
	double ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
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

	//clamp frequency within range
	if( (m_nco_phzinc+m_freq_error) > m_afc_max )
	{
		m_nco_phzinc = m_afc_max;
		m_freq_error = 0.0;
	}
	else if( (m_nco_phzinc+m_freq_error) < m_afc_min )
	{
		m_nco_phzinc = m_afc_min;
		m_freq_error = 0.0;
	}
}
/* ------------------------------------------------------------------------- */
void decoder::calc_ffreq_error( std::complex<double> IQ )
{
	constexpr auto FP_GN = 0.008;
	constexpr auto FI_GN = 3.0E-5;
	constexpr auto FP_CGN = 0.002;
	constexpr auto FI_CGN = 1.50E-5;
	constexpr auto FWIDE_GN = 1.0/.02;		//gain to make error in Hz
	constexpr auto FWLP_K = 300.0;
	if(!m_afc_on)
	{
		m_fferr_ave = 0.0;
		m_fperr_ave = 0.0;
		m_freq_error = 0.0;
		return;
	}
	double ferror = (IQ.real() - m_z2.real()) * m_z1.imag() - (IQ.imag() - m_z2.imag()) * m_z1.real();
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
	//clamp frequency within range
	if( (m_nco_phzinc+m_freq_error) > m_afc_max )
	{
		m_nco_phzinc = m_afc_max;
		m_freq_error = 0.0;
	}
	else if( (m_nco_phzinc+m_freq_error) < m_afc_min )
	{
		m_nco_phzinc = m_afc_min;
		m_freq_error = 0.0;
	}
}
/* ------------------------------------------------------------------------- */
void decoder::decode_symb( std::complex<double> newsamp )
{

	double angle;
	double energy;
	uint8_t ch = 0;
	bool bit;
	bool GotChar = false;
	m_I1 = m_I0;		//form the multi delayed symbol samples
	m_Q1 = m_Q0;
	m_I0 = newsamp.real();
	m_Q0 = newsamp.imag();
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
		auto vect = std::complex<double>( m_I1*m_Q0 - m_I0*m_Q1, m_I1*m_I0 + m_Q1*m_Q0 );
		energy = sqrt(vect.real()*vect.real() + vect.imag()*vect.imag())/1.0E3;
		if( m_agc_ave > 10.0 )
		{
			m_iq_phase_array[m_iq_phz_index++] = long(vect.real()/energy);
			m_iq_phase_array[m_iq_phz_index++] = long(vect.imag()/energy);
		}
		else
		{
			m_iq_phase_array[m_iq_phz_index++] = 2;
			m_iq_phase_array[m_iq_phz_index++] = 2;
		}
		m_iq_phz_index &= 0x000F;		//mod 16 index
		if(m_rx_mode == mode::qpskl)
			angle = (PI2/2) + atan2( vect.imag(), -vect.real()); //QPSK lower sideband;
		else
			angle = (PI2/2) + atan2( vect.imag(), vect.real()); //QPSK upper sideband or BPSK
		calc_quality( angle );
		if(m_rx_mode == mode::bpsk)
		{
			//calc BPSK symbol over 2 chips
			vect.imag(  m_I1 * m_I0 +  m_Q1 * m_Q0 );
			bit = bool(vect.imag() > 0.0);
		}
		else
			bit = viterbi_decode( angle );
		if( (bit==0) && m_last_bit_zero )	//if character delimiter
		{
			if(m_bit_acc != 0 )
			{
				constexpr _internal::varicode v;
				m_bit_acc >>= 2;				//get rid of last zero and one
				m_bit_acc &= 0x07FF;
				ch = v.reverse(m_bit_acc);
				m_bit_acc = 0;
				GotChar = true;
			}
		}
		else
		{
			m_bit_acc <<= 1;
			m_bit_acc |= bit;
			if(bit==0)
				m_last_bit_zero = true;
			else
				m_last_bit_zero = false;
		}
		//FIXME this
		if(GotChar && (ch!=0) && m_sq_open )
		{
			//::PostMessage(m_hWnd, MSG_PSKCHARRDY, ch, m_RxChannel);
			if( m_callback ) m_callback( cb_rxchar, ch, 0 );
		}
		GotChar = false;
}

/* ------------------------------------------------------------------------- */
void decoder::calc_quality( double angle )
{
	constexpr auto ELIMIT = 5;
	constexpr auto PHZDERIVED_GN =  1.0/.2;		//gain to make error in Hz
	double temp;
	double SqTimeK =  double(m_squelch_speed);
	if( (is_qpsk() && ((angle >= PHZ_180_QMIN) && (angle <= PHZ_180_QMAX) ) ) ||
		( !is_qpsk() && ((angle >= PHZ_180_BMIN) && (angle <= PHZ_180_BMAX))) )
	{	//look +/-45 or +/-180 deg. around 180 deg.
		if(m_rx_mode == mode::qpskl )
			temp = PI2/4.0 - angle;
		else
			temp = angle - PI2/4.0;
		m_q_freq_error = temp;
		if( is_qpsk() ) //if QPSK
			temp = 280.0*std::abs(temp);
		else
			temp = 150.0*std::abs(temp);
		if( temp < m_dev_ave)
			m_dev_ave=  (1.0-1.0/SqTimeK)*m_dev_ave + (1.0/SqTimeK)*temp;
		else
			m_dev_ave =  (1.0-1.0/(SqTimeK*2.0))*m_dev_ave + (1.0/(SqTimeK*2.0))*temp;
		if(m_on_count > 20 )		// fast squelch counter
			m_dev_ave = 100.0-75.0;	//set to 75%
		else
			m_on_count++;
		m_off_count = 0;
		if( m_q_freq_error >= 0.0 )
		{
			m_pcnt++;
			m_ncnt = 0;
		}
		else
		{
			m_ncnt++;
			m_pcnt = 0;
		}
		if( (m_pcnt<ELIMIT) && (m_ncnt<ELIMIT) )
			m_q_freq_error = 0.0;
	}
	else
	{
		if( (is_qpsk() && ((angle >= PHZ_0_QMIN) && (angle <= PHZ_0_QMAX) ) ) ||
			(!is_qpsk() && ((angle >= PHZ_0_BMIN) && (angle <= PHZ_0_BMAX) ) ) )

		{		//look +/-45 or +/- 180 deg. around 0 deg.
			if( m_rx_mode== mode::qpskl )
				temp = 3*PI2/4.0 - angle;
			else
				temp = angle - 3*PI2/4.0;
			m_q_freq_error = temp;
			if( is_qpsk() ) //if QPSK
				temp = 280.0*std::abs(temp);
			else
				temp = 150.0*std::abs(temp);
			if( temp < m_dev_ave)
				m_dev_ave =  (1.0-1.0/SqTimeK)*m_dev_ave + (1.0/SqTimeK)*temp;
			else
				m_dev_ave =  (1.0-1.0/(SqTimeK*2.0))*m_dev_ave + (1.0/(SqTimeK*2.0))*temp;
			if(m_off_count > 20 )	// fast squelch counter
				if( m_rx_mode == mode::bpsk  ) //if BPSK
					m_dev_ave = 100.0 - 0.0;		//set to 0%
			else
				m_off_count++;
			m_on_count = 0;
			if( m_q_freq_error >= 0.0 )
			{
				m_pcnt++;
				m_ncnt = 0;
			}
			else
			{
				m_ncnt++;
				m_pcnt = 0;
			}
			if( (m_pcnt<ELIMIT) && (m_ncnt<ELIMIT) )
				m_q_freq_error = 0.0;
		}

	}
	m_imd_valid = (m_on_count >2);

	if( m_agc_ave > 10.0 )
	{
		if( is_qpsk() ) //if QPSK
			m_sql_level = 100 - int(m_dev_ave);
		else
			m_sql_level = 100 - int(m_dev_ave);
		m_sq_open = ( m_sql_level >= m_sq_thresh );
	}
	else
	{
		m_sql_level = 0;
		m_sq_open = false;
	}
	if(is_qpsk())
	{
		if( m_q_freq_error > .6 )//  clamp range to +/- 3 Hz
			m_q_freq_error = .6;
		if( m_q_freq_error < -.6 )
			m_q_freq_error = -.6;
	}
	else
	{
		if( m_q_freq_error> 1.0 )//  clamp range to +/- 5 Hz
			m_q_freq_error = 1.0;
		if( m_q_freq_error < -1.0 )
			m_q_freq_error = -1.0;
	}
	m_fperr_ave = (1.0-1.0/m_nlp_k)*m_fperr_ave +
					( (1.0*PHZDERIVED_GN)/m_nlp_k)*m_q_freq_error;
}
/* ------------------------------------------------------------------------- */
bool decoder::symb_sync(std::complex<double> sample)
{
	bool Trigger=false;
	double max;
	double energy;
	int BitPos = m_bit_pos;
	if(BitPos<16)
	{
		energy = (sample.real()*sample.real()) + (sample.imag()*sample.imag());
		if( energy > 4.0)		//wait for AGC to settle down
			energy = 1.0;
		m_sync_ave[BitPos] = (1.0-1.0/82.0)*m_sync_ave[BitPos] + (1.0/82.0)*energy;
		if( BitPos == m_pk_pos )	// see if at middle of symbol
		{
			Trigger = true;
			m_sync_array[m_pk_pos] = int(900.0*m_sync_ave[m_pk_pos]);
		}
		else
		{
			Trigger = false;
			m_sync_array[BitPos] = int(750.0*m_sync_ave[BitPos]);
		}
		if( BitPos == HALF_TBL[m_new_pk_pos] )	//don't change pk pos until
			m_pk_pos = m_new_pk_pos;			// halfway into next bit.
		BitPos++;
	}

		m_bit_phase_pos += ( m_bit_phase_inc);
		if( m_bit_phase_pos >= Ts )
		{									// here every symbol time
			m_bit_phase_pos = std::fmod(m_bit_phase_pos, Ts);	//keep phase bounded
			if((BitPos==15) && (m_pk_pos==15))	//if missed the 15 bin before rollover
				Trigger = true;
			BitPos = 0;
			max = -1e10;
			for( int i=0; i<16; i++)		//find maximum energy pk
			{
				energy = m_sync_ave[i];
				if( energy > max )
				{
					m_new_pk_pos = i;
					max = energy;
				}
			}
			if(m_sq_open)
			{
				if( m_pk_pos == m_last_pk_pos+1 )	//calculate clock error
					m_clk_err_counter++;
				else
					if( m_pk_pos == m_last_pk_pos-1 )
						m_clk_err_counter--;
				if( m_clk_err_timer++ > 313 )	// every 10 seconds sample clk drift
				{
					m_clk_error = m_clk_err_counter*200;	//each count is 200ppm
					m_clk_err_counter = 0;
					m_clk_err_timer = 0;
					if( m_callback ) m_callback( cb_clkerror, m_clk_error, 0 );
				}
			}
			else
			{
				m_clk_error = 0;
				m_clk_err_counter = 0;
				m_clk_err_timer = 0;
			}
			m_last_pk_pos = m_pk_pos;
		}
		m_bit_pos = BitPos;
		return Trigger;
}
/* ------------------------------------------------------------------------- */
//Process input sample buffer
void decoder::operator()( const sample_type* samples, std::size_t sample_size )
{
	int j;
	const double* Kptr;
	std::complex<double> acc;
	std::complex<double> * Firptr;
	double vcophz = m_vco_phz;
	const int mod16_8 = (m_baudrate==baudrate::b63)?(8):(16);
	if(	m_afc_timer )
	{
		if(--m_afc_timer <= 0)
		{
			m_afc_timer = 0;
			m_afc_capture_on = false;
			// calculate new limits around latest receive frequency
			m_afc_max =  m_nco_phzinc + m_afc_limit;
			m_afc_min =  m_nco_phzinc - m_afc_limit;
			if(m_afc_min<=0.0)
				m_afc_min = 0.0;
		}
		else
		{
			m_afc_capture_on = true;
		}
	}
	for( std::size_t smpl = 0; smpl<sample_size; smpl++ )	// put new samples into Queue
	{
		//Generate complex sample by mixing input sample with NCO's sin/cos
		m_que1[m_fir1_state] = std::complex<double>(samples[smpl]*cos( vcophz ), samples[smpl]*sin( vcophz ) );
		//std::cout << m_que1[m_fir1_state] << " ZZZ " << samples[smpl] << std::endl;
		vcophz = vcophz + m_nco_phzinc + m_freq_error;
		if( vcophz > PI2)		//handle 2 Pi wrap around
			vcophz -= PI2;
		//decimate by 4 filter
		if( ( (++m_sample_cnt)%4 ) == 0 )	//calc first decimation filter every 4 samples
		{
			acc = std::complex<double>();
			Firptr = m_que1.data();
			Kptr = Dec4LPCoef + DEC4_LPFIR_LENGTH - m_fir1_state;
			for(j=0; j<	DEC4_LPFIR_LENGTH; j++)	//do the MAC's
			{
				//acc += std::complex<double>( Firptr->real()*(*Kptr), Firptr++->imag()*(*Kptr++) );
				acc += (*Firptr++) * (*Kptr++);
			}
			m_que2[m_fir2_state] = acc;
			//1.21 AA6YQ second decimation filter not required for PSK125
			if( m_baudrate==baudrate::b125 || ((m_sample_cnt%mod16_8) == 0) ) //calc second decimation filter every 8 or 16samples
			{
				if (m_baudrate!=baudrate::b125)	//decimate by 4 or 2 filter (PSK31 or PSK63)
				{
					acc = std::complex<double>();
					Firptr = m_que2.data();
					Kptr = Dec4LPCoef + DEC4_LPFIR_LENGTH - m_fir2_state;
					for(j=0; j<	DEC4_LPFIR_LENGTH; j++)	//do the MAC's
					{
						//acc += std::complex<double>((Firptr->real())*(*Kptr), (Firptr++->imag())*(*Kptr++));
						acc += (*Firptr++) * ( *Kptr++ );
					}
				}
				// here at Fs/16 == 500.0 Hz or 1000.0 Hz rate with latest sample in acc.
				// Matched Filter the I and Q data and also a frequency error filter
				//	filter puts filtered signals in variables m_FreqSignal and m_BitSignal.
				calc_bit_filter( acc );
				// Perform AGC operation
				calc_agc( m_freq_signal );
				// Calculate frequency error
				if(m_fast_afc_mode)
					calc_ffreq_error(m_freq_signal);
				else
					calc_freq_error(m_freq_signal);
				// Bit Timing synchronization
				if( symb_sync(m_bit_signal) )
					decode_symb(m_bit_signal);
				// Calculate IMD if only idles have been received and the energies collected
				if( m_imd_valid  )
				{
					if( m_calc_imd.calc_energies(acc) )
					{
						int m_imd_value;
						if( m_calc_imd.calc_value( m_imd_value ) )
						{
							if( m_callback ) m_callback( cb_imdrdy,  m_imd_value, 0 );
						}
						else
						{
							if( m_callback ) m_callback( cb_imdrdy, m_imd_value, 0x80 );
						}
					}
				}
				else
					m_calc_imd.reset();
			}
			if( --m_fir2_state < 0)	//deal with FIR pointer wraparound
				m_fir2_state = DEC4_LPFIR_LENGTH-1;
		}
		if( --m_fir1_state < 0)	//deal with FIR pointer wraparound
			m_fir1_state = DEC4_LPFIR_LENGTH-1;
	}
	m_sample_cnt = m_sample_cnt%16;
	m_vco_phz = vcophz;
	m_rx_frequency = int(0.5+((m_nco_phzinc + m_freq_error)*m_sample_freq/PI2 ) );
	if(0)
	{
		using namespace std;
		cout << "RXF " << m_rx_frequency << " PHZ "<< m_nco_phzinc << " SQLL " << m_agc_ave<< endl;
	}
	//	m_RxFrequency = (INT)(0.5+((m_NCOphzinc)*m_SampleFreq/PI2 ) );
}

/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */

