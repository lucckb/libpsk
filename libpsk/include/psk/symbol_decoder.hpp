/* ------------------------------------------------------------------------- */
/*
 * symbol_decoder.hpp
 *
 *  Created on: 03-05-2013
 *      Author: lucck
 */

#ifndef PSK_SYMBOL_DECODER_HPP_
#define PSK_SYMBOL_DECODER_HPP_

/* ------------------------------------------------------------------------- */
#include "psk/varicode.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */

class symbol_decoder
{
	symbol_decoder(const symbol_decoder&) = delete;
	symbol_decoder& operator=(const symbol_decoder&) = delete;
public:
	//Constructor
	symbol_decoder()
	{}
	//No char
	static constexpr auto NO_CHAR = -1;
	//Decode symbol from bit
	int operator()( bool bit )
	{
		int ch = NO_CHAR;
		if( (bit==0) && m_last_bit_zero )	//if character delimiter
		{
			if(m_bit_acc != 0 )
			{
				constexpr varicode v;
				m_bit_acc >>= 2;				//get rid of last zero and one
				m_bit_acc &= 0x07FF;
				ch = v.reverse(m_bit_acc);
				m_bit_acc = 0;
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
		return ch;
	}
private:
	bool m_last_bit_zero {};
	uint16_t m_bit_acc {};
};

/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* SYMBOL_DECODER_HPP_ */
