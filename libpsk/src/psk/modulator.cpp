/*
 * modulator.cpp
 *
 *  Created on: 08-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "psk/modulator.hpp"
#include <cmath>

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------- */

namespace
{
	//PSK state shapes structure
	struct psk_state
	{
		int (*iptr)(int,int);
		int (*qptr)(int,int);
		int next;
	};
	//PSK Shapes funcs
	namespace psk_shapes
	{
		//RMS constants
		static constexpr auto C_rms = 22000;
		//Cosinus funcs for ramp
		namespace mth
		{
			constexpr int PI2I = dsp::integer::trig::sin_arg_max<short,512>();
			int cos( int angle )
			{
				return dsp::integer::trig::sin<short, 512, C_rms>(PI2I/4 + angle);
			}
		}
		//zero shape
		int Z( int, int )
		{
			return 0;
		}
		//Plus one shape
		int P( int, int )
		{
			return C_rms;
		}
		//Minus one shape
		int M( int, int )
		{
			return -C_rms;
		}
		//Plus minus transient
		int PM( int pos, int max )
		{
			return mth::cos( (mth::PI2I*pos)/(2*max) );
		}
		//Minus plus transient
		int MP( int pos, int max )
		{
			return -mth::cos( (mth::PI2I*pos)/(2*max) );
		}
		//Plus zero transient
		int PZ( int pos, int max )
		{
			return (pos<max/2)?(mth::cos( (mth::PI2I*pos)/(2*max) )):(0);
		}
		//Minus zero transient
		int MZ(int pos, int max)
		{
			return (pos<max/2)?(-mth::cos((mth::PI2I*pos)/(2*max))):(0);
		}
		//Zero plus transient
		int ZP(int pos, int max)
		{
			return (pos<max/2)?(0):(-mth::cos((mth::PI2I*pos)/(2*max)));
		}
	}
	//Signal phaze states
	constexpr int  PHZ_0 = 0;			//specify various signal phase states
	constexpr int  PHZ_90  = 1;
	constexpr int  PHZ_180 = 2;
	constexpr int  PHZ_270 = 3;
	constexpr int  PHZ_OFF = 4;

	static constexpr psk_state psk_phase_lookup_table[6][5]=
	{
		// SYMBOL = 0 = SYM_NOCHANGE
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::P, psk_shapes::P, PHZ_0,	//present PHZ_0
			psk_shapes::M, psk_shapes::P, PHZ_90,	//present PHZ_90
			psk_shapes::M, psk_shapes::M, PHZ_180,	//present PHZ_180
			psk_shapes::P, psk_shapes::M, PHZ_270,	//present PHZ_270
			psk_shapes::Z, psk_shapes::Z, PHZ_OFF,	//present PHZ_OFF
		// SYMBOL = 1 = SYM_P90 = Advance 90 degrees
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::PM, psk_shapes::P, PHZ_90,	//present PHZ_0
			psk_shapes::M, psk_shapes::PM, PHZ_180,	//present PHZ_90
			psk_shapes::MP, psk_shapes::M, PHZ_270,	//present PHZ_180
			psk_shapes::P, psk_shapes::MP, PHZ_0,	//present PHZ_270
			psk_shapes::ZP, psk_shapes::ZP, PHZ_0,	//present PHZ_OFF
		// SYMBOL = 2 = SYM_P180 = Advance 180 degrees
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::PM, psk_shapes::PM, PHZ_180,//present PHZ_0
			psk_shapes::MP, psk_shapes::PM, PHZ_270,//present PHZ_90
			psk_shapes::MP, psk_shapes::MP, PHZ_0,	//present PHZ_180
			psk_shapes::PM, psk_shapes::MP, PHZ_90,	//present PHZ_270
			psk_shapes::ZP, psk_shapes::ZP, PHZ_0,	//present PHZ_OFF
		// SYMBOL = 3 = SYM_M90	= retard 90 degrees
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::P, psk_shapes::PM, PHZ_270,	//present PHZ_0
			psk_shapes::MP, psk_shapes::P, PHZ_0,	//present PHZ_90
			psk_shapes::M, psk_shapes::MP, PHZ_90,	//present PHZ_180
			psk_shapes::PM, psk_shapes::M, PHZ_180,	//present PHZ_270
			psk_shapes::ZP, psk_shapes::ZP, PHZ_0,	//present PHZ_OFF
		// SYMBOL = 4 = SYM_OFF
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::PZ, psk_shapes::PZ, PHZ_OFF,//present PHZ_0
			psk_shapes::MZ, psk_shapes::PZ, PHZ_OFF,//present PHZ_90
			psk_shapes::MZ, psk_shapes::MZ, PHZ_OFF,//present PHZ_180
			psk_shapes::PZ, psk_shapes::MZ, PHZ_OFF,//present PHZ_270
			psk_shapes::Z,  psk_shapes::Z, PHZ_OFF,	//present PHZ_OFF
		// SYMBOL = 5 = SYM_ON
		//   I ramp shape     Q ramp shape     Next Phase
			psk_shapes::P,  psk_shapes::P, PHZ_0,	//present PHZ_0
			psk_shapes::MP, psk_shapes::P, PHZ_0,	//present PHZ_90
			psk_shapes::MP, psk_shapes::MP, PHZ_0,	//present PHZ_180
			psk_shapes::P, psk_shapes::MP, PHZ_0,	//present PHZ_270
			psk_shapes::ZP, psk_shapes::ZP, PHZ_0	//present PHZ_OFF
		};

}
/* ------------------------------------------------------------------------- */
//Vect lookup table
constexpr int modulator::m_vect_lookup[6][2];

/* ------------------------------------------------------------------------- */
//Modulator
modulator::modulator( int sample_freq, int tx_freq, std::size_t char_que_len )
	: 	 m_sample_freq( sample_freq ),  m_p_psk_tx_i(psk_shapes::Z), m_p_psk_tx_q(psk_shapes::Z),
		 m_chqueue( char_que_len )
{

	//TODO convert to array
	for(int i=0; i<16; i++)
		m_iq_phase_array[i] = 1;
	m_present_phase = PHZ_OFF;
	set_mode( mode::bpsk, baudrate::b31 );
	set_freqency( tx_freq );
}

/* ------------------------------------------------------------------------- */
//Set frequency
void  modulator::set_freqency( int frequency )
{
	m_psk_phase_inc = (PI2I*frequency)/m_sample_freq;
}
/* ------------------------------------------------------------------------- */
//Set mode
void modulator::set_mode( mode mmode, baudrate baud )
{
	m_mode = mmode;
	m_ramp = 0;
	if( baud == baudrate::b63 )
	{
		m_symbol_rate = 6250;
	}
	else if( baud == baudrate::b125 )
	{
		m_symbol_rate = 12500;
	}
	else
	{
		m_symbol_rate = 3125;
	}
	switch( mmode )
	{
		case mode::bpsk:
		case mode::qpskl:
		case mode::qpsku:
			m_state = state::preamble;
			break;
		case mode::tune:
			m_state = state::tune;
			break;
		default:
			m_state = state::preamble;
			break;
	}
	m_psk_period_update = (m_sample_freq*RATE_SCALE)/ m_symbol_rate;
	//TODO: Fix this
	m_encoder.set_mode( (_internal::symbol_encoder::mode)mmode );
}

/* ------------------------------------------------------------------------- */
//Set char into the modulator
void modulator::put_tx( short txchar )
{
	constexpr char BACK_SPACE_CODE = 0x08;
	//EnterCriticalSection(&m_CriticalSection);
	if( txchar == ctrl_chars::TX_CNTRL_AUTOSTOP )
	{
		m_temp_need_shutoff = true;
		if( m_state==state::tune )
			m_need_shutoff = true;
	}
	else if( txchar == ctrl_chars::TX_CNTRL_NOSQTAIL )
	{
		m_temp_no_squelch_tail = true;
	}
	else
	{
		if( (txchar != BACK_SPACE_CODE) || m_chqueue.empty() )
		{
			m_chqueue.push( txchar );
		}
		else	//see if is a backspace and if can delete it in the queue
		{
			m_chqueue.pop();
		}
	}
	//LeaveCriticalSection(&m_CriticalSection);
}
/* ------------------------------------------------------------------------- */
//Operator on new samples
unsigned modulator::operator()( sample_type* sample, size_t len )
{
	//Amplitude factor
	int v = 0;
	unsigned r = 0;
	const auto ramp_size =  (m_sample_freq*RATE_SCALE/m_symbol_rate);
	for( size_t i=0; i<len; i++ )		//calculate n samples of tx data stream
	{
		// create sample from sin/cos and shape tables
		sample[i] =  m_nco_mix.mix_sin( m_p_psk_tx_i(m_ramp, ramp_size ), 0 )  +
				     m_nco_mix.mix_cos( m_p_psk_tx_q(m_ramp, ramp_size ), m_psk_phase_inc) ;
		m_ramp++;
		if( ++m_psk_sample_cnt >= m_psk_period_update )//if time to update symbol
		{
			m_psk_sample_cnt = 0;
			m_ramp = 0;						// time to update symbol
			short ch = 0;
			if( m_encoder.eos() )
			{
				ch = update_state_chr();
			}
			const int symbol = m_encoder( ch );
			//get new I/Q ramp tables and next phase
			m_p_psk_tx_i = psk_phase_lookup_table[symbol][m_present_phase].iptr;
			m_p_psk_tx_q = psk_phase_lookup_table[symbol][m_present_phase].qptr;
			m_present_phase = psk_phase_lookup_table[symbol][m_present_phase].next;
			//get new I/Q ramp tables and next phase
			m_iq_phase_array[v++] = m_vect_lookup[m_present_phase][0];
			m_iq_phase_array[v++] = m_vect_lookup[m_present_phase][1];
			v = v & 0x000F;	//keep bounded to 16
			if( m_state == state::sending && ch > 0 )
				r |= tx_codec::EV_SEND_CHAR_COMPLETE;
		}
	}
	if( m_state==state::off ) r |= tx_codec::EV_TX_COMPLETE;
	return r;
}


/* ------------------------------------------------------------------------- */
int modulator::get_tx_char()
{
	short ch;
	if( m_chqueue.pop( ch ) )
		ch = ctrl_chars::TXTOG_CODE;
	if(m_temp_need_shutoff)
	{
		m_temp_need_shutoff = false;
		m_need_shutoff = true;
	}
	if(m_temp_no_squelch_tail)
	{
		m_temp_no_squelch_tail = false;
		m_no_squelch_tail = true;
	}
	return ch;
}
/* ------------------------------------------------------------------------- */
int modulator::update_state_chr()
{
	int ch = 0;
	switch( m_state )
	{
		case state::off:		//is receiving
			ch = ctrl_chars::TXOFF_CODE;		//else turn off
			m_need_shutoff = false;
			break;
		case state::tune:
			ch = ctrl_chars::TXON_CODE;				// steady carrier
			if(	m_need_shutoff)
			{

				m_state = state::off;
				m_amble_ptr = 0;
				ch = ctrl_chars::TXOFF_CODE;
				m_need_shutoff = false;
			}
			break;
		case state::postamble:		// ending sequence
			if( ++m_amble_ptr>C_amble_size  || m_no_squelch_tail)
			{
				m_no_squelch_tail = false;
				m_state = state::off;
				m_amble_ptr = 0;
				ch = ctrl_chars::TXOFF_CODE;
				m_need_shutoff = false;
			}
			else
			{
				ch = C_postamble_chr;
			}
			break;
		case state::preamble:			//starting sequence
			if( ++m_amble_ptr>C_amble_size )
			{
				m_state = state::sending;
				m_amble_ptr = 0;
				ch = ctrl_chars::TXTOG_CODE;
			}
			else
			{
				ch = C_preamble_chr;
			}
			break;
		case state::sending:		//if sending text from TX window
			ch = get_tx_char();
			if(	(ch == ctrl_chars::TXTOG_CODE) && m_need_shutoff)
			{
				m_state = state::postamble;
			}
			m_amble_ptr = 0;
			break;
	}
	return( ch );
}

/* ------------------------------------------------------------------------- */
//Clear queue
void modulator::clear_tx()
{
	//EnterCriticalSection(&m_CriticalSection);
	m_chqueue.erase();
	//LeaveCriticalSection(&m_CriticalSection);
	m_no_squelch_tail = false;
	m_temp_need_shutoff = false;
	m_temp_no_squelch_tail = false;
}

/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
