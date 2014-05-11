/*
 * =====================================================================================
 *
 *       Filename:  stm32adac_device.hpp
 *
 *    Description:  STM32 AC/CA implementation
 *
 *        Version:  1.0
 *        Created:  06.05.2014 21:18:24
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Lucjan Bryndza (LB), lucck(at)boff(dot)pl
 *   Organization:  BoFF
 *
 * =====================================================================================
 */

#pragma once
/* ------------------------------------------------------------------ */ 
#include "libpsk/codec/trx_device_base.hpp"
#include <board/adc_audio.hpp>
#include <board/dac_audio.hpp>
#include <isix.h>
/* ------------------------------------------------------------------ */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------ */ 
//STM32 AD/AC device
class stm32adac_device : public trx_device_base, isix::task_base {
public:
	static constexpr auto err_success = 0;
	//! Constructor
	stm32adac_device( handler_t evt_callback ) 
		: trx_device_base( evt_callback ) {
	}
	//! Destructor
	virtual ~stm32adac_device() {
	}
protected:
	//Lock the device
	virtual void lock( int id );
	virtual void unlock( int id );
	virtual bool try_lock( int id );
	virtual int join();
	// Initialize sound hardware in selected mode
	virtual int setup_sound_hardware( trx_device_base::mode m );
	//! Thread func for isix base
	virtual void main();
private:
	//Hardware funcs
	int enable_hw_rx() {
		return m_adc_audio.record( get_tx_sample_rate() );
	}
	int disable_hw_rx() {
		return m_adc_audio.stop();
	}
	int enable_hw_tx() {
		return m_dac_audio.play( get_tx_sample_rate() );
	}
	int disable_hw_tx() {
		return m_dac_audio.stop();
	}
	int receive_thread();
	int transmit_thread();
private:
	static constexpr auto sample_size = 256U;		//! Sample buffer size
	static constexpr auto sample_nbufs = 10U;		//! Number of sample buffs
	drv::dac_audio m_dac_audio { sample_size, sample_nbufs };		//!ADC audio DEVICE
	drv::adc_audio m_adc_audio { sample_size, sample_nbufs  };		//!DAC audio DEVICE
	isix::semaphore m_lock { 1, 1 };								//! Global lock
	isix::semaphore m_spectrum { 1, 1 };							//! Spectrum lock
	isix::semaphore m_busy { 1, 1 };								//! Busy notificator
	isix::semaphore m_start { 0, 1 };								//! Start thread
	int m_thread_status {};											//! Thread error code
};
/* ------------------------------------------------------------------ */
}
}
