/*
 * imd_calculator.hpp
 *
 *  Created on: 24-03-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_IMD_CALCULATOR_HPP_
#define LIBPSK_IMD_CALCULATOR_HPP_
/* ------------------------------------------------------------------------- */
#include <complex>
#include <array>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */

class imd_calculator {
	//Make object noncopyable
	imd_calculator(const imd_calculator&) = delete;
	imd_calculator& operator=(const imd_calculator&) = delete;
	static auto constexpr NUM_FILTERS = 3;
public:
	imd_calculator() {}
	void reset();
	bool calc_energies( std::complex<int> samp );
	bool calc_value( int &imd_val );
private:
	std::array<double, NUM_FILTERS> I1 {{}};
	std::array<double, NUM_FILTERS> I2 {{}};
	std::array<double, NUM_FILTERS> Q1 {{}};
	std::array<double, NUM_FILTERS> Q2 {{}};
	std::array<double, NUM_FILTERS> m_energy {{}};
	int m_ncount {};
	double m_snr {};
	double m_imd {};
};
/* ------------------------------------------------------------------------- */
} /* namespace _internal */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
#endif /* IMD_CALCULATOR_HPP_ */
/* ------------------------------------------------------------------------- */
