/*
 * modulator_codes.hpp
 *
 *  Created on: 09-05-2013
 *      Author: lucck
 */

#ifndef PSK_CONTROL_CHARS_HPP_
#define PSK_CONTROL_CHARS_HPP_


namespace ham {
namespace psk {

// Special control codes
namespace ctrl_chars {
enum ctrl_chars	: short
{
	TXOFF_CODE = 0xFF,
	TXON_CODE  = 0xFE,
	TXTOG_CODE = 0xFD,
	TX_CNTRL_AUTOSTOP = 0xFC,
	TX_CNTRL_NOSQTAIL = 0xFB,
	TX_CNTRL_END_CODES = 0xF0
};
}

// Definition of the character type
typedef short txchar_type;


}}


#endif /* MODULATOR_CODES_HPP_ */
