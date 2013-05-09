/*
 * modulator.cpp
 *
 *  Created on: 08-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "psk/modulator.hpp"
#include <cmath>
#include <iostream>
using namespace std;
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------- */

namespace
{
	static constexpr auto m_2PI = 8.0 * atan(1.0);
	struct PSKSTATE
	{
		double* iptr;
		double* qptr;
		int	next;
	};

	struct psk_state
	{
		double (*iptr)(double,double);
		double (*qptr)(double,double);
		int next;
	};
	namespace psk_shapes
	{
		//zero shape
		double Z( double, double )
		{
			return 0;
		}
		//Plus one shape
		double P( double, double )
		{
			return 1;
		}
		//Minus one shape
		double M( double, double )
		{
			return -1;
		}
		double PM( double angle, double ramp )
		{
			return std::cos( angle*m_2PI/(ramp*2) );
		}
		double MP( double angle, double ramp )
		{
			return -std::cos( angle*m_2PI/(ramp*2) );
		}
		double PZ( double angle, double ramp )
		{
			return (angle<ramp/2)?(std::cos( angle*m_2PI/(ramp*2))):(0);
		}
		double MZ(double angle, double ramp)
		{
			return (angle<ramp/2)?(-std::cos(angle*m_2PI/(ramp*2))):(0);
		}
		double ZP(double angle, double ramp)
		{
			return (angle<ramp/2)?(0):(-std::cos(angle*m_2PI/(ramp*2)));
		}
	}
	constexpr int  PHZ_0 = 0;			//specify various signal phase states
	constexpr int  PHZ_90  = 1;
	constexpr int  PHZ_180 = 2;
	constexpr int  PHZ_270 = 3;
	constexpr int  PHZ_OFF = 4;
	constexpr int  MAXRAMP_SIZE = ( ((100*8160)/3125) +1);
	static double PSKShapeTbl_Z[MAXRAMP_SIZE];	// 0
	static double PSKShapeTbl_P[MAXRAMP_SIZE];	// +1
	static double PSKShapeTbl_M[MAXRAMP_SIZE];	// -1
	static double PSKShapeTbl_ZP[MAXRAMP_SIZE];	// 0 to +1
	static double PSKShapeTbl_PZ[MAXRAMP_SIZE];	// +1 to 0
	static double PSKShapeTbl_MZ[MAXRAMP_SIZE];	// -1 to 0
	static double PSKShapeTbl_PM[MAXRAMP_SIZE];	// +1 to -1
	static double PSKShapeTbl_MP[MAXRAMP_SIZE];	// -1 to +1


	static constexpr PSKSTATE PSKPhaseLookupTable[6][5]=
	{
	// SYMBOL = 0 = SYM_NOCHANGE
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_P, PSKShapeTbl_P, PHZ_0,	//present PHZ_0
		PSKShapeTbl_M, PSKShapeTbl_P, PHZ_90,	//present PHZ_90
		PSKShapeTbl_M, PSKShapeTbl_M, PHZ_180,	//present PHZ_180
		PSKShapeTbl_P, PSKShapeTbl_M, PHZ_270,	//present PHZ_270
		PSKShapeTbl_Z, PSKShapeTbl_Z, PHZ_OFF,	//present PHZ_OFF
	// SYMBOL = 1 = SYM_P90 = Advance 90 degrees
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_PM, PSKShapeTbl_P, PHZ_90,	//present PHZ_0
		PSKShapeTbl_M, PSKShapeTbl_PM, PHZ_180,	//present PHZ_90
		PSKShapeTbl_MP, PSKShapeTbl_M, PHZ_270,	//present PHZ_180
		PSKShapeTbl_P, PSKShapeTbl_MP, PHZ_0,	//present PHZ_270
		PSKShapeTbl_ZP, PSKShapeTbl_ZP, PHZ_0,	//present PHZ_OFF
	// SYMBOL = 2 = SYM_P180 = Advance 180 degrees
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_PM, PSKShapeTbl_PM, PHZ_180,//present PHZ_0
		PSKShapeTbl_MP, PSKShapeTbl_PM, PHZ_270,//present PHZ_90
		PSKShapeTbl_MP, PSKShapeTbl_MP, PHZ_0,	//present PHZ_180
		PSKShapeTbl_PM, PSKShapeTbl_MP, PHZ_90,	//present PHZ_270
		PSKShapeTbl_ZP, PSKShapeTbl_ZP, PHZ_0,	//present PHZ_OFF
	// SYMBOL = 3 = SYM_M90	= retard 90 degrees
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_P, PSKShapeTbl_PM, PHZ_270,	//present PHZ_0
		PSKShapeTbl_MP, PSKShapeTbl_P, PHZ_0,	//present PHZ_90
		PSKShapeTbl_M, PSKShapeTbl_MP, PHZ_90,	//present PHZ_180
		PSKShapeTbl_PM, PSKShapeTbl_M, PHZ_180,	//present PHZ_270
		PSKShapeTbl_ZP, PSKShapeTbl_ZP, PHZ_0,	//present PHZ_OFF
	// SYMBOL = 4 = SYM_OFF
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_PZ, PSKShapeTbl_PZ, PHZ_OFF,//present PHZ_0
		PSKShapeTbl_MZ, PSKShapeTbl_PZ, PHZ_OFF,//present PHZ_90
		PSKShapeTbl_MZ, PSKShapeTbl_MZ, PHZ_OFF,//present PHZ_180
		PSKShapeTbl_PZ, PSKShapeTbl_MZ, PHZ_OFF,//present PHZ_270
		PSKShapeTbl_Z, PSKShapeTbl_Z, PHZ_OFF,	//present PHZ_OFF
	// SYMBOL = 5 = SYM_ON
	//   I ramp shape     Q ramp shape     Next Phase
		PSKShapeTbl_P, PSKShapeTbl_P, PHZ_0,	//present PHZ_0
		PSKShapeTbl_MP, PSKShapeTbl_P, PHZ_0,	//present PHZ_90
		PSKShapeTbl_MP, PSKShapeTbl_MP, PHZ_0,	//present PHZ_180
		PSKShapeTbl_P, PSKShapeTbl_MP, PHZ_0,	//present PHZ_270
		PSKShapeTbl_ZP, PSKShapeTbl_ZP, PHZ_0	//present PHZ_OFF
	};
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

	namespace ch = ctrl_chars;
	static constexpr short ct_preamble[] =
	{
		ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE,
		ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE,
		ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE,
		ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, ch::TXTOG_CODE, 0
	};
	static constexpr short ct_postamble[] =
	{
		ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE,
		ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE,
		ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE,
		ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, ch::TXON_CODE, 0
	};
}
/* ------------------------------------------------------------------------- */
//Vect lookup table
constexpr int modulator::m_vect_lookup[6][2];

/* ------------------------------------------------------------------------- */
//Modulator
modulator::modulator( int sample_freq, int tx_freq, std::size_t char_que_len )
	: m_sample_freq( sample_freq ),  m_chqueue( char_que_len )
{
	m_psk_phase_inc = m_2PI * tx_freq/sample_freq;		//carrier frequency
	m_psk_sec_per_samp = 1.0/sample_freq;
	m_psk_time = 0.0;
	m_t = 0.0;

	//TODO convert to array
	for(int i=0; i<16; i++)
		m_iq_phase_array[i] = 1;
	m_p_psk_tx_i = PSKShapeTbl_Z;
	m_p_psk_tx_q = PSKShapeTbl_Z;
	m_present_phase = PHZ_OFF;
	set_mode( mode::bpsk, baudrate::b31 );
	set_freqency( tx_freq );
}

/* ------------------------------------------------------------------------- */
//Set frequency
void  modulator::set_freqency( int frequency )
{
	m_psk_phase_inc = m_2PI * frequency/m_sample_freq;
}
/* ------------------------------------------------------------------------- */
//Set mode
void modulator::set_mode( mode mmode, baudrate baud )
{
	m_mode = mmode;
	m_ramp = 0;
	if( baud == baudrate::b63 )
	{
		m_symbol_rate = 62.5;
	}
	else if( baud == baudrate::b125 )
	{
		m_symbol_rate = 125;
	}
	else
	{
		m_symbol_rate = 31.25;
	}

	m_psk_period_update = 1.0/m_symbol_rate;	//symbol period
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
	//enerate cosine ramp envelope lookup tables
	const size_t ramp_size =  (int)(m_sample_freq/m_symbol_rate); //  number of envelope ramp steps per symbol
	for( size_t i=0; i<MAXRAMP_SIZE; i++)
	{
		PSKShapeTbl_Z[i] = 0.0;
		PSKShapeTbl_P[i] = 1.0;
		PSKShapeTbl_M[i] = -1.0;
		PSKShapeTbl_PM[i] = cos( (double)i*m_2PI/(ramp_size*2) );
		PSKShapeTbl_MP[i] = -PSKShapeTbl_PM[i];

		if( i <ramp_size/2 )
		{
			PSKShapeTbl_PZ[i] = cos( (double)i*m_2PI/(ramp_size*2) );
			PSKShapeTbl_MZ[i] = -PSKShapeTbl_PZ[i];
			PSKShapeTbl_ZP[i] = 0.0;
		}
		else
		{
			PSKShapeTbl_ZP[i] = -cos( (double)i*m_2PI/(ramp_size*2) );
			PSKShapeTbl_PZ[i] = 0.0;
			PSKShapeTbl_MZ[i] = 0.0;
		}

	}
	m_encoder.set_mode( (_internal::symbol_encoder::mode)mmode );
}

/* ------------------------------------------------------------------------- */
//Set char into the modulator
void modulator::put_tx( short txchar )
{
	constexpr char BACK_SPACE_CODE = 0x08;
	//EnterCriticalSection(&m_CriticalSection);
#if 0
	if( cntrl )	//is a tx control code
	{
		switch( txchar )
		{
			case TX_CNTRL_AUTOSTOP:
				m_temp_need_shutoff = TRUE;
				if( m_TXState==TX_TUNE_STATE )
					m_need_shutoff = TRUE;
				break;
			case TX_CNTRL_ADDCWID:
				m_TempNeedCWid = TRUE;
				break;
			case TX_CNTRL_NOSQTAIL:
				m_temp_no_squelch_tail = TRUE;
				break;
		}
	}
	else		//is a character to xmit
#endif
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
void modulator::operator()( int16_t* sample, size_t len )
{
	//Amplitude factor
	constexpr double m_RMSConstant = 22000;
	int v = 0;
	for( size_t i=0; i<len; i++ )		//calculate n samples of tx data stream
	{
		m_t += m_psk_phase_inc;			// increment radian phase count
		// create sample from sin/cos and shape tables
		sample[i] = m_RMSConstant*( m_p_psk_tx_i[m_ramp]* sin( m_t ) + m_p_psk_tx_q[m_ramp++]* cos( m_t ) );
		m_psk_time += m_psk_sec_per_samp;
		if( m_psk_time >= m_psk_period_update )//if time to update symbol
		{
			short ch = 0;
			m_psk_time -= m_psk_period_update;	//keep time bounded
			m_ramp = 0;						// time to update symbol
			m_t = fmod(m_t,m_2PI);			//keep radian counter bounded
			if( m_encoder.eos() )
			{
				ch = get_char();
				cout << "Get char " << ch << endl;
			}
			const int symbol = m_encoder( ch );
			//get new I/Q ramp tables and next phase
			m_p_psk_tx_i = PSKPhaseLookupTable[symbol][m_present_phase].iptr;
			m_p_psk_tx_q = PSKPhaseLookupTable[symbol][m_present_phase].qptr;
			m_present_phase = PSKPhaseLookupTable[symbol][m_present_phase].next;
			m_iq_phase_array[v++] = m_vect_lookup[m_present_phase][0];
			m_iq_phase_array[v++] = m_vect_lookup[m_present_phase][1];
			v = v & 0x000F;	//keep bounded to 16
		}
	}
}


/* ------------------------------------------------------------------------- */
int modulator::get_tx_char()
{
	short ch;
	if( m_chqueue.pop( ch ) )
		ch = ch::TXTOG_CODE;
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
int modulator::get_char()
{
	int ch = 0;
	// static test = '0';  //AA6YQ - not referenced
	switch( m_state )
	{
		case state::off:		//is receiving
			ch = ch::TXOFF_CODE;		//else turn off
			m_need_shutoff = false;
			break;
		case state::tune:
			ch = ch::TXON_CODE;				// steady carrier
			if(	m_need_shutoff)
			{

				m_state = state::off;
				m_amble_ptr = 0;
				ch = ch::TXOFF_CODE;
				m_need_shutoff = false;
			}
			break;
		case state::postamble:		// ending sequence
			if( !(ch = ct_postamble[m_amble_ptr++] ) || m_no_squelch_tail)
			{
				m_no_squelch_tail = false;
				m_state = state::off;
				m_amble_ptr = 0;
				ch = ch::TXOFF_CODE;
				m_need_shutoff = false;
			}
			break;
		case state::preamble:			//starting sequence
			if( !(ch = ct_preamble[m_amble_ptr++] ))
			{
				m_state = state::sending;
				m_amble_ptr = 0;
				ch = ctrl_chars::TXTOG_CODE;
			}
			break;
		case state::sending:		//if sending text from TX window
			ch = get_tx_char();
			if(	(ch == ctrl_chars::TXTOG_CODE) && m_need_shutoff)
			{
				m_state = state::postamble;
			}
			else
			{
				//if( ch > 0 )
				//	::PostMessage(m_hWnd, MSG_PSKCHARRDY,ch,-1);
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
//Get number of chars remaining
size_t modulator::size_tx() const
{
	return m_chqueue.size();
}
/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
