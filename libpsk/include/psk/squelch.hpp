/*
 * squelch.hpp
 *
 *  Created on: 04-05-2013
 *      Author: lucck
 */

#ifndef PSK_SQUELCH_HPP_
#define PSK_SQUELCH_HPP_

/* ------------------------------------------------------------------------- */
namespace ham {
namespace psk {
/* ------------------------------------------------------------------------- */
//Squelch tresh type
typedef int sqelch_value_type;
//Squelch mode
enum class squelch_mode
{
	slow,
	fast
};
/* ------------------------------------------------------------------------- */
namespace _internal {
/* ------------------------------------------------------------------------- */
/** Squelch class calculate signal quality and setup the squelch */
class squelch
{
	//Make object noncopyable
	squelch(const squelch&) = delete;
	squelch& operator=(const squelch&) = delete;
	static constexpr auto PI2 = 8.0 * std::atan(1.0);
	static constexpr auto PHZ_180_QMIN	= PI2/8.0;		// Pi/4
	static constexpr auto PHZ_180_QMAX	= 3.0*PI2/8.0;	// 3Pi/4
	static constexpr auto PHZ_0_QMIN	= 5.0*PI2/8.0;		// 5Pi/4
	static constexpr auto PHZ_0_QMAX	= 7.0*PI2/8.0;		// 7Pi/4
	static constexpr auto PHZ_180_BMIN = 0.0;			// 0
	static constexpr auto PHZ_180_BMAX = PI2/2.0;		// Pi
	static constexpr auto PHZ_0_BMIN	= PI2/2.0;			// Pi
	static constexpr auto PHZ_0_BMAX	= PI2;
public:
	//Constructor
	squelch()
	{}
	//Set squelch tresh
	void set_tresh( sqelch_value_type thresh, squelch_mode mode )
	{
		m_sq_thresh = thresh;
		if( mode == squelch_mode::fast ) m_squelch_speed = 20;
		else if( mode == squelch_mode::slow ) m_squelch_speed = 75;
	}
	//Get squelch level
	int get_level() const
	{
		return m_sql_level>0?m_sql_level:0;
	}
	//Is squelch open
	bool is_open() const
	{
		return m_sq_open;
	}
	//IMD is valid
	bool is_imd_valid() const
	{
		return m_imd_valid;
	}
	//Reset squelch
	void reset()
	{
		m_sql_level = 10;
		m_dev_ave = 90.0;
	}
	//Reset frequency
	void reset_frequency()
	{
		m_pcnt = 0;
		m_ncnt = 0;
	}
	// Squelch functor return signal level
	double operator()( double angle, int agc_value, bool is_qpsk, bool is_qpsk_lsb, bool is_bpsk )
	{
		constexpr auto ELIMIT = 5;
		double temp;
		double SqTimeK =  double(m_squelch_speed);
		if( (is_qpsk && ((angle >= PHZ_180_QMIN) && (angle <= PHZ_180_QMAX) ) ) ||
		( !is_qpsk && ((angle >= PHZ_180_BMIN) && (angle <= PHZ_180_BMAX))) )
		{	//look +/-45 or +/-180 deg. around 180 deg.
			if( is_qpsk_lsb )
				temp = PI2/4.0 - angle;
			else
				temp = angle - PI2/4.0;
			m_q_freq_error = temp;
			if( is_qpsk ) //if QPSK
				temp = 280.0*std::abs(temp);
			else
				temp = 150.0*std::abs(temp);
			if( temp < m_dev_ave)
				m_dev_ave=  (1.0-1.0/SqTimeK)*m_dev_ave + (1.0/SqTimeK)*temp;
			else
				m_dev_ave =  (1.0-1.0/(SqTimeK*2.0))*m_dev_ave + (1.0/(SqTimeK*2.0))*temp;
			if(m_on_count > 20 )		// fast squelch counter
				m_dev_ave = 100.0-75.0;	//set to 75%
			else
				m_on_count++;
			m_off_count = 0;
			if( m_q_freq_error >= 0.0 )
			{
				m_pcnt++;
				m_ncnt = 0;
			}
			else
			{
				m_ncnt++;
				m_pcnt = 0;
			}
			if( (m_pcnt<ELIMIT) && (m_ncnt<ELIMIT) )
					m_q_freq_error = 0.0;
		}
		else
		{
			if( (is_qpsk && ((angle >= PHZ_0_QMIN) && (angle <= PHZ_0_QMAX) ) ) ||
				(!is_qpsk && ((angle >= PHZ_0_BMIN) && (angle <= PHZ_0_BMAX) ) ) )

			{		//look +/-45 or +/- 180 deg. around 0 deg.
				if( is_qpsk_lsb )
					temp = 3*PI2/4.0 - angle;
				else
					temp = angle - 3*PI2/4.0;
				m_q_freq_error = temp;
				if( is_qpsk ) //if QPSK
					temp = 280.0*std::abs(temp);
				else
					temp = 150.0*std::abs(temp);
				if( temp < m_dev_ave)
					m_dev_ave =  (1.0-1.0/SqTimeK)*m_dev_ave + (1.0/SqTimeK)*temp;
				else
					m_dev_ave =  (1.0-1.0/(SqTimeK*2.0))*m_dev_ave + (1.0/(SqTimeK*2.0))*temp;
				if(m_off_count > 20 )	// fast squelch counter
					if( is_bpsk ) //if BPSK
						m_dev_ave = 100.0 - 0.0;		//set to 0%
				else
					m_off_count++;
				m_on_count = 0;
				if( m_q_freq_error >= 0.0 )
				{
					m_pcnt++;
					m_ncnt = 0;
				}
				else
				{
					m_ncnt++;
					m_pcnt = 0;
				}
				if( (m_pcnt<ELIMIT) && (m_ncnt<ELIMIT) )
					m_q_freq_error = 0.0;
			}

		}
		m_imd_valid = (m_on_count >2);

		if( agc_value > 10.0 )
		{
			if( is_qpsk ) //if QPSK
				m_sql_level = 100 - int(m_dev_ave);
			else
				m_sql_level = 100 - int(m_dev_ave);
			m_sq_open = ( m_sql_level >= m_sq_thresh );
		}
		else
		{
			m_sql_level = 0;
			m_sq_open = false;
		}
		if(is_qpsk)
		{
			if( m_q_freq_error > .6 )//  clamp range to +/- 3 Hz
				m_q_freq_error = .6;
			if( m_q_freq_error < -.6 )
				m_q_freq_error = -.6;
		}
		else
		{
			if( m_q_freq_error> 1.0 )//  clamp range to +/- 5 Hz
					m_q_freq_error = 1.0;
			if( m_q_freq_error < -1.0 )
				m_q_freq_error = -1.0;
		}
		return m_q_freq_error;
	}
//Private data squelch members
private:
	int m_squelch_speed { 75 };
	double m_q_freq_error {};
	double m_dev_ave;
	int m_on_count {};
	int m_off_count {};
	int m_pcnt {};
	int m_ncnt {};
	bool m_imd_valid {};
	int m_sql_level { 10 };
	bool m_sq_open {};
	int m_sq_thresh { 50 };
};
/* ------------------------------------------------------------------------- */
}}}
/* ------------------------------------------------------------------------- */
#endif /* SQUELCH_HPP_ */
/* ------------------------------------------------------------------------- */
