/*
 * psk_config.hpp
 *
 *  Created on: 26-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef CODEC_PSK_CONFIG_HPP_
#define CODEC_PSK_CONFIG_HPP_
/* ------------------------------------------------------------------------- */
#include "modulation_config.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------ */
/* PSK Modulation config */
struct mod_psk_config : public modulation_config_base
{
	enum class mode : short {
		bpsk,
		qpsku,
		qpskl,
		tune	//Valid only in transmission
	};
	enum class baud	: short {
		b31,				//Baudrate 31
		b63,				//Baudrate 63
		b125				//Baudrate 125
	};
	mod_psk_config( mode _mode = mode::bpsk, baud _baudrate = baud::b31 )
		: modulation_config_base( modulation_config_base::type::psk ),
		  mmode(_mode), baudrate(_baudrate)
	{
	}
	mode mmode {};
	baud baudrate {};
};


/* ------------------------------------------------------------------------- */
}}

/* ------------------------------------------------------------------------- */
#endif /* CODEC_PSK_CONFIG_HPP_ */
