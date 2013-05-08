/*
 * modulator.hpp
 *
 *  Created on: 08-05-2013
 *      Author: lucck
 */

/* ------------------------------------------------------------------------- */
#ifndef LIBPSK_MODULATOR_HPP_
#define LIBPSK_MODULATOR_HPP_
/* ------------------------------------------------------------------------- */
#include <cstddef>
#include <cstdint>
/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {

/* ------------------------------------------------------------------------- */
/* PSK modulator class */
class modulator {
	//Make object noncopyable
	modulator(const modulator&) = delete;
	modulator& operator=(const modulator&) = delete;
public:
	//Enum special chars
	enum class mode
	{
		bpsk,
		qpsku,
		qpskl,
		tune,
	};
	enum class state
	{
		off,
		sending,
		paused,
		preamble,
		postamble,
		tune
	};
	enum class baudrate
	{
		b31,
		b63,
		b125
	};
	//Constructor
	explicit modulator( int sample_freq, int tx_freq );
	//Operator on new samples
	void operator()( int16_t* sample, size_t len );
	//Set char into the modulator
	void put_tx( char c );
	//Clear queue
	void clear_tx();
	//Set frequency
	void set_freqency( int frequency );
	//Set mode
	void set_mode( mode mmode, baudrate baud );
	//Get number of chars remaining
	size_t size_tx() const;
	//Set auto shut off
	void set_auto_shutoff( bool en = true )
	{
		m_NeedShutoff = en;
	}
	//Get tx state
	state get_state() const
	{
		return m_state;
	}
private:
	enum sym {
		SYM_NOCHANGE,	//Stay the same phase
		SYM_P90,		//Plus 90  deg
		SYM_P180,		//Plus 180 deg
		SYM_M90,		//Minus 90 deg
		SYM_OFF,		//No output
		SYM_ON			//constant output
	};
private:
	int get_tx_char();
	int get_char();
	modulator::sym get_next_bpsk_symbol();
	modulator::sym get_next_qpsk_symbol();
	modulator::sym get_next_tune_symbol();
private:
	const int m_sample_freq;
	mode m_mode { mode::bpsk };
	double m_t {};
	double m_PSKPhaseInc;
	double* m_pPSKtxI {};
	double* m_pPSKtxQ {};
	int m_Ramp {};
	double m_PSKSecPerSamp {};
	double m_PSKTime {};
	double m_PSKPeriodUpdate {};
	short m_PresentPhase {};
	int m_IQPhaseArray[20] {};
	int m_VectLookup[6][2];
	sym m_Lastsymb {};
	bool  m_AddEndingZero {};
	short m_TxShiftReg {};
	state m_state;
	bool m_NeedShutoff { true };
	int m_AmblePtr {};
	int m_Postamble[33] {};
	int m_Preamble[33] {};
	bool m_NoSquelchTail {};
	int m_pTail {};
	int m_pHead {};
	static constexpr auto  TX_BUF_SIZE = 256;
	int m_pXmitQue[TX_BUF_SIZE];
	bool m_TempNeedShutoff {};
	bool m_TempNoSquelchTail {};
	double m_SymbolRate { 31.25 };
	short m_TxCodeWord {};
};

/* ------------------------------------------------------------------------- */

} /* namespace psk */
} /* namespace ham */

/* ------------------------------------------------------------------------- */
#endif /* MODULATOR_HPP_ */

/* ------------------------------------------------------------------------- */
