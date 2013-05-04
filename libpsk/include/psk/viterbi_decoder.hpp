/*
 * viterbi_decoder.hpp
 *
 *  Created on: 04-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_VITERBI_DECODER_HPP_
#define LIBPSK_VITERBI_DECODER_HPP_
/* ------------------------------------------------------------------------- */
#include <cmath>
#include <array>
#include <limits>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
//Viterbi decoder class for PSK
class viterbi_decoder
{
	//Make object noncopyable
	viterbi_decoder(const viterbi_decoder&) = delete;
	viterbi_decoder& operator=(const viterbi_decoder&) = delete;
public:
	//SCALE factor
	static constexpr auto SCALE = 1<<15;
private:
	static constexpr auto PI2 = 8.0 * std::atan(1.0);
	// phase wraparound correction tables for viterbi decoder
	static constexpr int ANGLE_TBL1[] = { int((3.0*PI2/4.0)*SCALE), 0, int((PI2/4.0)*SCALE), int((PI2/2.0)*SCALE) };
	static constexpr int ANGLE_TBL2[] = { int((3.0*PI2/4.0)*SCALE), int(PI2*SCALE), int((PI2/4.0)*SCALE), int((PI2/2.0)*SCALE) };
	// Lookup table to get symbol from non-inverted data stream
	static constexpr unsigned char ConvolutionCodeTable[] =
	{
		2, 1, 3, 0, 3, 0, 2, 1,
		0, 3, 1, 2, 1, 2, 0, 3,
		1, 2, 0, 3, 0, 3, 1, 2,
		3, 0, 2, 1, 2, 1, 3, 0
	};
	//Survivor states in decoder
	struct survivor_states
	{
		survivor_states( int _path_distance = int(), int _bit_estimates = int() )
			: path_distance(_path_distance),  bit_estimates(_bit_estimates ) {}
		int path_distance;	// sum of all metrics for a given survivor path
		int bit_estimates;		// the bit pattern estimate associated with given survivor path
	};
	static constexpr int RESET_VAL = (3.0*PI2/4.0)*SCALE;
public:
	//Constructor
	viterbi_decoder()
	{}
	//Reset the decoder
	void reset()
	{
		for( auto &v : m_survivor_states )
		{
			v = survivor_states();
		}
		for( int i=0; i<21; i++ )
		{
			(*this)( RESET_VAL );	// init the Viterbi decoder
		}
	}
	//Default decode func
	bool operator()( int newangle )
	{
		int pathdist[32];
		int min( std::numeric_limits<int>::max() ); //make sure can find a minimum value
		int bitestimates[32];
		int ones;
		const int * const pAngleTbl = (newangle >= int(PI2/2*SCALE))?(ANGLE_TBL2):(ANGLE_TBL1);
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
private:
	std::array<survivor_states, 16> m_survivor_states {{}};
};
/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* VITERBI_DECODER_HPP_ */
