/*
 * modulator_codes.hpp
 *
 *  Created on: 09-05-2013
 *      Author: lucck
 */

#ifndef PSK_MODULATOR_CODES_HPP_
#define PSK_MODULATOR_CODES_HPP_


namespace ham {
namespace psk {
namespace tx {

	constexpr auto TXOFF_CODE = -1;			// control codes that can be placed in the input
	constexpr auto TXON_CODE  = -2;			// queue for various control functions
	constexpr auto TXTOG_CODE = -3;

}}}



#endif /* MODULATOR_CODES_HPP_ */
