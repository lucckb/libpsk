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
private:
	std::unique_ptr<trx_device_base> m_iodev;	//IO device
	handler_t m_callback;
};

/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
#endif /* HAM_DIGI_HPP_ */
/* ------------------------------------------------------------------------- */
