/*
 * modulator.cpp
 *
 *  Created on: 08-05-2013
 *      Author: lucck
 */
/* ------------------------------------------------------------------------- */
#include "psk/modulator.hpp"
#include "psk/varicode.hpp"
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
	constexpr auto TXOFF_CODE = -1;			// control codes that can be placed in the input
	constexpr auto TXON_CODE  = -2;			// queue for various control functions
	constexpr auto TXTOG_CODE = -3;

	static constexpr unsigned char ConvolutionCodeTable[32] =
	{
		2, 1, 3, 0, 3, 0, 2, 1,
		0, 3, 1, 2, 1, 2, 0, 3,
		1, 2, 0, 3, 0, 3, 1, 2,
		3, 0, 2, 1, 2, 1, 3, 0
	};
}

/* ------------------------------------------------------------------------- */
//Modulator
modulator::modulator( int sample_freq, int tx_freq )
	: m_sample_freq( sample_freq )
{
	m_PSKPhaseInc = m_2PI * tx_freq/sample_freq;		//carrier frequency
	m_PSKSecPerSamp = 1.0/sample_freq;
	m_PSKTime = 0.0;
	m_t = 0.0;
	m_Lastsymb = SYM_OFF;
	m_AddEndingZero = true;

	for(int i=0; i<16; i++)
		m_IQPhaseArray[i] = 1;
		m_VectLookup[SYM_NOCHANGE][0] = 0;
		m_VectLookup[SYM_NOCHANGE][1] = 1000;
		m_VectLookup[SYM_P90][0] = 1000;
		m_VectLookup[SYM_P90][1] = 0;
		m_VectLookup[SYM_P180][0] = 0;
		m_VectLookup[SYM_P180][1] = -1000;
		m_VectLookup[SYM_M90][0] = -1000;
		m_VectLookup[SYM_M90][1] = 0;
		m_VectLookup[SYM_OFF][0] = 0;
		m_VectLookup[SYM_OFF][1] = 0;
		m_VectLookup[SYM_ON][0] = 0;
		m_VectLookup[SYM_ON][1] = 1000;
		int i = 0;
		while(i<32)		//create post/preamble tables
		{
			m_Preamble[i] = TXTOG_CODE;
			m_Postamble[i++] = TXON_CODE;
		}
		m_Preamble[i] = 0;		// null terminate these tables
		m_Postamble[i] = 0;

		m_pPSKtxI = PSKShapeTbl_Z;
		m_pPSKtxQ = PSKShapeTbl_Z;
		m_PresentPhase = PHZ_OFF;
		m_TxShiftReg = 0;
		//m_TxCodeWord = 0;
		set_mode( mode::bpsk, baudrate::b31 );
		set_freqency( tx_freq );
}
/* ------------------------------------------------------------------------- */
//Set frequency
void  modulator::set_freqency( int frequency )
{
	m_PSKPhaseInc = m_2PI * frequency/m_sample_freq;
}
/* ------------------------------------------------------------------------- */
//Set mode
void modulator::set_mode( mode mmode, baudrate baud )
{
	m_mode = mmode;
	m_Ramp = 0;
	if( baud == baudrate::b63 )
	{
		m_SymbolRate = 62.5;
	}
	else if( baud == baudrate::b125 )
	{
		m_SymbolRate = 125;
	}
	else
	{
		m_SymbolRate = 31.25;
	}

	m_PSKPeriodUpdate = 1.0/m_SymbolRate;	//symbol period
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
	size_t RampSize =  (int)(m_sample_freq/m_SymbolRate); //  number of envelope ramp steps per symbol
	for( size_t i=0; i<MAXRAMP_SIZE; i++)
	{
		PSKShapeTbl_Z[i] = 0.0;
		PSKShapeTbl_P[i] = 1.0;
		PSKShapeTbl_M[i] = -1.0;
		PSKShapeTbl_PM[i] = cos( (double)i*m_2PI/(RampSize*2) );
		PSKShapeTbl_MP[i] = -PSKShapeTbl_PM[i];

		if( i <RampSize/2 )
		{
			PSKShapeTbl_PZ[i] = cos( (double)i*m_2PI/(RampSize*2) );
			PSKShapeTbl_MZ[i] = -PSKShapeTbl_PZ[i];
			PSKShapeTbl_ZP[i] = 0.0;
		}
		else
		{
			PSKShapeTbl_ZP[i] = -cos( (double)i*m_2PI/(RampSize*2) );
			PSKShapeTbl_PZ[i] = 0.0;
			PSKShapeTbl_MZ[i] = 0.0;
		}

	}
}

/* ------------------------------------------------------------------------- */
//Set char into the modulator
void modulator::put_tx( char txchar )
{
	constexpr char BACK_SPACE_CODE = 0x08;
	//EnterCriticalSection(&m_CriticalSection);
#if 0
	if( cntrl )	//is a tx control code
	{
		switch( txchar )
		{
			case TX_CNTRL_AUTOSTOP:
				m_TempNeedShutoff = TRUE;
				if( m_TXState==TX_TUNE_STATE )
					m_NeedShutoff = TRUE;
				break;
			case TX_CNTRL_ADDCWID:
				m_TempNeedCWid = TRUE;
				break;
			case TX_CNTRL_NOSQTAIL:
				m_TempNoSquelchTail = TRUE;
				break;
		}
	}
	else		//is a character to xmit
#endif
	{
		if( (txchar != BACK_SPACE_CODE) || (m_pHead==m_pTail) )
		{
			m_pXmitQue[m_pHead++] = txchar;
			if( m_pHead >= TX_BUF_SIZE )
				m_pHead = 0;
		}
		else	//see if is a backspace and if can delete it in the queue
		{
			if(--m_pHead < 0 )		//look at last character in queue
				m_pHead = 0;
			if( m_pXmitQue[m_pHead] == BACK_SPACE_CODE)
			{								//if another backspace, leave it there
				if(++m_pHead >= TX_BUF_SIZE )
					m_pHead = 0;
				m_pXmitQue[m_pHead++] = txchar;
				if( m_pHead >= TX_BUF_SIZE )
					m_pHead = 0;
			}
		}
	}
	//LeaveCriticalSection(&m_CriticalSection);
}
/* ------------------------------------------------------------------------- */
//Operator on new samples
void modulator::operator()( int16_t* sample, size_t len )
{
	int symbol;
	int v = 0;
	//Amplitude factor
	constexpr double m_RMSConstant = 22000;
	for( size_t i=0; i<len; i++ )		//calculate n samples of tx data stream
	{
		m_t += m_PSKPhaseInc;			// increment radian phase count
		// create sample from sin/cos and shape tables
		sample[i] = m_RMSConstant*( m_pPSKtxI[m_Ramp]* sin( m_t ) + m_pPSKtxQ[m_Ramp++]* cos( m_t ) );
		m_PSKTime += m_PSKSecPerSamp;
		if( m_PSKTime >= m_PSKPeriodUpdate )//if time to update symbol
		{
			m_PSKTime -= m_PSKPeriodUpdate;	//keep time bounded
			m_Ramp = 0;						// time to update symbol
			m_t = fmod(m_t,m_2PI);			//keep radian counter bounded
			switch( m_mode )				//get next symbol to send
			{
				case mode::bpsk:
					symbol = get_next_bpsk_symbol();
					break;
				case mode::qpsku:
					symbol = get_next_qpsk_symbol();
					break;
				case mode::qpskl:
					symbol = get_next_qpsk_symbol();
					//rotate vectors the opposite way
					if(symbol==SYM_P90) symbol = SYM_M90;
					else if(symbol==SYM_M90) symbol = SYM_P90;
					break;
				case mode::tune:
					symbol = get_next_tune_symbol();
					break;
			}
			//get new I/Q ramp tables and next phase
			m_pPSKtxI = PSKPhaseLookupTable[symbol][m_PresentPhase].iptr;
			m_pPSKtxQ = PSKPhaseLookupTable[symbol][m_PresentPhase].qptr;
			m_PresentPhase = PSKPhaseLookupTable[symbol][m_PresentPhase].next;
			m_IQPhaseArray[v++] = m_VectLookup[m_PresentPhase][0];
			m_IQPhaseArray[v++] = m_VectLookup[m_PresentPhase][1];
			v = v & 0x000F;	//keep bounded to 16
		}
	}
}


/* ------------------------------------------------------------------------- */
int modulator::get_tx_char()
{
	int ch;

	if( m_pHead != m_pTail )	//if something in Queue
	{
		ch = m_pXmitQue[m_pTail++] & 0x00FF;
		if( m_pTail >= TX_BUF_SIZE )
			m_pTail = 0;
	}
	else
		ch = TXTOG_CODE;		// if que is empty return TXTOG_CODE
	if(m_TempNeedShutoff)
	{
		m_TempNeedShutoff = false;
		m_NeedShutoff = true;
	}
	if(m_TempNoSquelchTail)
	{
		m_TempNoSquelchTail = false;
		m_NoSquelchTail = true;
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
			ch = TXOFF_CODE;		//else turn off
			m_NeedShutoff = false;
			break;
		case state::tune:
			ch = TXON_CODE;				// steady carrier
			if(	m_NeedShutoff)
			{

				m_state = state::off;
				m_AmblePtr = 0;
				ch = TXOFF_CODE;
				m_NeedShutoff = false;
			}
			break;
		case state::postamble:		// ending sequence
			if( !(ch = m_Postamble[m_AmblePtr++] ) || m_NoSquelchTail)
			{
				m_NoSquelchTail = false;
				m_state = state::off;
				m_AmblePtr = 0;
				ch = TXOFF_CODE;
				m_NeedShutoff = false;
			}
			break;
		case state::preamble:			//starting sequence
			if( !(ch = m_Preamble[m_AmblePtr++] ))
			{
				m_state = state::sending;
				m_AmblePtr = 0;
				ch = TXTOG_CODE;
			}
			break;
		case state::sending:		//if sending text from TX window
			ch = get_tx_char();
			if(	(ch == TXTOG_CODE) && m_NeedShutoff)
			{
				m_state = state::postamble;
			}
			else
			{
				//if( ch > 0 )
				//	::PostMessage(m_hWnd, MSG_PSKCHARRDY,ch,-1);
			}
			m_AmblePtr = 0;
			break;
	}
	return( ch );
}
/* ------------------------------------------------------------------------- */
modulator::sym modulator::get_next_bpsk_symbol()
{
	int ch;
	modulator::sym symb = m_Lastsymb;
	_internal::varicode varicode;
	if( m_TxShiftReg == 0 )
	{
		if( m_AddEndingZero )		// if is end of code
		{
			symb = SYM_P180;		// end with a zero
			m_AddEndingZero = false;
		}
		else
		{
			ch = get_char();			//get next character to xmit
			if( ch >=0 )			//if is not a control code
			{						//get next VARICODE codeword to send
				m_TxShiftReg = varicode.forward( ch );
				symb = SYM_P180;	//Start with a zero
			}
			else					// is a control code
			{
				switch( ch )
				{
				case TXON_CODE:
					symb = SYM_ON;
					break;
				case TXTOG_CODE:
					symb = SYM_P180;
					break;
				case TXOFF_CODE:
					symb = SYM_OFF;
					break;
				}
			}
		}
	}
	else			// is not end of code word so send next bit
	{
		if( m_TxShiftReg&0x8000 )
			symb = SYM_NOCHANGE;
		else
			symb = SYM_P180;
		m_TxShiftReg = m_TxShiftReg<<1;	//point to next bit
		if( m_TxShiftReg == 0 )			// if at end of codeword
			m_AddEndingZero = true;		// need to send a zero nextime
	}
	m_Lastsymb = symb;
	cout << "SYMB " << symb << endl;
	return symb;
}

/* ------------------------------------------------------------------------- */
modulator::sym modulator::get_next_qpsk_symbol()
{
	int ch;
	modulator::sym symb = (modulator::sym)ConvolutionCodeTable[m_TxShiftReg&0x1F];	//get next convolution code
	m_TxShiftReg = m_TxShiftReg<<1;
	if( m_TxCodeWord == 0 )			//need to get next codeword
	{
		if( m_AddEndingZero )		//if need to add a zero
		{
			m_AddEndingZero = false;	//end with a zero
		}
		else
		{
			ch = get_char();			//get next character to xmit
			if( ch >=0 )			//if not a control code
			{						//get next VARICODE codeword to send
				_internal::varicode varicode;
				m_TxCodeWord = varicode.forward( ch );
			}
			else					//is a control code
			{
				switch( ch )
				{
				case TXON_CODE:
					symb = SYM_ON;
					break;
				case TXTOG_CODE:
					m_TxCodeWord = 0;
					break;
				case TXOFF_CODE:
					symb = SYM_OFF;
					break;
				}
			}
		}
	}
	else
	{
		if(m_TxCodeWord&0x8000 )
		{
			m_TxShiftReg |= 1;
		}
		m_TxCodeWord = m_TxCodeWord<<1;
		if(m_TxCodeWord == 0)
			m_AddEndingZero = true;	//need to add another zero
	}
	return symb;
}

/* ------------------------------------------------------------------------- */
modulator::sym modulator::get_next_tune_symbol()
{
	modulator::sym symb;
	const int ch = get_char();			//get next character to xmit
	switch( ch )
	{
		case TXON_CODE:
			symb = SYM_ON;
			break;
		default:
			symb = SYM_OFF;
			break;
	}
	return symb;
}
/* ------------------------------------------------------------------------- */
//Clear queue
void modulator::clear_tx()
{
	//EnterCriticalSection(&m_CriticalSection);
	m_pTail = m_pHead = 0;
	//LeaveCriticalSection(&m_CriticalSection);
	m_NoSquelchTail = false;
	m_TempNeedShutoff = false;
	m_TempNoSquelchTail = false;
}
//Get number of chars remaining
size_t modulator::size_tx() const
{
	//EnterCriticalSection(&m_CriticalSection);
	int num = m_pHead - m_pTail;
	//LeaveCriticalSection(&m_CriticalSection);
	if( num < 0 )
		num = num + TX_BUF_SIZE;
	return num;
}
/* ------------------------------------------------------------------------- */
} /* namespace psk */
} /* namespace ham */
/* ------------------------------------------------------------------------- */
