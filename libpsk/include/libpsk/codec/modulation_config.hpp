/*
 * modulation_config.hpp
 *
 *  Created on: 26-05-2013
 *      Author: lucck
 */

#ifndef CODEC_MODULATION_CONFIG_HPP_
#define CODEC_MODULATION_CONFIG_HPP_

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------ */
class modulation_config_base
{
public:
	enum class type : short
	{
		psk
	};
	type cfg_type() const
	{
		return m_type;
	}
protected:
	modulation_config_base( type _type )
		: m_type(_type)
	{
	}
private:
	type m_type {};
};

/* ------------------------------------------------------------------------ */
}
}

/* ------------------------------------------------------------------------ */
#endif /* MODULATION_CONFIG_HPP_ */
