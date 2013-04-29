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

	void operator()( std::complex<T> sample, T angle_error )
	{

	}

private:
	//FAST AFC mode
	void calc_ffreq_error( std::complex<T> IQ,  T angle_error )
	{

	}
	void calc_freq_error( std::complex<T> IQ,  T angle_error )
	{

	}
};

/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* AFC_CONTROL_HPP_ */
/* ------------------------------------------------------------------------- */
