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
public:
	//Constructor
	symbol_synchronizer( int sample_freq, std::function< void(int param1)> callback )
		: m_bit_phase_inc( 16.0 / sample_freq ), m_callback(callback)
	{}
	//Reset the decoder
	void reset()
	{
		m_sync_ave.fill( 0 );
		m_clk_err_counter = 0;
		m_clk_err_timer = 0;
		m_clk_error = 0;
	}
	const std::array<long, 16>& get_sync_data() const
	{
		return m_sync_array;
	}
	///Synchronize symbol data
	bool operator()( std::complex<int> s, bool sq_open )
	{

		//TEMPORARY ONLY
		std::complex<double> sample = std::complex<double>( (double)s.real()/(1<<15), (double)s.imag()/(1<<15) );
		constexpr auto Ts = .032+.00000;			// Ts == symbol period
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

		m_bit_phase_pos += m_bit_phase_inc;
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
			if(sq_open)
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
					//if( m_callback ) m_callback( cb_clkerror, m_clk_error, 0 );
					if( m_callback ) m_callback( m_clk_error );
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
private:
	int m_bit_pos {};
	std::array<double, 21> m_sync_ave {{}};
	int m_pk_pos {};
	std::array<long, 16> m_sync_array {{}};
	int m_new_pk_pos { 5 };
	double m_bit_phase_pos {};
	double m_bit_phase_inc;
	int m_last_pk_pos {};
	int m_clk_err_counter {};
	int m_clk_err_timer {};
	int m_clk_error {};
	std::function< void(int param1)> m_callback;
};


/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* SYNCHRONIZER_HPP_ */

/* ------------------------------------------------------------------------- */
