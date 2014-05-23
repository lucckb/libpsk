/*
 * trx_device_base.cpp
 *
 *  Created on: 22-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#include "libpsk/codec/trx_device_base.hpp"

/* ------------------------------------------------------------------------- */
//Namespace def
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
// Destroy RX codec and remove from pool
bool trx_device_base::remove_rx_codec( int idx )
{
	if( idx == ALL) {
		for( auto &v : m_rx_codecs ) {
			v.reset();
		}
	}
	else if( idx < MAX_CODECS ) {
		if( m_rx_codecs[idx] ) {
			m_rx_codecs[idx].reset();
		}
		else {
			return true;
		}
	}
	else {
		return true; //FAIL
	}
	return false; //OK
}
/* ------------------------------------------------------------------------- */
//Register RX codec
void trx_device_base::add_rx_codec( rx_codec * codec, int slot )
{
	if( m_rx_codecs[slot] == nullptr ) {
		m_rx_codecs[slot].reset( codec );
	}
}
/* ------------------------------------------------------------------------- */
//Get first free rx slot
int trx_device_base::get_rx_slot() const
{
	for( auto idx = 0; idx<MAX_CODECS; ++idx ) {
		if( m_rx_codecs[idx] == nullptr )
			return idx;
	}
	return INVALID;
}
/* ------------------------------------------------------------------------- */
/* Set RX or TX dev mode */
void trx_device_base::set_mode( trx_device_base::mode m )
{
	//Safe lock
	detail::safe_lock<decltype (*this)> lock(*this);
	setup_sound_hardware( m );
	m_mode = m;
}
/* ------------------------------------------------------------------------- */
//ADC vector func
void trx_device_base::adc_process( const sample_type *buf, size_t len )
{
	for( auto &v : m_rx_codecs ) {
		if( v != nullptr )
			(*v)( buf, len );
	}
	//Calculate the tout sample
	m_spectrum_tmr += (1000*len)/get_rx_sample_rate();
	if( m_spectrum_tmr >= m_spectrum_timeout ) {
		class locker {
		public:
			locker( trx_device_base &obj) :
				m_obj(obj) {
				m_lock = m_obj.try_lock( lock_spectrum );
			}
			~locker() {
				if(m_lock) m_obj.unlock( lock_spectrum );
			}
			operator bool() const {
				return m_lock;
			}
		private:
			trx_device_base &m_obj;
			bool m_lock;
		} lock( *this );
		if( lock ) {
			//True if all samples completed
			if( m_spectrum.copy_samples( buf, len ) )
			{
				m_spectrum_tmr = 0;
				//Inform that user can calculate the FFT
				callback_notify( event::type::spectrum );
			}
		}
	}
}
/* ------------------------------------------------------------------------- */
//DAC vector func return true if finished
bool trx_device_base::dac_process( sample_type *buf, size_t len )
{
	if ( (*m_tx_codec)( buf, len ) ) {
		m_mode = mode::on;
		//Notify switch to RX
		callback_notify( event::type::tx_end );
		return true;
	}
	return false;
}
/* ------------------------------------------------------------------------- */
}}	//NS end
/* ------------------------------------------------------------------------- */


