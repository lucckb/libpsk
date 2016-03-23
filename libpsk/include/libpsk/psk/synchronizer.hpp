/*
 * synchronizer.hpp
 *
 *  Created on: 02-05-2013
 *      Author: lucck
 */

#ifndef PSK_SYNCHRONIZER_HPP_
#define PSK_SYNCHRONIZER_HPP_

/* ------------------------------------------------------------------------- */
#include <complex>
#include <array>
#include <functional>
#include <limits>
#include <foundation/dsp/dsp_basic_ops.hpp>


/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
class symbol_synchronizer
{
	//Make object noncopyable
	symbol_synchronizer(const symbol_synchronizer&) = delete;
	symbol_synchronizer& operator=(const symbol_synchronizer&) = delete;
private:
	static constexpr int HALF_TBL[] =
	{
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
	static constexpr auto PHZ_SCALE = 1<<15;
public:
	//Constructor
	symbol_synchronizer()
	{}
	//Reset the decoder
	void reset()
	{
		m_sync_ave.fill( 0 );
		m_clk_err_counter = 0;
		m_clk_err_timer = 0;
		m_clk_error = 0;
	}
	const std::array<unsigned int, 16>& get_sync_data() const
	{
		return m_sync_array;
	}
	///Synchronize symbol data
	bool operator()( std::complex<int> sample, bool sq_open, bool &clk_err_avail )
	{
		constexpr auto SMPL_SCALE = 1<<15;
		constexpr auto ENERGY_SCALE = SMPL_SCALE * 2;
		//constexpr int Ts = .032;

		bool trigger=false;
		unsigned int max, energy;
		int bit_pos = m_bit_pos;
		if(bit_pos<16)
		{
			//TODO: Add saturate from DSP
			{
				using namespace dsp::cpu;
				const auto r2 = static_cast<unsigned int>(saturated_cast<short>(sample.real())) *
								static_cast<unsigned int>(saturated_cast<short>(sample.real()));
				const auto i2 = static_cast<unsigned int>(saturated_cast<short>(sample.imag())) *
						        static_cast<unsigned int>(saturated_cast<short>(sample.imag()));
				energy = r2/SMPL_SCALE + i2/SMPL_SCALE;
			}
			//if( energy > 4 * ENERGY_SCALE)		//wait for AGC to settle down
			//	energy = ENERGY_SCALE;
			m_sync_ave[bit_pos] = m_sync_ave[bit_pos] - m_sync_ave[bit_pos]/82 + energy/82;

			if( bit_pos == m_pk_pos )	// see if at middle of symbol
			{
				trigger = true;
				//TODO: Fixme later???
				m_sync_array[m_pk_pos] = (900*m_sync_ave[m_pk_pos])/ENERGY_SCALE;
			}
			else
			{
				trigger = false;
				m_sync_array[bit_pos] = (750*m_sync_ave[bit_pos])/ENERGY_SCALE;
			}
			if( bit_pos == HALF_TBL[m_new_pk_pos] )	//don't change pk pos until
				m_pk_pos = m_new_pk_pos;			// halfway into next bit.
			bit_pos++;
		}
		++m_bit_phase_pos;
		if( m_bit_phase_pos >= 16 )
		{									// here every symbol time
			m_bit_phase_pos = 0;	//keep phase bounded
			if((bit_pos==15) && (m_pk_pos==15))	//if missed the 15 bin before rollover
				trigger = true;
			bit_pos = 0;
			max = std::numeric_limits<decltype(max)>::min();
			for( int i=0; i<16; i++)		//find maximum energy pk
			{
				energy = m_sync_ave[i];
				if( energy > max )
				{
					m_new_pk_pos = i;
					max = energy;
				}
			}
			if( sq_open )
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
					//m_clk_error callback update
					//if( m_callback ) m_callback( m_clk_error );
					clk_err_avail = true;
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
		m_bit_pos = bit_pos;
		return trigger;
	}
	int get_clk_error() const
	{
		return m_clk_error;
	}
private:
	int m_bit_pos {};
	std::array<unsigned int, 16> m_sync_ave {{}};
	int m_pk_pos {};
	std::array<unsigned int, 16> m_sync_array {{}};
	int m_new_pk_pos { 5 };
	int m_bit_phase_pos {};
	int m_last_pk_pos {};
	int m_clk_err_counter {};
	int m_clk_err_timer {};
	int m_clk_error {};
};


/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* SYNCHRONIZER_HPP_ */

/* ------------------------------------------------------------------------- */
