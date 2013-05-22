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
#include "codec_types.hpp"
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
//Event type argument structure
struct event
{
	enum class type	: char
	{
		rx_char,			//receive char event type
		tx_char,			//Transmit char event type
		spectrum,			//Spectrum event type
		imd_rdy,			//IMD signal event
		clk_err				//CLK error event
	} evt;
	struct imd_s
	{
		imd_s( int v, bool n)
		: value(v), noise(n )
		{}
		int value;					//IMD value
		bool noise;					//IMD over noise floor
	};
	union
	{
		imd_s imd;					//IMD signal description
		int clkerr;					//CLK error description
		int chr;					//Received char
	};
	//Constructor version 1
	event( int imd, bool noise )
		: evt( type::imd_rdy ), imd( imd, noise )
		 {}
	//Constructor version 2
	event( type ev, int value )
		: evt(ev), clkerr( value )
	{}
};

/* ------------------------------------------------------------------------- */
/* Function event handler type
 * first parameter channel number
 * second channel type
 */
typedef std::function <void( int, const event &ev )> event_handler_t;

/* ------------------------------------------------------------------------- */
/* Receiver codec base class */
class rx_codec
{
	//Make object noncopyable
	rx_codec(const rx_codec&) = delete;
	rx_codec& operator=(const rx_codec&) = delete;
public:
	typedef std::function <void( const event &ev )> handler_t;
	rx_codec( handler_t callback )
		: m_callback( callback )
	{}
	virtual ~rx_codec() {}
	virtual void operator()( const sample_type* samples, std::size_t sample_size ) = 0;
	virtual void reset() = 0;
	virtual void set_frequency( int freq ) = 0;
	virtual int get_frequency( ) const = 0;
	virtual sqelch_value_type get_signal_level() const = 0;
	virtual void set_squelch( sqelch_value_type , squelch_mode ) = 0;
protected:
	void callback_notify( const event &ev )
	{
		m_callback( ev );
	}
private:
	const handler_t m_callback;
};
/* ------------------------------------------------------------------------- */
/* Transmitter codec base class */
class tx_codec
{
	//Make object noncopyable
	tx_codec(const tx_codec&) = delete;
	tx_codec& operator=(const tx_codec&) = delete;
public:
	typedef std::function <void( const event &ev )> handler_t;
	tx_codec()
	{}
	tx_codec( handler_t callback )
		: m_callback( callback )
	{}
	virtual ~tx_codec() {}
	virtual void put_tx( short ) = 0;
	virtual void clear_tx() = 0;
	virtual void set_freqency( int freq ) = 0;
	virtual size_t get_count() const = 0;
	virtual void reset() = 0;
	/* Return true if need to switch back to RX */
	virtual bool operator()( sample_type* sample, size_t len ) = 0;
protected:
	void callback_notify( short ch )
	{
		const event ev( event::type::tx_char, ch );
		if( m_callback ) m_callback( ev );
	}
private:
	const handler_t m_callback;
};
/* ------------------------------------------------------------------------- */
//Safe lock device engine
template <typename T>
class safe_lock
{
	safe_lock(const safe_lock&) = delete;
	safe_lock& operator=(const safe_lock&) = delete;
public:
	safe_lock( T &obj )
	  : m_obj(obj )
	{
		m_obj.lock( true );
	}
	~safe_lock( )
	{
		m_obj.lock( false );
	}
private:
	T& m_obj;
};

/* ------------------------------------------------------------------------- */
/* TRX base class hardware independent for managing the
 * transmitter receiver code
 */
class trx_device_base	//Add noncopyable
{
	//Make object noncopyable
	trx_device_base(const trx_device_base&) = delete;
	trx_device_base& operator=(const trx_device_base&) = delete;
	static constexpr auto MAX_CODECS = 4;
public:
	enum class mode		//Hardware device mode
	{
		off,		//TRX is disabled
		on,			//TRX is ON
		transmit	//TRX is OFF
	};
	/* Invalid values defs */
	static constexpr auto INVALID = -1;
	static constexpr auto ALL = -1;
	/* Constructor */
	trx_device_base( event_handler_t evt_callback )
		: m_evt_callback( evt_callback )
	{}
	/* Destructor */
	~trx_device_base()
	{
		destroy_tx_codec();
		destroy_rx_codec();
	}
	/* Set RX or TX dev mode */
	void set_mode( mode m );
	/* Get RX or TX dev mode */
	mode get_mode() const
	{
		return m_mode;
	}
	//Register RX codec
	int add_rx_codec( rx_codec* codec );
	// Destroy RX codec and remove from pool
	bool destroy_rx_codec( int idx = ALL );
	// Add codec to the poll
	bool add_tx_codec( const tx_codec *);
	//Destroy TX codec and remove from pool
	void destroy_tx_codec()
	{
		delete m_tx_codec;
		m_tx_codec = nullptr;
	}
	//Spectrum calculator object
	const spectrum_calculator& get_spectrum() const;
	// Get RX object by IDX
	rx_codec* get_rx_codec( int idx )
	{
		return idx<MAX_CODECS?m_rx_codecs[idx]:nullptr;
	}
	// Get TX object
	tx_codec* get_tx_codec()
	{
		return m_tx_codec;
	}
	//Lock and unlock device
	virtual void lock( bool lock ) = 0;
private:
	// Initialize sound hardware in selected mode
	virtual int setup_sound_hardware( mode m ) = 0;
	//ADC vector func
	void adc_hardware_isr( const sample_type *buf, size_t len );
	//DAC vector func
	void dac_hardware_isr( sample_type *buf, size_t len );
private:
	tx_codec* m_tx_codec { nullptr };	/* Transmit codec ptr */
	std::array<rx_codec*, MAX_CODECS> m_rx_codecs {{}};	/* RX codecs array */
	spectrum_calculator m_spectrum;		/* Spectrum calculator object */
	const event_handler_t m_evt_callback;	/* Event handler object */
	mode m_mode { mode::off };				/* Current device mode */
};


/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
#endif /* TRX_DEVICE_BASE_HPP_ */
/* ------------------------------------------------------------------------- */
