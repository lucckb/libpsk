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
#include <memory>
#include "libpsk/psk/spectrum_calculator.hpp"
#include "libpsk/codec/codec_types.hpp"
#include "libpsk/codec/trx_device_base.hpp"
#include "libpsk/codec/modulation_config.hpp"
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
		clk_err,			//CLK error event
		tx_end,				//TX end goto rx
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
	//Construction version 3 no data
	event( type ev )
		: evt(ev), clkerr( 0 )
	{
	}
};

/* ------------------------------------------------------------------------- */

//Vector data
typedef std::array<int, 16> signal_vector_type;
//Sync vector data
typedef std::array<unsigned int, 16> sync_array_type;

namespace detail
{
	//Safe lock device engine
	template <typename T>
	class safe_lock
	{
		safe_lock(const safe_lock&) = delete;
		safe_lock& operator=(const safe_lock&) = delete;
	public:
		safe_lock( T &obj)
		  : m_obj(obj )
		{
			m_obj.lock();
		}
		~safe_lock( )
		{
			m_obj.unlock();
		}
	private:
		T& m_obj;
	};
}

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
	virtual int set_mode( const modulation_config_base &config ) = 0;
	virtual bool get_vector_data( signal_vector_type& ) const
	{
		//Not handled
		return true;
	}
	virtual bool set_afc_limit( int /*limit*/ )
	{
		//Not handled
		return true;
	}
	virtual bool get_sync_data(sync_array_type& ) const
	{
		return true;
	}
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
	tx_codec( handler_t callback )
		: m_callback( callback )
	{}
	virtual ~tx_codec() {}
	virtual void put_tx( short ) = 0;
	virtual void clear_tx() = 0;
	virtual void set_freqency( int freq ) = 0;
	virtual size_t get_count() const = 0;
	virtual void reset() = 0;
	virtual int set_mode( const  modulation_config_base &config ) = 0;
	//Check if device is in transmit state and can't switch to RX
	virtual bool is_transmitting() const = 0;
	/* Return true if need to switch back to RX */
	virtual bool operator()( sample_type* sample, size_t len ) = 0;
protected:
	void callback_notify( short ch )
	{
		if( m_callback )
		{
			const event ev( event::type::tx_char, ch );
			m_callback( ev );
		}
	}
private:
	const handler_t m_callback;
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
	static constexpr auto FFT_UPDATE_TIME_MS = 250; //FFT update time in ms
	static const auto TX_SAMPLE_RATE = 32000;
	static const auto RX_SAMPLE_RATE = 8000;
public:
	//Os specific locker
	enum lock_type
	{
		lock_object,
		lock_spectrum
	};
	typedef std::function <void( const event &ev )> handler_t;
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
	trx_device_base( handler_t evt_callback )
		: m_evt_callback( evt_callback ) {
	}
	/* Destructor */
	virtual ~trx_device_base() {
	}
	unsigned get_tx_sample_rate() const {
		return TX_SAMPLE_RATE;
	}
	unsigned get_rx_sample_rate() const {
		return RX_SAMPLE_RATE;
	}
	/* Set RX or TX dev mode */
	void set_mode( mode m );
	/* Get RX or TX dev mode */
	mode get_mode() const {
		return m_mode;
	}
	//Register RX codec
	void add_rx_codec( rx_codec* codec, int slot );
	//Get first RX slot
	int get_rx_slot() const;
	// Destroy RX codec and remove from pool
	bool remove_rx_codec( int idx = ALL );
	// Remove tx codec
	bool remove_tx_codec() {
		m_tx_codec.reset();
		return false;
	}
	// Add codec to the poll
	bool add_tx_codec( tx_codec *c ) {
		m_tx_codec.reset( c );
		return false;
	}
	//Spectrum calculator object
	spectrum_calculator& get_spectrum() {
		return m_spectrum;
	}
	// Get RX object by IDX
	rx_codec* get_rx_codec( int idx ) {
		return idx<MAX_CODECS?m_rx_codecs[idx].get():nullptr;
	}
	// Get TX object
	tx_codec* get_tx_codec() {
		return m_tx_codec.get();
	}
	//Set FFT interval
	void set_fft_interval( unsigned short timeout ) {
		m_spectrum_timeout = timeout;
	}
	//Lock and unlock device
	virtual void lock( int id = lock_object ) = 0;
	//Lock and unlock device
	virtual void unlock( int id = lock_object ) = 0;
	//Try lock the device
	virtual bool try_lock( int id = lock_object ) = 0;
	//Sleep for terminate audio thread
	virtual int join( ) = 0;
private:
	// Initialize sound hardware in selected mode
	virtual int setup_sound_hardware( mode m ) = 0;
protected:
	//ADC vector func
	void adc_process( const sample_type *buf, size_t len );
	//DAC vector func
	bool dac_process( sample_type *buf, size_t len );
	//Set mode
	void set_mode_off() {
		m_mode = mode::off;
	}
	//Callback notify when extra params was ready
	void callback_notify( event::type ev ) {
		if( m_evt_callback ) {
			const event _ev( ev );
			m_evt_callback( _ev );
		}
	}
private:
	std::unique_ptr<tx_codec> m_tx_codec;		/* TX codec */
	std::array<std::unique_ptr<rx_codec>, MAX_CODECS> m_rx_codecs;	/* RX codecs array */
	spectrum_calculator m_spectrum;				/* Spectrum calculator object */
	unsigned m_spectrum_tmr {};					/* Spectrum time */
	unsigned short m_spectrum_timeout { 250 };	/* Spectrum timeout */
	const handler_t m_evt_callback;		/* Event handler object */
	mode m_mode { mode::off };					/* Current device mode */
};


/* ------------------------------------------------------------------------- */
}}
/* ------------------------------------------------------------------------- */
#endif /* TRX_DEVICE_BASE_HPP_ */
/* ------------------------------------------------------------------------- */
