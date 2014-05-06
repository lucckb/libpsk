/*
 * ham_digi.cpp
 *
 *  Created on: 28-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "libpsk/ham/ham_digi.hpp"
#ifndef COMPILED_UNDER_ISIX
#include "libpsk/port/pulse/pulse_device.hpp"
#else
#warning not implemented yet
#endif
#include "libpsk/psk/decoder.hpp"
#include "libpsk/psk/modulator.hpp"
#include <functional>
#include <new>
#include <cstdlib>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------- */
namespace {
	inline void terminate() {
#ifdef __EXCEPTIONS
		throw std::bad_alloc();
#else
		std::abort();
#endif
	}
#ifndef COMPILED_UNDER_ISIX
	inline trx_device_base* create_default_device( ham_digi::handler_t h )
	{
		static const int sys_idx = ham_digi::SYS_CALLBACK_ID;
		return new pulse_device( std::bind(h, sys_idx, std::placeholders::_1) );
	}
#else
	inline trx_device_base* create_default_device( ham_digi::handler_t h ) {
		terminate();
		return nullptr;
	}
#endif
}
/* ------------------------------------------------------------------------- */
//Default ham digi constructor
ham_digi::ham_digi( handler_t handler )
	:  m_iodev( create_default_device( handler ) )
	,  m_callback( handler )
{
}
/* ------------------------------------------------------------------------- */
/* Activate transmission */
int ham_digi::enable( bool en )
{
	if( !m_iodev->get_tx_codec() || !m_iodev->get_tx_codec() )
	{
		return err_no_modulation;
	}
	if( en )
	{
		m_iodev->set_mode( trx_device_base::mode::on );
	}
	else
	{
		m_iodev->set_mode( trx_device_base::mode::off );
	}
	return err_ok;
}
/* ------------------------------------------------------------------------- */
/* Switch to TX RX */
int ham_digi::set_tx( bool tx )
{
	if( !m_iodev->get_tx_codec() || !m_iodev->get_tx_codec() )
	{
		return err_no_modulation;
	}
	/* Not working yet */
	//if( m_iodev->get_tx_codec()->is_transmitting()  )
	//{
	//	return err_tx_busy;
	//}
	if( tx && m_iodev->get_mode()==trx_device_base::mode::on )
	{

		m_iodev->set_mode( trx_device_base::mode::transmit );
	}
	else
	{
		return err_invalid_request;
	}
	return err_ok;
}
/* ------------------------------------------------------------------------- */
/* Set current modulation currenty PSK only is supported */
int ham_digi::set_modulation( modulation mod )
{
	if( mod == m_modulation )
	{
		return err_ok;
	}
	//Change modulation
	if( m_iodev->remove_tx_codec() )
	{
		return err_remove_codec;
	}
	if( m_iodev->remove_rx_codec() )
	{
		return err_remove_codec;
	}
	if( mod == modulation::psk )
	{
	  static const int sys_idx = SYS_CALLBACK_ID;
		m_iodev->add_tx_codec(
	     new modulator( m_iodev->get_tx_sample_rate(), DEF_FREQ, TX_QUELEN,
	    	std::bind( m_callback, sys_idx, std::placeholders::_1 ) )
	  );
	  const int idx =  m_iodev->get_rx_slot();
	  if( idx != trx_device_base::INVALID )
	  {
		m_iodev->add_rx_codec(
			new decoder( m_iodev->get_rx_sample_rate(),
			std::bind( m_callback, idx, std::placeholders::_1 ) ), idx );
	  }
	  else
	  {
		  return err_nofree_slots;
	  }
	}
	m_modulation = mod;
	return err_ok;
}
/* ------------------------------------------------------------------------- */
/* New RX channel */
int ham_digi::rx_channel_add()
{
	if( !m_iodev->get_tx_codec() || !m_iodev->get_tx_codec() )
	{
		return err_no_modulation;
	}
 	const int idx =  m_iodev->get_rx_slot();
	if( idx != trx_device_base::INVALID )
	{
		m_iodev->add_rx_codec(
				new decoder( m_iodev->get_rx_sample_rate(),
			std::bind( m_callback, idx, std::placeholders::_1 ) ), idx );
	}
	else
	{
		 return err_nofree_slots;
	}
	return err_ok;
}

/* ------------------------------------------------------------------------- */
/* Extra channel remove */
int ham_digi::rx_channel_remove( int chn_id )
{
	if( !m_iodev->get_tx_codec() || !m_iodev->get_tx_codec() )
	{
		return err_no_modulation;
	}
	if( chn_id == 0 )
	{
		return err_invalid_request;
	}
	if( m_iodev->remove_rx_codec( chn_id ) )
	{
		return err_invalid_slot;
	}
	return err_ok;
}
/* ------------------------------------------------------------------------- */
//Get locked TX
ham_digi::tx_proxy&& ham_digi::tx()
{
	return std::move( tx_proxy( m_iodev.get() ) );
}
/* ------------------------------------------------------------------------- */
//Locked access RX
ham_digi::rx_proxy&& ham_digi::rx( int id )
{
	return std::move( rx_proxy( m_iodev.get(), id) );
}
/* ------------------------------------------------------------------------- */
//Get spectrum
ham_digi::spectrum_proxy&& ham_digi::get_spectrum()
{
	return std::move( spectrum_proxy( m_iodev.get()) );
}

/* ------------------------------------------------------------------------- */
}}	//Namespace end

