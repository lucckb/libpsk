/*
 * ham_digi.hpp
 *
 *  Created on: 27-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#pragma once
/* ------------------------------------------------------------------------- */
#include "libpsk/codec/trx_device_base.hpp"
#include <memory>
/* ------------------------------------------------------------------------- */

namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Object noncopyable
class ham_digi {
	ham_digi(const ham_digi&) = delete;
	ham_digi& operator=(const ham_digi&) = delete;
	static constexpr auto TX_QUELEN = 512;
public:
	static constexpr auto SYS_CALLBACK_ID = -1;
	static constexpr auto DEF_FREQ = 1000;
	/* Modulation structure */
	enum class modulation : short {
		notset,
		psk			//PSK modulaton
	};
	/* Error codes API */
	enum err {
		err_ok ,			//No error
		err_no_modulation= -4096,	//Modulation no set
		err_tx_busy= -4997,			//Device is transmit busy
		err_invalid_request= -4098,	//Invalid request
		err_remove_codec= -4099,		//Remove codec invalid request
		err_nofree_slots= -4100,		//No free slots
		err_invalid_slot= -4101		//Invalid slot
	};

	/* Function event handler type
	 * first parameter channel number
	 * second channel type
	 */
	typedef std::function <void( int chn, const event &ev )> handler_t;
	/* Constructor with handler */
	ham_digi( handler_t handler );
	/* Activate transmission */
	int enable( bool en );
	/* Switch to TX RX */
	int set_tx( bool tx );
	/* Set current modulation currenty PSK only is supported */
	int set_modulation( modulation mod );
	/* New RX channel */
	int rx_channel_add();						/* Extra channel ADD */
	int rx_channel_remove( int chn_id );		/* Extra channel remove */
	/* Wait to finish */
	int join() {
		return m_iodev->join();
	}
	/** Get device base status */
	using dev_mode = trx_device_base::mode;
	dev_mode get_mode() const {
		return m_iodev->get_mode();
	}
public:
	class tx_proxy {
		tx_proxy(const tx_proxy&) = delete;
		tx_proxy& operator=(const tx_proxy&) = delete;
	public:
		tx_proxy( tx_proxy&& other ) 
			: m_obj( other.m_obj ) {
				m_obj = nullptr;
			}
		tx_proxy( trx_device_base *obj )
		: m_obj(obj) {
			if( m_obj ) m_obj->lock();
		}
		~tx_proxy() {
			if( m_obj ) m_obj->unlock();
		}
		tx_codec* operator->() {
			return m_obj->get_tx_codec();
		}
		tx_codec const* operator->() const {
			return m_obj->get_tx_codec();
		}
	private:
		trx_device_base* m_obj {};
	};
	class rx_proxy {
		rx_proxy(const rx_proxy&) = delete;
		rx_proxy& operator=(const rx_proxy&) = delete;
	public:
		static constexpr auto invalid_id = -1;
		rx_proxy( rx_proxy&& other ) 
			: m_obj( other.m_obj ), m_id( other.m_id )
		{
			m_obj = nullptr;
			m_id = invalid_id;
		}
		rx_proxy( trx_device_base *obj, int id )
			: m_obj(obj), m_id(id) {
			if( m_obj ) m_obj->lock();
		}
		~rx_proxy() {
			if( m_obj ) m_obj->unlock();
		}
		rx_codec* operator->() {
			return m_obj->get_rx_codec( m_id );
		}
		rx_codec const* operator->() const {
			return m_obj->get_rx_codec( m_id );
		}
	private:
		trx_device_base*  m_obj {};
		int m_id;
	};
public:
	class spectrum_proxy
	{
		spectrum_proxy(const spectrum_proxy&) = delete;
		spectrum_proxy& operator=(const spectrum_proxy&) = delete;
	public:
		//! Move constructor
		spectrum_proxy( spectrum_proxy&& other )
			: m_obj( other.m_obj ) {
			m_obj = nullptr;
		}
		spectrum_proxy( trx_device_base *obj)
			: m_obj(obj) {
			if( m_obj ) m_obj->lock( trx_device_base::lock_spectrum );
		}
		~spectrum_proxy() {
			if( m_obj ) m_obj->unlock( trx_device_base::lock_spectrum );
		}
		spectrum_calculator& get() const {
			return m_obj->get_spectrum();
		}
		spectrum_calculator* operator->() const {
			return &m_obj->get_spectrum();
		}
	private:
		trx_device_base * m_obj;
	};

public:
	//Get locked TX
	tx_proxy tx();
	//Locked access RX
	rx_proxy rx( int id = 0 );
	//Get locked TX
	tx_proxy tx() const;
	//Locked access RX
	rx_proxy rx( int id = 0 ) const;
	//Get spectrum
	spectrum_proxy get_spectrum();
private:
	const std::unique_ptr<trx_device_base> m_iodev;	//IO device
	const handler_t m_callback;						//Callback function
	modulation m_modulation {};				//Current modulation
};

/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
