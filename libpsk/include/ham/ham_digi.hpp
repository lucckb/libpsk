/*
 * ham_digi.hpp
 *
 *  Created on: 27-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#ifndef HAM_DIGI_HPP_
#define HAM_DIGI_HPP_
/* ------------------------------------------------------------------------- */
#include "codec/trx_device_base.hpp"
#include <memory>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Object noncopyable
class ham_digi
{
	ham_digi(const ham_digi&) = delete;
	ham_digi& operator=(const ham_digi&) = delete;
public:
	/* Modulation structure */
	enum class modulation : short
	{
		psk			//PSK modulaton
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
	int set_modulation( modulator mod );
	/* New RX channel */
	int rx_channel_add();						/* Extra channel ADD */
	int rx_channel_remove( int chn_id );		/* Extra channel remove */
private:
	class tx_access
	{
		tx_access(const tx_access&) = delete;
		tx_access& operator=(const tx_access&) = delete;
	public:
		tx_access( trx_device_base *obj )
		: m_obj(obj)
		{
			if( m_obj ) m_obj->lock();
		}
		~tx_access()
		{
			if( m_obj ) m_obj->unlock();
		}
		tx_codec* operator->()
		{
			return m_obj->get_tx_codec();
		}
	private:
		trx_device_base* const m_obj;
	};
	class rx_access
	{
		rx_access(const rx_access&) = delete;
		rx_access& operator=(const rx_access&) = delete;
	public:
		rx_access( trx_device_base *obj, int id )
			: m_obj(obj), m_id(id)
		{
			if( m_obj ) m_obj->lock();
		}
		~rx_access()
		{
			if( m_obj ) m_obj->unlock();
		}
		rx_codec* operator->()
		{
			return m_obj->get_rx_codec( m_id );
		}
	private:
		trx_device_base* const m_obj;
		const int m_id;
	};
public:
	//Get locked TX
	tx_access tx();
	//Locked access RX
	rx_access rx(int id=0);

	spectrum_calculator const& get_lock_spectrum();
	void unlock_spectrum();
private:
	std::unique_ptr<trx_device_base> m_iodev;	//IO device
	handler_t m_callback;
};

/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
#endif /* HAM_DIGI_HPP_ */
/* ------------------------------------------------------------------------- */
