/*
 * trx_device_base.hpp
 *
 *  Created on: 20-05-2013
 *      Author: lucck
 */

#ifndef LIBPSK_TRX_DEVICE_BASE_HPP_
#define LIBPSK_TRX_DEVICE_BASE_HPP_

/* ------------------------------------------------------------------------- */
#include <functional>
#include <array>
#include "psk/spectrum_calculator.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
enum class event
{
	rx_char,
	tx_char,
	spectrum,
	imd_rdy,
	clk_err
};

/* ------------------------------------------------------------------------- */
//Status channel
typedef std::function <int( event, int , int )> event_handler_t;

/* ------------------------------------------------------------------------- */
class rx_codec
{
	//Make object noncopyable
	rx_codec(const rx_codec&) = delete;
	rx_codec& operator=(const rx_codec&) = delete;
public:
	virtual void set_freqency( int value );
	//Retuer event mask
	enum event
	{
		EV_RX_CHAR = 1,
		EV_CLK_ERR = 2,
		EV_IMD_RDY = 4
	} ;
	virtual void operator()( const sample_type* samples, std::size_t sample_size ) = 0;
	virtual void reset() = 0;
	virtual short read_char() const = 0;
	virtual size_t get_count() const = 0;
	virtual unsigned get_event( event ev ) = 0;
	virtual int get_signal_level() const = 0;
	virtual sqelch_value_type  get_squelch() const = 0;
	virtual void set_squelch(sqelch_value_type , squelch_mode) = 0;
};
/* ------------------------------------------------------------------------- */
class tx_codec
{
	//Make object noncopyable
	tx_codec(const tx_codec&) = delete;
	tx_codec& operator=(const tx_codec&) = delete;
public:
	//Constructor and destructor
	tx_codec() {}
	virtual ~tx_codec()
	{}
	//Public interface
	virtual void put_tx( short chr ) = 0;
	virtual void clear_tx() = 0;
	virtual void set_freqency( int freq ) = 0;
	virtual size_t get_count() const = 0;
	virtual void reset() = 0;
	enum event
	{
		EV_SEND_CHAR_COMPLETE  = 1,
		EV_TX_COMPLETE    	   = 2
	};
	virtual unsigned operator()( sample_type* sample, size_t len ) = 0;
};

/* ------------------------------------------------------------------------- */
class trx_device_base	//Add noncopyable
{
	//Make object noncopyable
	trx_device_base(const trx_device_base&) = delete;
	trx_device_base& operator=(const trx_device_base&) = delete;
	static constexpr auto MAX_CODECS = 4;
public:
	enum class mode
	{
		off,
		on,
		transmit
	};
	//Set mode
	void set_mode( mode );
	/* It is not manage memory */
	int register_rx_codec( const rx_codec* );	//Register RX codec
	void deregister_rx_codec( int idx ); 		//Codec must be deregisrered manually
	const spectrum_calculator& get_spectrum() const;
	rx_codec* get_rx( int idx );
	tx_codec* get_tx();
private:
	virtual int activate_rx_mode( bool enable );
	virtual int activate_tx_mode( bool enable );
	//AC and CA handler
	void adc_handler( const sample_type *buf, size_t len );
	void dac_handler( sample_type *buf, size_t len );
private:
	tx_codec* m_tx_codec;
	std::array<rx_codec*, MAX_CODECS> m_rx_codecs;
	spectrum_calculator m_spectrum;
	short m_nrxcodecs;
};


/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
#endif /* TRX_DEVICE_BASE_HPP_ */
/* ------------------------------------------------------------------------- */
