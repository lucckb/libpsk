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
#include <foundation/dbglog.h>
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
		dbprintf("Do receive processing");
	}
	if( m == trx_device_base::mode::transmit && get_mode()!=trx_device_base::mode::transmit )
	{
		dbprintf("Do transmit processing");
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
		m_thread_cont = false;
		join();
		dbprintf("Thread exited - terminate");
		return m_thread_status;
	}
	else if( get_mode()==trx_device_base::mode::off && m != trx_device_base::mode::off  )
	{
		//! Restart the thread again
		dbprintf("Thread wakeup request");
		m_thread_cont = true;
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
		dbprintf("Start procesing PSK");
		for(; m_thread_cont && !errcode ;) {
			lock( trx_device_base::lock_object );
			if     ( get_mode()==trx_device_base::mode::on ) 	   errcode = receive_thread();
			else if( get_mode()==trx_device_base::mode::transmit ) errcode = transmit_thread();
			else {
				dbprintf("Unknown mode %i!", get_mode() );
				break;
			}
			unlock( trx_device_base::lock_object );
		}
		dbprintf("Stop procesing PSK reason getval: %i error: %i", m_start.getval(), errcode );
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
	//FIXME: Fix it later ( Pointers failure )
	const auto ptr = reinterpret_cast<int16_t*>( m_adc_audio.get_sample_buffer() );
	if( ptr == nullptr ) {
		return m_adc_audio.errno();
	}
	adc_process( ptr, sample_size );
	//dbprintf("RX process ptr %p", ptr );
	return m_adc_audio.commit_buffer( reinterpret_cast<uint16_t*>(ptr) );
}
/* ------------------------------------------------------------------ */ 
int stm32adac_device::transmit_thread() 
{
	//FIXME: Pointer matching
	auto buf = m_dac_audio.reserve_buffer();
	if( !buf ) {
		return m_dac_audio.errno();
	}
	auto status = dac_process( reinterpret_cast<int16_t*>(buf), sample_size );
	if( status == 0 ) {
		for( size_t i = 0; i < sample_size; ++i ) {
			reinterpret_cast<uint16_t*>(buf)[i] =  int( reinterpret_cast<int16_t*>(buf)[i] ) + 32767;
		}
	}
//	memcpy( buf, m_tbuf, sizeof m_tbuf );
	//dbprintf("TX process ptr %p", buf );
	auto cstatus = m_dac_audio.commit_buffer( buf );
	if( cstatus) return cstatus;
	if( status) return status;
	return err_success;
}
/* ------------------------------------------------------------------ */
//! Join and wait for finish trx
int stm32adac_device::join() 
{
	return m_busy.wait( isix::ISIX_TIME_INFINITE );
}
/* ------------------------------------------------------------------ */ 
}}
