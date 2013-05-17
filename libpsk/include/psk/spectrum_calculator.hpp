/*
 * spectrum_calculator.hpp
 *
 *  Created on: 15-05-2013
 *      Author: lucck
 */
/*----------------------------------------------------------*/
#ifndef LIBPSK_SPECTRUM_CALCULATOR_HPP_
#define LIBPSK_SPECTRUM_CALCULATOR_HPP_
/*----------------------------------------------------------*/
#include <cstddef>
#include <complex>
#include <cstring>
#include <limits>
/*----------------------------------------------------------*/
namespace ham {
namespace psk {

/*----------------------------------------------------------*/
//Class spectrum calculator calculate the current spectrum
class spectrum_calculator {
public:
	//Spectrum data type
	typedef short pow_t;
	//FFT scale
	enum class scale : char
	{
		lin,
		log
	};
private:
	//Number of samples
	static constexpr auto MLOG2 = 9;
	static constexpr auto WIDTH = 1<<MLOG2;
	static constexpr int LOG_SCALE =  std::numeric_limits<pow_t>::max() + 1;
	static constexpr int LOGVAL_SCALE = std::log2(std::log2( LOG_SCALE + 1 ) + 1);
	//Make object noncopyable
	spectrum_calculator(const spectrum_calculator&) = delete;
	spectrum_calculator& operator=(const spectrum_calculator&) = delete;
public:
	//Constructor
	spectrum_calculator()
	{}
public:
	//Copy sample buffer
	void copy_samples( const pow_t* samples )
	{
		std::memcpy( m_real, samples, sizeof(pow_t) * WIDTH );
		m_energy_calculated = false;
	}
	//Get buffer samples and calculate it
	const pow_t& operator[]( size_t idx )
	{
		if( !m_energy_calculated )
		{
			m_energy_calculated = true;
			calculate_samples();
		}
		//TODO: Range checking
		return m_real[idx];
	}
	//Set scale
	void set_scale( scale sc, pow_t factor = std::numeric_limits<pow_t>::max() )
	{
		if( sc != m_scale)
		{
			m_scale = sc;
			m_energy_calculated = false;
		}
		if( m_factor != factor )
		{
			m_factor = factor;
			m_energy_calculated = false;
		}
	}
	//Get min and max value
	int min_value() const
	{
		constexpr int minv = 20 * std::log10( 1.0/std::numeric_limits<pow_t>::max() );
		if( m_scale == scale::log ) return minv;
		else return 0;
	}
	//Get MAX scale value
	int max_value() const
	{
		if( m_scale == scale::log ) return 0;
		else return 100;
	}
private:
	//Calculate samplees
	void calculate_samples();
private:
	//Private pointer for store data
	std::complex<pow_t>m_cplx[WIDTH];
	/*
	    For any complex number z, reinterpret_cast<T(&)[2]>(z)[0] is the real part of z
	    and reinterpret_cast<T(&)[2]>(z)[1] is the imaginary part of z.
		For any pointer to an element of an array of complex numbers p
		and any valid array index i, reinterpret_cast<T*>(p)[2*i] is the real part
		of the complex number p[i], and reinterpret_cast<T*>(p)[2*i + 1] is the
		imaginary part of the complex number p[i]
	 */
	pow_t* const m_real { reinterpret_cast<pow_t* const>(m_cplx) };
	bool m_energy_calculated { false };		//Energy is calculated
	scale m_scale { scale::log };			//Current scale
	pow_t m_factor { std::numeric_limits<pow_t>::max() };
};

/*----------------------------------------------------------*/

} /* namespace psk */
} /* namespace ham */
/*----------------------------------------------------------*/
#endif /* LIBPSK_SPECTRUM_CALCULATOR_HPP_ */
/*----------------------------------------------------------*/
