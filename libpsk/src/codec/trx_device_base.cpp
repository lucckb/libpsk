/*
 * trx_device_base.cpp
 *
 *  Created on: 22-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#include "codec/trx_device_base.hpp"

/* ------------------------------------------------------------------------- */
//Namespace def
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
// Destroy RX codec and remove from pool
bool trx_device_base::destroy_rx_codec( int idx )
{
	if( idx == ALL)
	{
		for( auto v : m_rx_codecs )
		{
			delete v;
		}
	}
	else if( idx < MAX_CODECS )
	{
		if( m_rx_codecs[idx] )
		{
			delete m_rx_codecs[idx];
		}
		else
		{
			return true;
		}
	}
	else
	{
		return true; //FAIL
	}
	return false; //OK
}
/* ------------------------------------------------------------------------- */
//Register RX codec
int trx_device_base::add_rx_codec( rx_codec * codec )
{
	for( auto idx = 0; idx<MAX_CODECS; idx++ )
	{
		if( m_rx_codecs[idx] == nullptr )
		{
			m_rx_codecs[idx] = codec;
			return idx;
		}
	}
	return INVALID;
}

/* ------------------------------------------------------------------------- */
/* Set RX or TX dev mode */
void trx_device_base::set_mode( trx_device_base::mode m )
{
	//Safe lock
	safe_lock<decltype (*this)> lock(*this);
	setup_sound_hardware( m );
	m_mode = m;
}
/* ------------------------------------------------------------------------- */
//ADC vector func
void trx_device_base::adc_process( const sample_type *buf, size_t len )
{
	for( auto v : m_rx_codecs )
	{
		if( v != nullptr )
			(*v)( buf, len );
	}
}
/* ------------------------------------------------------------------------- */
//DAC vector func return true if finished
bool trx_device_base::dac_process( sample_type *buf, size_t len )
{
	if ( (*m_tx_codec)( buf, len ) )
	{
		m_mode = mode::on;
		return true;
	}
	return false;
}
/* ------------------------------------------------------------------------- */
}}	//NS end
/* ------------------------------------------------------------------------- */


