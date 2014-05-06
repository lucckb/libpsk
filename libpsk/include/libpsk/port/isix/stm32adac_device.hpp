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
/* ------------------------------------------------------------------ */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------ */ 
class stm32adac_device : public trx_device_base {
public:
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
};
/* ------------------------------------------------------------------ */
}
}
