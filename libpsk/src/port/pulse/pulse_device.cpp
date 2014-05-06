/*
 * pulse_device.cpp
 *
 *  Created on: 23-05-2013
 *      Author: lucck
 */

#include "libpsk/port/pulse/pulse_device.hpp"
#include <pulse/simple.h>
#include <pulse/error.h>
#include <iostream>
using namespace std;
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {


/* ------------------------------------------------------------------------- */
namespace
{
	const char C_pa_name[] = "PSK BoFF HAMLIB";
}

/* ------------------------------------------------------------------------- */
// Initialize sound hardware in selected mode
int pulse_device::setup_sound_hardware( trx_device_base::mode m )
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
		m_thread_running = false;
		m_thread->join();
		return m_thread_status;
	}
	else if( get_mode()==trx_device_base::mode::off && m != trx_device_base::mode::off  )
	{
		m_thread_running = true;
		m_thread.reset( new std::thread( &pulse_device::hardware_sound_thread, this ) );
		return 0;
	}
	return 0;
}
/* ------------------------------------------------------------------------- */
//Hardware sound thread func
void pulse_device::hardware_sound_thread()
{
	int errcode = 0;
	for(; m_thread_running && !errcode ;)
	{

		if     ( get_mode()==trx_device_base::mode::on ) 	   errcode = receive_thread();
		else if( get_mode()==trx_device_base::mode::transmit ) errcode = transmit_thread();
	}
	if( get_mode()==trx_device_base::mode::on )
	{
		//Disable sound receive
		set_mode_off();
		disable_hw_rx();
	}
	if( get_mode()==trx_device_base::mode::transmit )
	{
		//Disable sound transmit
		set_mode_off();
		disable_hw_tx();
	}
	m_thread_status = errcode;
	m_thread_running = false;
}
/* ------------------------------------------------------------------------- */
//Receive and transmit thread
int pulse_device::receive_thread()
{
	int error = 0;
	{
		std::lock_guard<std::mutex> lock( m_pulse_mutex );
		/* Record some data ... */
		if (pa_simple_read(m_pa_ctx, &m_audio_buf[0], audio_buf_len*sizeof(short), &error) < 0)
		{
			return error;
		}
	}
	{
		std::lock_guard<std::mutex> lock( m_codec_mutex );
		adc_process( &m_audio_buf[0], audio_buf_len );
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Transmit thread
int pulse_device::transmit_thread()
{
	int error = 0;
	bool status;
	{
		std::lock_guard<std::mutex> lock( m_codec_mutex );
		status = dac_process( &m_audio_buf[0], audio_buf_len );
	}
	if( !status )
	{
		std::lock_guard<std::mutex> lock( m_pulse_mutex );
		if (pa_simple_write(m_pa_ctx,&m_audio_buf[0], audio_buf_len*sizeof(short), &error) < 0)
		{
			return error;
		}
	}
	else
	{
		if( (error = disable_hw_tx()) )
			return error;
		if( (error = enable_hw_rx()) )
			return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Enable hardware receive
int pulse_device::enable_hw_rx()
{
	std::lock_guard<std::mutex> lock( m_pulse_mutex );
	/* Pulse receive config */
	const pa_sample_spec pconfig
	{
		PA_SAMPLE_S16LE,		//Format
		get_rx_sample_rate(),	//Rate
		1						//Channels
	};
	int error = 0;
	if (!(m_pa_ctx = pa_simple_new(nullptr, C_pa_name, PA_STREAM_RECORD, nullptr, "record", &pconfig, nullptr, nullptr, &error)))
	{
		return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Disable hardware receive
int pulse_device::disable_hw_rx()
{
	std::lock_guard<std::mutex> lock( m_pulse_mutex );
	pa_simple_free(m_pa_ctx);
	m_pa_ctx = nullptr;
	return 0;
}
/* ------------------------------------------------------------------------- */
//Enable hardware trasmit
int pulse_device::enable_hw_tx()
{
	std::lock_guard<std::mutex> lock( m_pulse_mutex );
	/* Pulse receive config */
	const pa_sample_spec pconfig
	{
		PA_SAMPLE_S16LE,		//Format
		get_tx_sample_rate(),	//Transmit oversample for better Antialias filter
		1						//Channels
	};
	int error = 0;
	/* Create a new playback stream */
	if (!(m_pa_ctx = pa_simple_new(nullptr, C_pa_name, PA_STREAM_PLAYBACK, nullptr, "playback", &pconfig, nullptr, nullptr, &error)))
	{
	    return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Disable hardware tramsmit
int pulse_device::disable_hw_tx()
{
	std::lock_guard<std::mutex> lock( m_pulse_mutex );
	int error = 0;
	/* Make sure that every single sample was played */
	if (pa_simple_drain(m_pa_ctx, &error) < 0)
	{
		return error;
	}
	pa_simple_free(m_pa_ctx);
	m_pa_ctx = nullptr;
	return error;
}
/* ------------------------------------------------------------------------- */
//Lock the device
void pulse_device::lock( int id )
{
	if( id == trx_device_base::lock_object )
		m_codec_mutex.lock();
	else if( id == trx_device_base::lock_spectrum )
		m_sectrum_mutex.lock();
}
/* ------------------------------------------------------------------------- */
void pulse_device::unlock( int id )
{
	if( id == trx_device_base::lock_object )
		m_codec_mutex.unlock();
	else if( id == trx_device_base::lock_spectrum )
		m_sectrum_mutex.unlock();
}
/* ------------------------------------------------------------------------- */
bool pulse_device::try_lock( int id )
{
	if( id == trx_device_base::lock_object )
		return m_codec_mutex.try_lock();
	else if( id == trx_device_base::lock_spectrum )
		return m_sectrum_mutex.try_lock();
	return false;
}
/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
