#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include "dsp/fft.h"

/* ---------------------------------------------------------------- */
template <typename T, typename K >
void plot( const T *x, const K *y, int len, const char *title)
{
    char name[L_tmpnam];
    char cmd[L_tmpnam+512];
    if(!tmpnam(name))
    {
        perror("mktemp");
        exit(-1);
    }
    std::ofstream os( name ,std::ofstream::binary );
    if(!os.good())
    {
        perror("Open tmp file");
        exit(-1);
    }
    for(int i=0;i<len;i++)
    {
        os << (x?x[i]:i) << " " << (y[i]) << std::endl;
    }
    strcpy(cmd,"echo 'plot \"");
    strcat(cmd,name);
    strcat(cmd,"\" title \"");
    strcat(cmd,title);
    strcat(cmd,"\" with lines' | gnuplot -persist");
    if(system(cmd)<0)
    {
        perror("system");
        exit(-1);
    }
    if(remove(name)!=0)
    {
       perror("system");
       exit(-1);

    }
}

/* ---------------------------------------------------------------- */


float cplx_abs( std::complex<short> v )
{
    return sqrt( v.real() * v.real() + v.imag() * v.imag() );
}

/* ---------------------------------------------------------------- */

constexpr auto N_SAMPL = 1024*1;
constexpr auto FREQUENCY = N_SAMPL / 10;

int main()
{
	using namespace dsp::refft;
	short y[N_SAMPL*2];
	std::complex<short> yc[N_SAMPL*2];
	float t[N_SAMPL];
	float abst[N_SAMPL*2];
	for(int s=0; s< N_SAMPL; s++ )
    {
	    if( s > N_SAMPL/10)
		y[s] =  (sin((2*M_PI/N_SAMPL)*s*FREQUENCY) + 0.5*sin( (2*M_PI/N_SAMPL)*s*FREQUENCY*2)) * 15000;
	    else
        y[s] = 0;
	    yc[s].real( y[s] );
        yc[s].imag ( 0 );
        t[s] = s/(float)N_SAMPL;
    }
	plot( t, y, N_SAMPL , "SI");
    if( 0 )
    {
		std::complex<short> x[N_SAMPL];
		bzero(x, sizeof x);
		fft_complex(x, yc, log2(N_SAMPL) );
		for(int s=0; s< N_SAMPL; s++ )
		  abst[s] = cplx_abs( x[s] );
		plot( t, abst, N_SAMPL , "FFT");
    }
    else
    {
    	std::complex<short> x[N_SAMPL];
    	fft_real( x, y, log2(N_SAMPL));
    	for(int s=0; s< N_SAMPL; s++ )
    		abst[s] = cplx_abs( x[s] );
    	plot( t, abst, N_SAMPL , "FFT");
    }
}

