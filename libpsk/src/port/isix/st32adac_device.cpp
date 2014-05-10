/*
 * =====================================================================================
 *
 *       Filename:  st32adac_device.cpp
 *
 *    Description:  STM32ADAC device driver implementation
 *
 *        Version:  1.0
 *        Created:  06.05.2014 21:49:23
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lucjan Bryndza (LB), lucck(at)boff(dot)pl
 *   Organization:  BoFF
 *
 * =====================================================================================
 */
#include "libpsk/port/isix/stm32adac_device.hpp"

/* ------------------------------------------------------------------ */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------ */ 
//! Lock or unlock selected primitive
void stm32adac_device::lock( int id )
{
	if( id == trx_device_base::lock_object ) {
		m_lock.wait( isix::ISIX_TIME_INFINITE );
	} else if ( id == trx_device_base::lock_spectrum ) {
		m_spectrum.wait( isix::ISIX_TIME_INFINITE );
	}
}
/* ------------------------------------------------------------------ */
//! Lock or unlock selected primitive
void stm32adac_device::unlock( int id )
{
	if( id == trx_device_base::lock_object ) {
		m_lock.signal();
	} else if ( id == trx_device_base::lock_spectrum ) {
		m_spectrum.signal();
	}
}
/* ------------------------------------------------------------------ */ 
//Try lock selected primitive
bool stm32adac_device::try_lock( int id )
{
	if( id == trx_device_base::lock_object ) {
		return m_lock.getval();
	} else if ( id == trx_device_base::lock_spectrum ) {
		return m_spectrum.getval();
	}
	return false;
}
/* ------------------------------------------------------------------ */ 
//! Initialize and setup sound hardware
int stm32adac_device::setup_sound_hardware( trx_device_base::mode m )
{
	//Hardware  SETUP state
	if( m == trx_device_base::mode::on && get_mode()!=trx_device_base::mode::on )
	{

		if( get_mode()==trx_device_base::mode::transmit )
		{
			const int ret = disable_hw_tx();
			if( ret != 0 ) return ret;
		}
		const int ret = enable_hw_rx();
		if( ret != 0 ) return ret;
	}
	if( m == trx_device_base::mode::transmit && get_mode()!=trx_device_base::mode::transmit )
	{
		if( get_mode()==trx_device_base::mode::on )
		{
			const int ret = disable_hw_rx();
			if( ret != 0 ) return ret;
		}
		const int ret = enable_hw_tx();
		if( ret != 0 ) return ret;
	}
	//Thread specific switch
	if( get_mode()!=trx_device_base::mode::off && m == trx_device_base::mode::off )
	{
		//Disable and wait for stop
		join();
		return m_thread_status;
	}
	else if( get_mode()==trx_device_base::mode::off && m != trx_device_base::mode::off  )
	{
		//! Restart the thread again
		return  m_start.signal();
	}
	return err_success;
}
/* ------------------------------------------------------------------ */
//! Main thread handler 
void stm32adac_device::main() 
{
	//! Signal when thread finish
	for(;;) {
		m_thread_status = m_start.wait( isix::ISIX_TIME_INFINITE );
		if( m_thread_status < 0 )  break;
		m_thread_status = m_busy.wait( isix::ISIX_TIME_INFINITE );
		if( m_thread_status < 0 ) break;
		int errcode = 0;
		for(;m_start.getval()>0 && !errcode ;) {
			if     ( get_mode()==trx_device_base::mode::on ) 	   errcode = receive_thread();
			else if( get_mode()==trx_device_base::mode::transmit ) errcode = transmit_thread();
		}
		if( get_mode()==trx_device_base::mode::on ) {
			//Disable sound receive
			set_mode_off();
			disable_hw_rx();
		}
		if( get_mode()==trx_device_base::mode::transmit ) {
			//Disable sound transmit
			set_mode_off();
			disable_hw_tx();
		}
		m_thread_status = errcode;
		//! Do something here
		m_busy.signal();
	}
}
/* ------------------------------------------------------------------ */
//! Receive thread
int stm32adac_device::receive_thread()
{
	{
	//	std::lock_guard<std::mutex> lock( m_codec_mutex );
	//	adc_process( &m_audio_buf[0], audio_buf_len );
	}
	return err_success;
}
/* ------------------------------------------------------------------ */ 
int stm32adac_device::transmit_thread() 
{
	{
		//std::lock_guard<std::mutex> lock( m_codec_mutex );
		//status = dac_process( &m_audio_buf[0], audio_buf_len );
	}
	return err_success;
}
/* ------------------------------------------------------------------ */
//! Join and wait for finish trx
int stm32adac_device::join() 
{
	return m_busy.wait( isix::ISIX_TIME_INFINITE );
}
/* ------------------------------------------------------------------ */
//!Disable hardware transmision
int stm32adac_device::disable_hw_tx()
{
	return err_success;
}
/* ------------------------------------------------------------------ */ 
//! Enable hardware transmision
int stm32adac_device::enable_hw_tx()
{
	return err_success;
}
/* ------------------------------------------------------------------ */ 
//! Disable hardware reception
int stm32adac_device::disable_hw_rx()
{
	return err_success;
}
/* ------------------------------------------------------------------ */ 
//! Enable hardware reception 
int stm32adac_device::enable_hw_rx() 
{
	return err_success;
}
/* ------------------------------------------------------------------ */ 
}}
