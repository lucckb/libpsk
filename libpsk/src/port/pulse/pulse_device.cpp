/*
 * pulse_device.cpp
 *
 *  Created on: 23-05-2013
 *      Author: lucck
 */

#include "port/pulse/pulse_device.hpp"
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
	/* Pulse receive config */
	constexpr pa_sample_spec pulse_rx_config
	{
		PA_SAMPLE_S16LE,	//Format
		8000,				//Rate
		1					//Channels
	};
	/* Pulse receive config */
	constexpr pa_sample_spec pulse_tx_config
	{
		PA_SAMPLE_S16LE,	//Format
		8000,				//Rate
		1					//Channels
	};
	const char C_pa_name[] = "PSK BoFF HAMLIB";
}
/* ------------------------------------------------------------------------- */
//Lock the device
void pulse_device::lock( bool lock )
{
	if( lock )
		m_thrmutex.lock();
	else
		m_thrmutex.unlock();
}
/* ------------------------------------------------------------------------- */
// Initialize sound hardware in selected mode
int pulse_device::setup_sound_hardware( trx_device_base::mode m )
{

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
	if( get_mode()!=trx_device_base::mode::off && m == trx_device_base::mode::off )
	{
		//Disable and wait for stop
		m_thread_running = false;
		m_thread->join();
	}
	else if( get_mode()==trx_device_base::mode::off && m != trx_device_base::mode::off  )
	{
		m_thread_running = true;
		m_thread.reset( new std::thread( &pulse_device::hardware_sound_thread, this ) );
	}
	return 0;
}
/* ------------------------------------------------------------------------- */
//Hardware sound thread func
void pulse_device::hardware_sound_thread()
{
	for(;m_thread_running;)
	{
		m_thrmutex.lock();
		if     ( get_mode()==trx_device_base::mode::on ) 	   receive_thread();
		else if( get_mode()==trx_device_base::mode::transmit ) transmit_thread();
		m_thrmutex.unlock();
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
}
/* ------------------------------------------------------------------------- */
//Receive and transmit thread
bool pulse_device::receive_thread()
{
	int error = 0;
	/* Record some data ... */
	if (pa_simple_read(m_pa_ctx, &m_audio_buf[0], audio_buf_len*sizeof(short), &error) < 0)
	{
	    return error;
	}
	adc_hardware_isr( &m_audio_buf[0], audio_buf_len );
	return error;
}
/* ------------------------------------------------------------------------- */
//Transmit thread
bool pulse_device::transmit_thread()
{
	int error = 0;
	dac_hardware_isr( &m_audio_buf[0], audio_buf_len  );
	if (pa_simple_write(m_pa_ctx,&m_audio_buf[0], audio_buf_len*sizeof(short), &error) < 0)
	{
		return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Enable hardware receive
int pulse_device::enable_hw_rx()
{
	int error = 0;
	if (!(m_pa_ctx = pa_simple_new(nullptr, C_pa_name, PA_STREAM_RECORD, nullptr, "record", &pulse_tx_config, nullptr, nullptr, &error)))
	{
		return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Disable hardware receive
int pulse_device::disable_hw_rx()
{
	pa_simple_free(m_pa_ctx);
	m_pa_ctx = nullptr;
	return 0;
}
/* ------------------------------------------------------------------------- */
//Enable hardware trasmit
int pulse_device::enable_hw_tx()
{
	int error = 0;
	/* Create a new playback stream */
	if (!(m_pa_ctx = pa_simple_new(nullptr, C_pa_name, PA_STREAM_PLAYBACK, nullptr, "playback", &pulse_rx_config, nullptr, nullptr, &error)))
	{
	    return error;
	}
	return error;
}
/* ------------------------------------------------------------------------- */
//Disable hardware tramsmit
int pulse_device::disable_hw_tx()
{
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
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
