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
	TXOFF_CODE = -1,
	TXON_CODE  = -2,
	TXTOG_CODE = -3,
	TX_CNTRL_AUTOSTOP = -4,
	TX_CNTRL_NOSQTAIL = -5
};
}

// Definition of the character type
typedef short txchar_type;


}}


#endif /* MODULATOR_CODES_HPP_ */
