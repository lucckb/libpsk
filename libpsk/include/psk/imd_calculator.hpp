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
/* Calculate the IMD signal quality */
class imd_calculator
{
	//Make object noncopyable
	imd_calculator(const imd_calculator&) = delete;
	imd_calculator& operator=(const imd_calculator&) = delete;
	static auto constexpr NUM_FILTERS = 3;
public:
	imd_calculator() {}
	void reset();
	bool calc_energies( std::complex<int> samp );
	bool calc_value();
	int get_value() const
	{
		return m_imd;
	}
private:
	std::array<int, NUM_FILTERS> I1 {{}};
	std::array<int, NUM_FILTERS> I2 {{}};
	std::array<int, NUM_FILTERS> Q1 {{}};
	std::array<int, NUM_FILTERS> Q2 {{}};
	std::array<unsigned long long, NUM_FILTERS> m_energy {{}};
	int m_ncount {};
	int m_snr {};
	int m_imd {};
};
/* ------------------------------------------------------------------------- */
} /* namespace _internal */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
#endif /* IMD_CALCULATOR_HPP_ */
/* ------------------------------------------------------------------------- */
