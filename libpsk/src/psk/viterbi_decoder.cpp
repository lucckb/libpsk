/*
 * viterbi_decoder.cpp
 *
 *  Created on: 04-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#include "psk/viterbi_decoder.hpp"

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
namespace _internal {
/* ------------------------------------------------------------------------- */
//Half table symbols
constexpr int viterbi_decoder::ANGLE_TBL1[];
constexpr int viterbi_decoder::ANGLE_TBL2[];
constexpr unsigned char viterbi_decoder::ConvolutionCodeTable[];
/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */


