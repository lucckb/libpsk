/* ------------------------------------------------------------------------- */
/*
 * decoder.cpp
 *
 *  Created on: 24-03-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "psk/decoder.hpp"
#include <complex>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <iomanip>



//TODO: Temporary  cast for test purpose only
template< typename T>
	std::complex<double> inline cplxf_cast( std::complex<T> v, int scale = 1 )
{
	return std::complex<double>(double(v.real())/double(scale), double(v.imag())/double(scale));
}

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Unnamed anon namespace
namespace
{

     constexpr double SMUL = std::numeric_limits<short>::max();
	 constexpr short Dec4LPCoef[ ] = {
	 short(-0.00021203644 * SMUL + 0.5),
	 short(-0.00070252426 * SMUL + 0.5),
	 short(-0.0016680526 * SMUL + 0.5),
	 short(-0.0031934799 * SMUL + 0.5),
	 short(-0.0051899752 * SMUL + 0.5),
	 short(-0.0072862086 * SMUL + 0.5),
	 short(-0.0087714235 * SMUL + 0.5),
	 short(-0.0086272102 * SMUL + 0.5),
	 short(-0.0056735648 * SMUL + 0.5),
	 short(0.0011784719 * SMUL + 0.5),
     short(0.01261353 * SMUL + 0.5),
     short(0.028615709 * SMUL + 0.5),
	 short(0.048280707 * SMUL + 0.5),
	 short(0.069812051 * SMUL + 0.5),
	 short(0.090735013 * SMUL + 0.5),
	 short(0.10830381 * SMUL + 0.5),
	 short(0.12001897 * SMUL + 0.5),
	 short(0.12413413 * SMUL + 0.5),
	 short(0.12001897 * SMUL + 0.5),
     short(0.10830381 * SMUL + 0.5),
	 short(0.090735013 * SMUL + 0.5),
	 short(0.069812051 * SMUL + 0.5),
	 short(0.048280707 * SMUL + 0.5),
	 short(0.028615709 * SMUL + 0.5),
	 short(0.01261353 * SMUL + 0.5),
	 short(0.0011784719 * SMUL + 0.5),
	 short(-0.0056735648 * SMUL + 0.5),
	 short(-0.0086272102 * SMUL + 0.5),
	 short(-0.0087714235 * SMUL + 0.5),
	 short(-0.0072862086 * SMUL + 0.5),
	 short(-0.0051899752 * SMUL + 0.5),
	 short(-0.0031934799 * SMUL + 0.5),
	 short(-0.0016680526 * SMUL + 0.5),
	 short(-0.00070252426 * SMUL + 0.5),
	 short(-0.00021203644 * SMUL + 0.5),
	//
	short(-0.00021203644 * SMUL + 0.5),
	short(-0.00070252426 * SMUL + 0.5),
	short(-0.0016680526 * SMUL + 0.5),
	short(-0.0031934799 * SMUL + 0.5),
	short(-0.0051899752 * SMUL + 0.5),
	short(-0.0072862086 * SMUL + 0.5),
	short(-0.0087714235 * SMUL + 0.5),
	short(-0.0086272102 * SMUL + 0.5),
	short(-0.0056735648 * SMUL + 0.5),
	short(0.0011784719 * SMUL + 0.5),
    short(0.01261353 * SMUL + 0.5),
	short(0.028615709 * SMUL + 0.5),
	short(0.048280707 * SMUL + 0.5),
	short(0.069812051 * SMUL + 0.5),
	short(0.090735013 * SMUL + 0.5),
	short(0.10830381 * SMUL + 0.5),
	short(0.12001897 * SMUL + 0.5),
	short(0.12413413 * SMUL + 0.5),
	short(0.12001897 * SMUL + 0.5),
	short(0.10830381 * SMUL + 0.5),
	short(0.090735013 * SMUL + 0.5),
	short(0.069812051 * SMUL + 0.5),
	short(0.048280707 * SMUL + 0.5),
	short(0.028615709 * SMUL + 0.5),
	short(0.01261353 * SMUL + 0.5),
	short(0.0011784719 * SMUL + 0.5),
	short(-0.0056735648 * SMUL + 0.5),
	short(-0.0086272102 * SMUL + 0.5),
	short(-0.0087714235 * SMUL + 0.5),
	short(-0.0072862086 * SMUL + 0.5),
	short(-0.0051899752 * SMUL + 0.5),
	short(-0.0031934799 * SMUL + 0.5),
	short(-0.0016680526 * SMUL + 0.5),
	short(-0.00070252426 * SMUL + 0.5),
	short(-0.00021203644 * SMUL + 0.5)
	};

	constexpr short FreqFirCoef[] = { // 31.25 Hz bw LP filter for frequency error
	short( -0.00088229088 * SMUL + 0.5),
	short(  -0.0011374389 * SMUL + 0.5),
	short(  -0.0016412868 * SMUL + 0.5),
	short(  -0.0020520989 * SMUL + 0.5),
	short(  -0.0022175618 * SMUL + 0.5),
	short(  -0.0019791161 * SMUL + 0.5),
	short(  -0.0012057952 * SMUL + 0.5),
	short(  0.00016527251 * SMUL + 0.5),
	short(   0.0020943472 * SMUL + 0.5),
	short(   0.0044122668 * SMUL + 0.5),
	short(   0.0068189371 * SMUL + 0.5),
	short(   0.0089045933 * SMUL + 0.5),
	short(    0.010203678 * SMUL + 0.5),
	short(    0.010266316 * SMUL + 0.5),
	short(   0.0087507451 * SMUL + 0.5),
	short(   0.0055136683 * SMUL + 0.5),
	short(  0.00068616242 * SMUL + 0.5),
	short(   -0.005288313 * SMUL + 0.5),
	short(   -0.011653249 * SMUL + 0.5),
	short(   -0.017399749 * SMUL + 0.5),
	short(   -0.021376642 * SMUL + 0.5),
	short(   -0.022435377 * SMUL + 0.5),
	short(   -0.019606873 * SMUL + 0.5),
	short(   -0.012263665 * SMUL + 0.5),
	short( -0.00026078671 * SMUL + 0.5),
	short(    0.015986088 * SMUL + 0.5),
	short(    0.035491391 * SMUL + 0.5),
	short(    0.056773305 * SMUL + 0.5),
	short(    0.078002392 * SMUL + 0.5),
	short(    0.097207364 * SMUL + 0.5),
	short(     0.11251413 * SMUL + 0.5),
	short(     0.12237776 * SMUL + 0.5),
	short(     0.12580366 * SMUL + 0.5),
	short(     0.12237776 * SMUL + 0.5),
	short(     0.11251413 * SMUL + 0.5),
	short(    0.097207364 * SMUL + 0.5),
	short(    0.078002392 * SMUL + 0.5),
	short(    0.056773305 * SMUL + 0.5),
	short(    0.035491391 * SMUL + 0.5),
	short(    0.015986088 * SMUL + 0.5),
	short( -0.00026078671 * SMUL + 0.5),
	short(   -0.012263665 * SMUL + 0.5),
	short(   -0.019606873 * SMUL + 0.5),
	short(   -0.022435377 * SMUL + 0.5),
	short(   -0.021376642 * SMUL + 0.5),
	short(   -0.017399749 * SMUL + 0.5),
	short(   -0.011653249 * SMUL + 0.5),
	short(   -0.005288313 * SMUL + 0.5),
	short(  0.00068616242 * SMUL + 0.5),
	short(   0.0055136683 * SMUL + 0.5),
	short(   0.0087507451 * SMUL + 0.5),
	short(    0.010266316 * SMUL + 0.5),
	short(    0.010203678 * SMUL + 0.5),
	short(   0.0089045933 * SMUL + 0.5),
	short(   0.0068189371 * SMUL + 0.5),
	short(   0.0044122668 * SMUL + 0.5),
	short(   0.0020943472 * SMUL + 0.5),
	short(  0.00016527251 * SMUL + 0.5),
	short(  -0.0012057952 * SMUL + 0.5),
	short(  -0.0019791161 * SMUL + 0.5),
	short(  -0.0022175618 * SMUL + 0.5),
	short(  -0.0020520989 * SMUL + 0.5),
	short(  -0.0016412868 * SMUL + 0.5),
	short(  -0.0011374389 * SMUL + 0.5),
	short( -0.00088229088 * SMUL + 0.5),
	//
	short( -0.00088229088 * SMUL + 0.5),
	short(  -0.0011374389 * SMUL + 0.5),
	short(  -0.0016412868 * SMUL + 0.5),
	short(  -0.0020520989 * SMUL + 0.5),
	short(  -0.0022175618 * SMUL + 0.5),
	short(  -0.0019791161 * SMUL + 0.5),
	short(  -0.0012057952 * SMUL + 0.5),
	short(  0.00016527251 * SMUL + 0.5),
	short(   0.0020943472 * SMUL + 0.5),
	short(   0.0044122668 * SMUL + 0.5),
	short(   0.0068189371 * SMUL + 0.5),
	short(   0.0089045933 * SMUL + 0.5),
	short(    0.010203678 * SMUL + 0.5),
	short(    0.010266316 * SMUL + 0.5),
	short(   0.0087507451 * SMUL + 0.5),
	short(   0.0055136683 * SMUL + 0.5),
	short(  0.00068616242 * SMUL + 0.5),
	short(   -0.005288313 * SMUL + 0.5),
	short(   -0.011653249 * SMUL + 0.5),
	short(   -0.017399749 * SMUL + 0.5),
	short(   -0.021376642 * SMUL + 0.5),
	short(   -0.022435377 * SMUL + 0.5),
	short(   -0.019606873 * SMUL + 0.5),
	short(   -0.012263665 * SMUL + 0.5),
	short( -0.00026078671 * SMUL + 0.5),
	short(    0.015986088 * SMUL + 0.5),
	short(    0.035491391 * SMUL + 0.5),
	short(    0.056773305 * SMUL + 0.5),
	short(    0.078002392 * SMUL + 0.5),
	short(    0.097207364 * SMUL + 0.5),
	short(     0.11251413 * SMUL + 0.5),
	short(     0.12237776 * SMUL + 0.5),
	short(     0.12580366 * SMUL + 0.5),
	short(     0.12237776 * SMUL + 0.5),
	short(     0.11251413 * SMUL + 0.5),
	short(    0.097207364 * SMUL + 0.5),
	short(    0.078002392 * SMUL + 0.5),
	short(    0.056773305 * SMUL + 0.5),
	short(    0.035491391 * SMUL + 0.5),
	short(    0.015986088 * SMUL + 0.5),
	short( -0.00026078671 * SMUL + 0.5),
	short(   -0.012263665 * SMUL + 0.5),
	short(   -0.019606873 * SMUL + 0.5),
	short(   -0.022435377 * SMUL + 0.5),
	short(   -0.021376642 * SMUL + 0.5),
	short(   -0.017399749 * SMUL + 0.5),
	short(   -0.011653249 * SMUL + 0.5),
	short(   -0.005288313 * SMUL + 0.5),
	short(  0.00068616242 * SMUL + 0.5),
	short(   0.0055136683 * SMUL + 0.5),
	short(   0.0087507451 * SMUL + 0.5),
	short(    0.010266316 * SMUL + 0.5),
	short(    0.010203678 * SMUL + 0.5),
	short(   0.0089045933 * SMUL + 0.5),
	short(   0.0068189371 * SMUL + 0.5),
	short(   0.0044122668 * SMUL + 0.5),
	short(   0.0020943472 * SMUL + 0.5),
	short(  0.00016527251 * SMUL + 0.5),
	short(  -0.0012057952 * SMUL + 0.5),
	short(  -0.0019791161 * SMUL + 0.5),
	short(  -0.0022175618 * SMUL + 0.5),
	short(  -0.0020520989 * SMUL + 0.5),
	short(  -0.0016412868 * SMUL + 0.5),
	short(  -0.0011374389 * SMUL + 0.5),
	short( -0.00088229088 * SMUL + 0.5)
	};
	constexpr short BitFirCoef[] = { // 16 Hz bw LP filter for data recovery
	short( 4.3453566e-005 * SMUL + 0.5),
	short( -0.00049122414 * SMUL + 0.5),
	short( -0.00078771292 * SMUL + 0.5),
	short(  -0.0013507826 * SMUL + 0.5),
	short(  -0.0021287814 * SMUL + 0.5),
	short(   -0.003133466 * SMUL + 0.5),
	short(   -0.004366817 * SMUL + 0.5),
	short(  -0.0058112187 * SMUL + 0.5),
	short(  -0.0074249976 * SMUL + 0.5),
	short(  -0.0091398882 * SMUL + 0.5),
	short(   -0.010860157 * SMUL + 0.5),
	short(   -0.012464086 * SMUL + 0.5),
	short(   -0.013807772 * SMUL + 0.5),
	short(   -0.014731191 * SMUL + 0.5),
	short(   -0.015067057 * SMUL + 0.5),
	short(   -0.014650894 * SMUL + 0.5),
	short(   -0.013333425 * SMUL + 0.5),
	short(    -0.01099166 * SMUL + 0.5),
	short(  -0.0075431246 * SMUL + 0.5),
	short(  -0.0029527849 * SMUL + 0.5),
	short(   0.0027546292 * SMUL + 0.5),
	short(   0.0094932775 * SMUL + 0.5),
	short(    0.017113308 * SMUL + 0.5),
	short(    0.025403511 * SMUL + 0.5),
	short(    0.034099681 * SMUL + 0.5),
	short(    0.042895839 * SMUL + 0.5),
	short(    0.051458575 * SMUL + 0.5),
	short(    0.059444853 * SMUL + 0.5),
	short(    0.066521003 * SMUL + 0.5),
	short(    0.072381617 * SMUL + 0.5),
	short(    0.076767694 * SMUL + 0.5),
	short(    0.079481619 * SMUL + 0.5),
	short(    0.080420311 * SMUL + 0.5),
	short(    0.079481619 * SMUL + 0.5),
	short(    0.076767694 * SMUL + 0.5),
	short(    0.072381617 * SMUL + 0.5),
	short(    0.066521003 * SMUL + 0.5),
	short(    0.059444853 * SMUL + 0.5),
	short(    0.051458575 * SMUL + 0.5),
	short(    0.042895839 * SMUL + 0.5),
	short(    0.034099681 * SMUL + 0.5),
	short(    0.025403511 * SMUL + 0.5),
	short(    0.017113308 * SMUL + 0.5),
	short(   0.0094932775 * SMUL + 0.5),
	short(   0.0027546292 * SMUL + 0.5),
	short(  -0.0029527849 * SMUL + 0.5),
	short(  -0.0075431246 * SMUL + 0.5),
	short(    -0.01099166 * SMUL + 0.5),
	short(   -0.013333425 * SMUL + 0.5),
	short(   -0.014650894 * SMUL + 0.5),
	short(   -0.015067057 * SMUL + 0.5),
	short(   -0.014731191 * SMUL + 0.5),
	short(   -0.013807772 * SMUL + 0.5),
	short(   -0.012464086 * SMUL + 0.5),
	short(   -0.010860157 * SMUL + 0.5),
	short(  -0.0091398882 * SMUL + 0.5),
	short(  -0.0074249976 * SMUL + 0.5),
	short(  -0.0058112187 * SMUL + 0.5),
	short(   -0.004366817 * SMUL + 0.5),
	short(   -0.003133466 * SMUL + 0.5),
	short(  -0.0021287814 * SMUL + 0.5),
	short(  -0.0013507826 * SMUL + 0.5),
	short( -0.00078771292 * SMUL + 0.5),
	short( -0.00049122414 * SMUL + 0.5),
	short( 4.3453566e-005 * SMUL + 0.5),
	//
	short( 4.3453566e-005 * SMUL + 0.5),
	short( -0.00049122414 * SMUL + 0.5),
	short( -0.00078771292 * SMUL + 0.5),
	short(  -0.0013507826 * SMUL + 0.5),
	short(  -0.0021287814 * SMUL + 0.5),
	short(   -0.003133466 * SMUL + 0.5),
	short(   -0.004366817 * SMUL + 0.5),
	short(  -0.0058112187 * SMUL + 0.5),
	short(  -0.0074249976 * SMUL + 0.5),
	short(  -0.0091398882 * SMUL + 0.5),
	short(   -0.010860157 * SMUL + 0.5),
	short(   -0.012464086 * SMUL + 0.5),
	short(   -0.013807772 * SMUL + 0.5),
	short(   -0.014731191 * SMUL + 0.5),
	short(   -0.015067057 * SMUL + 0.5),
	short(   -0.014650894 * SMUL + 0.5),
	short(   -0.013333425 * SMUL + 0.5),
	short(    -0.01099166 * SMUL + 0.5),
	short(  -0.0075431246 * SMUL + 0.5),
	short(  -0.0029527849 * SMUL + 0.5),
	short(   0.0027546292 * SMUL + 0.5),
	short(   0.0094932775 * SMUL + 0.5),
	short(    0.017113308 * SMUL + 0.5),
	short(    0.025403511 * SMUL + 0.5),
	short(    0.034099681 * SMUL + 0.5),
	short(    0.042895839 * SMUL + 0.5),
	short(    0.051458575 * SMUL + 0.5),
	short(    0.059444853 * SMUL + 0.5),
	short(    0.066521003 * SMUL + 0.5),
	short(    0.072381617 * SMUL + 0.5),
	short(    0.076767694 * SMUL + 0.5),
	short(    0.079481619 * SMUL + 0.5),
	short(    0.080420311 * SMUL + 0.5),
	short(    0.079481619 * SMUL + 0.5),
	short(    0.076767694 * SMUL + 0.5),
	short(    0.072381617 * SMUL + 0.5),
	short(    0.066521003 * SMUL + 0.5),
	short(    0.059444853 * SMUL + 0.5),
	short(    0.051458575 * SMUL + 0.5),
	short(    0.042895839 * SMUL + 0.5),
	short(    0.034099681 * SMUL + 0.5),
	short(    0.025403511 * SMUL + 0.5),
	short(    0.017113308 * SMUL + 0.5),
	short(   0.0094932775 * SMUL + 0.5),
	short(   0.0027546292 * SMUL + 0.5),
	short(  -0.0029527849 * SMUL + 0.5),
	short(  -0.0075431246 * SMUL + 0.5),
	short(    -0.01099166 * SMUL + 0.5),
	short(   -0.013333425 * SMUL + 0.5),
	short(   -0.014650894 * SMUL + 0.5),
	short(   -0.015067057 * SMUL + 0.5),
	short(   -0.014731191 * SMUL + 0.5),
	short(   -0.013807772 * SMUL + 0.5),
	short(   -0.012464086 * SMUL + 0.5),
	short(   -0.010860157 * SMUL + 0.5),
	short(  -0.0091398882 * SMUL + 0.5),
	short(  -0.0074249976 * SMUL + 0.5),
	short(  -0.0058112187 * SMUL + 0.5),
	short(   -0.004366817 * SMUL + 0.5),
	short(   -0.003133466 * SMUL + 0.5),
	short(  -0.0021287814 * SMUL + 0.5),
	short(  -0.0013507826 * SMUL + 0.5),
	short( -0.00078771292 * SMUL + 0.5),
	short( -0.00049122414 * SMUL + 0.5),
	short( 4.3453566e-005 * SMUL + 0.5)
	};

}

/* ------------------------------------------------------------------------- */
//Construct the decoder object
decoder::decoder( samplerate_type sample_rate, event_callback_type callback ) :
	  m_callback(callback ),
	  m_nco_phzinc( (PI2I*m_rx_frequency)/int(sample_rate) ),
      m_sync(  std::bind( callback, decoder::cb_clkerror, std::placeholders::_1, 0) ),
	  m_sample_freq( sample_rate ),
	  m_afc(m_nco_phzinc, 50.0*PI2I/int(sample_rate) ),
      m_fir1_dec( Dec4LPCoef ), m_fir2_dec( Dec4LPCoef ),
      m_bit_fir( BitFirCoef ), m_freq_fir( FreqFirCoef )
{
	//Initialy reset the decoder
	reset();
}

/* ------------------------------------------------------------------------- */
void decoder::set_frequency( int freq )
{
	if( freq != m_rx_frequency)
	{
		m_rx_frequency = freq;
		m_nco_phzinc = (PI2I*freq)/m_sample_freq;
		m_squelch.reset_frequency();
		m_afc.reset( m_nco_phzinc );
	}
}
/* ------------------------------------------------------------------------- */
//Set AFC limit
void decoder::set_afc_limit( int limit )
{
	m_afc.set_afc_limit( limit, m_sample_freq,  m_nco_phzinc );
}

/* ------------------------------------------------------------------------- */
//Reset decoder
void decoder::reset()
{
	m_fir1_dec.reset();
    m_fir2_dec.reset();
    m_viterbi_decoder.reset();
	m_sync.reset();
	m_angle_calc.reset();
	m_squelch.reset();
}

/* ------------------------------------------------------------------------- */
void decoder::decode_symb( std::complex<int> newsamp )
{
	//Successive fix it
	int angle = m_angle_calc( newsamp, m_agc(), m_rx_mode == mode::qpskl );
	{
		//Calc quality
		auto freq_error = m_squelch( angle, m_agc(), is_qpsk(),
		    	m_rx_mode == mode::qpskl, m_rx_mode == mode::bpsk );
		 m_afc.update_angle_error( freq_error );
	}
	bool bit;
	if(m_rx_mode == mode::bpsk)
	{
		//calc BPSK symbol over 2 chips
		//vect.imag( m_angle_calc.get_energy() );
		bit = bool(m_angle_calc.get_energy() > 0);
	}
	else
	{
		bit = m_viterbi_decoder( angle );
	}
	//Decode symbol
	auto ch = m_symb_decoder( bit );
	if(ch>0 && m_squelch.is_open() )
	{
		if( m_callback ) m_callback( cb_rxchar, ch, 0 );
	}
}

/* ------------------------------------------------------------------------- */
//Process input sample buffer
void decoder::operator()( const sample_type* samples, std::size_t sample_size )
{
	const int mod16_8 = (m_baudrate==baudrate::b63)?(8):(16);
	m_afc.handle_sample_timer( m_nco_phzinc );
	for( std::size_t smpl = 0; smpl<sample_size; smpl++ )	// put new samples into Queue
	{
        m_fir1_dec(
        	m_nco_mix( samples[smpl], m_nco_phzinc + m_afc.get_freq_error() )
        );
		//decimate by 4 filter
		if( ( (++m_sample_cnt)%4 ) == 0 )	//calc first decimation filter every 4 samples
		{
            //1.21 AA6YQ second decimation filter not required for PSK125
			if( m_baudrate==baudrate::b125 || ((m_sample_cnt%mod16_8) == 0) ) //calc second decimation filter every 8 or 16samples
			{
				std::complex<double> filtered_sample;
                if (m_baudrate!=baudrate::b125)	//decimate by 4 or 2 filter (PSK31 or PSK63)
				{
                    m_fir2_dec ( m_fir1_dec() );
                    filtered_sample =  m_fir2_dec();
				}
                else
                {
                    filtered_sample =  m_fir1_dec();
                }
				// here at Fs/16 == 500.0 Hz or 1000.0 Hz rate with latest sample in acc.
				// Matched Filter the I and Q data and also a frequency error filter
				//	filter puts filtered signals in variables m_FreqSignal and m_BitSignal.
            	m_bit_fir( filtered_sample );
            	m_freq_fir( filtered_sample );
            	std::complex<long> freq_signal = m_freq_fir();
            	std::complex<long> bit_signal = m_bit_fir();
				// Perform AGC operation
            	m_agc( freq_signal );
            	freq_signal = m_agc.scale( freq_signal );
            	bit_signal  = m_agc.scale( bit_signal );

            	//Perform AFC operation
            	m_nco_phzinc = m_afc( freq_signal, m_nco_phzinc);
				// Bit Timing synchronization
				if( m_sync(bit_signal, m_squelch.is_open()) )
					decode_symb( bit_signal );
				// Calculate IMD if only idles have been received and the energies collected
				if( m_squelch.is_imd_valid() )
				{
					if( m_calc_imd.calc_energies( filtered_sample ) )
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
		}
	}
	m_sample_cnt = m_sample_cnt%16;
	m_rx_frequency = ((m_nco_phzinc + m_afc.get_freq_error())*m_sample_freq)/PI2I;
}

/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */

