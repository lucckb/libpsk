/*
 * symbol_encoder.hpp
 *
 *  Created on: 09-05-2013
 *      Author: lucck
 */

#ifndef PSK_SYMBOL_ENCODER_HPP_
#define PSK_SYMBOL_ENCODER_HPP_

/* ------------------------------------------------------------------------- */
#include "libpsk/psk/control_chars.hpp"
#include "libpsk/psk/varicode.hpp"
#include "libpsk/codec/psk_config.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
namespace _internal {

/* ------------------------------------------------------------------------- */
class symbol_encoder
{
	//Make object noncopyable
	symbol_encoder(const symbol_encoder&) = delete;
	symbol_encoder& operator=(const symbol_encoder&) = delete;
	typedef mod_psk_config::mode mode;
public:
	enum sym
	{
		SYM_NOCHANGE,	//Stay the same phase
		SYM_P90,		//Plus 90  deg
		SYM_P180,		//Plus 180 deg
		SYM_M90,		//Minus 90 deg
		SYM_OFF,		//No output
		SYM_ON			//constant output
	};
	//Constructor
	symbol_encoder()
	{}
	//Set encoder mode
	void set_mode( mode m ) {
		m_mode = m;
	}
	//Get encoder mode
	mode get_mode() const {
		return m_mode;
	}
	//Get next symbol
	sym operator()( short ch )
	{
		switch( m_mode )				//get next symbol to send
		{
			case mode::bpsk:
				return get_next_bpsk_symbol( ch );
			case mode::qpsku:
				return get_next_qpsk_symbol( ch );
			case mode::qpskl:
			{
				sym symbol = get_next_qpsk_symbol( ch );
				//rotate vectors the opposite way
				if(symbol==SYM_P90) symbol = SYM_M90;
				else if(symbol==SYM_M90) symbol = SYM_P90;
				return symbol;
			}
			case mode::tune:
				return get_next_tune_symbol( ch );
		}
		return SYM_OFF;
	}
	//Is end of symbol
	bool eos() const
	{
		if( m_mode == mode::bpsk )
			return (m_tx_shift_reg == 0) && !m_add_ending_zero;
		else if( m_mode == mode::qpskl || m_mode == mode::qpsku )
			return (m_tx_code_word == 0) && !m_add_ending_zero;
		else
			return true;
	}
private:
	static constexpr unsigned char ConvolutionCodeTable[32] =
	{
		2, 1, 3, 0, 3, 0, 2, 1,
		0, 3, 1, 2, 1, 2, 0, 3,
		1, 2, 0, 3, 0, 3, 1, 2,
		3, 0, 2, 1, 2, 1, 3, 0
	};

	sym get_next_bpsk_symbol( short ch )
	{
		sym symb = m_last_symb;
		if( m_tx_shift_reg == 0 )
		{
			if( m_add_ending_zero )		// if is end of code
			{
				symb = SYM_P180;		// end with a zero
				m_add_ending_zero = false;
			}
			else
			{
				//ch = get_char();			//get next character to xmit
				if( ch >=0 )			//if is not a control code
				{						//get next VARICODE codeword to send
					_internal::varicode varicode;
					m_tx_shift_reg = varicode.forward( ch );
					symb = SYM_P180;	//Start with a zero
				}
				else					// is a control code
				{
					switch( ch )
					{
					case ctrl_chars::TXON_CODE: symb = SYM_ON;    break;
					case ctrl_chars::TXTOG_CODE: symb = SYM_P180; break;
					case ctrl_chars::TXOFF_CODE: symb = SYM_OFF;  break;
					}
				}
			}
		}
		else			// is not end of code word so send next bit
		{
			if( m_tx_shift_reg&0x8000 )
				symb = SYM_NOCHANGE;
			else
				symb = SYM_P180;
			m_tx_shift_reg = m_tx_shift_reg<<1;	//point to next bit
			if( m_tx_shift_reg == 0 )			// if at end of codeword
				m_add_ending_zero = true;		// need to send a zero nextime
		}
		m_last_symb = symb;
		return symb;
	}
	sym get_next_qpsk_symbol( short ch )
	{
		sym symb = (sym)ConvolutionCodeTable[m_tx_shift_reg&0x1F];	//get next convolution code
		m_tx_shift_reg = m_tx_shift_reg<<1;
		if( m_tx_code_word == 0 )			//need to get next codeword
		{
			if( m_add_ending_zero )		//if need to add a zero
			{
				m_add_ending_zero = false;	//end with a zero
			}
			else
			{
				//ch = get_char();			//get next character to xmit
				if( ch >=0 )			//if not a control code
				{						//get next VARICODE codeword to send
					_internal::varicode varicode;
					m_tx_code_word = varicode.forward( ch );
				}
				else					//is a control code
				{
					switch( ch )
					{
					case ctrl_chars::TXON_CODE: symb = SYM_ON; 	 		break;
					case ctrl_chars::TXTOG_CODE: m_tx_code_word = 0; 	break;
					case ctrl_chars::TXOFF_CODE: symb = SYM_OFF; 		break;
					}
				}
			}
		}
		else
		{
			if(m_tx_code_word&0x8000 )
			{
				m_tx_shift_reg |= 1;
			}
			m_tx_code_word = m_tx_code_word<<1;
			if(m_tx_code_word == 0)
				m_add_ending_zero = true;	//need to add another zero
		}
		return symb;
	}
	sym get_next_tune_symbol( short ch )
	{
		sym symb;
		//const int ch = get_char();			//get next character to xmit
		return ( ch == ctrl_chars::TXON_CODE )?(SYM_ON):(SYM_OFF);
		return symb;
	}
private:
	mode m_mode { mode::bpsk };
	sym m_last_symb { SYM_OFF };
	short m_tx_shift_reg {};
	bool  m_add_ending_zero { true };
	short m_tx_code_word {};
};
/* ------------------------------------------------------------------------- */
}}}

/* ------------------------------------------------------------------------- */
#endif /* SYMBOL_ENCODER_HPP_ */
/* ------------------------------------------------------------------------- */
