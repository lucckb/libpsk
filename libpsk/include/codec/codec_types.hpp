/*
 * psk_types.hpp
 *
 *  Created on: 20-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_PSK_TYPES_HPP_
#define LIBPSK_PSK_TYPES_HPP_

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------- */
//Sample type variable
typedef int16_t sample_type;

/* ------------------------------------------------------------------------- */
//Sample rate type
typedef short samplerate_type;

/* ------------------------------------------------------------------------- */
//Squelch tresh type
typedef int sqelch_value_type;
//Squelch mode
enum class squelch_mode
{
	slow,
	fast
};

/* ------------------------------------------------------------------------- */
}}




#endif /* PSK_TYPES_HPP_ */
