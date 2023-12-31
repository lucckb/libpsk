/*
 * pulse_device.hpp
 *
 *  Created on: 23-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef PORT_PULSE_DEVICE_HPP_
#define PORT_PULSE_DEVICE_HPP_
/* ------------------------------------------------------------------------- */

#include "libpsk/codec/trx_device_base.hpp"
#include <thread>
#include <memory>
#include <mutex>
#include <array>
#include <pulse/simple.h>

/* ------------------------------------------------------------------------- */

namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Pulse audio device port
class pulse_device: public trx_device_base
{
public:
	//Constructor
	pulse_device( handler_t evt_callback, const char *name = nullptr )
		:  trx_device_base(evt_callback), m_dev_name( name )
	{
	}
	virtual ~pulse_device()
	{
		if( m_pa_ctx ) pa_simple_free( m_pa_ctx );
	}
	//Sleep for terminate audio thread
	virtual int join( )
	{
		if( m_thread )
			m_thread->join();
		return m_thread_status;
	}
protected:
	//Lock the device
	virtual void lock( int id );
	virtual void unlock( int id );
	virtual bool try_lock( int id );
	// Initialize sound hardware in selected mode
	virtual int setup_sound_hardware( trx_device_base::mode m );
private:
	//Hardware sound thread func
	void hardware_sound_thread();
	//Receive and transmit thread
	int receive_thread();
	//Transmit thread
	int transmit_thread();
	//Hardware funcs
	int enable_hw_rx();
	int disable_hw_rx();
	int enable_hw_tx();
	int disable_hw_tx();
private:
	const char *m_dev_name;							//Device name
	std::unique_ptr< std::thread > m_thread;		//TX thread
	std::mutex m_codec_mutex;						//Thread mutex
	std::mutex m_sectrum_mutex;						//Spectrum mutex
	std::mutex m_pulse_mutex;
	volatile bool m_thread_running {};				//Thread
	volatile int  m_thread_status {};				//Thread error status
	pa_simple *m_pa_ctx {nullptr};					//PA CTX
	static constexpr auto audio_buf_len = 1024;
	std::array<short,audio_buf_len> m_audio_buf;
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */
#endif /* PULSE_DEVICE_HPP_ */
