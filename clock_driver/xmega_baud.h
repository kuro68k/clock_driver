/*
 * xmega_baud.h
 *
 * Created: 04/04/2016 15:05:53
 *  Author: omegacs
 *
 *	https://web.archive.org/web/20130601050533/http://blog.omegacs.net/2010/08/18/xmega-fractional-baud-rate-source-code/
 *
 * Example:
 *	#define BAUDRATE 115200
 *
 *	USARTC0.BAUDCTRLA = BSEL(F_CPU,BAUDRATE) & 0xff;
 *	USARTC0.BAUDCTRLB = (BSCALE(F_CPU,BAUDRATE) << USART_BSCALE0_bp) | (BSEL(F_CPU,BAUDRATE) >> 8);
 */ 

#ifndef XMEGA_BAUD_H_
#define XMEGA_BAUD_H_


#define _BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,bscale) (					\
((bscale) < 0) ?														\
  (int)((((float)(f_cpu)/(8*(float)(baud)))-1)*(1<<-(bscale)))			\
: (int)((float)(f_cpu)/((1<<(bscale))*8*(float)(baud)))-1 )

#define _BSCALE(f_cpu,baud) (											\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-7) < 4096) ? -7 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-6) < 4096) ? -6 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-5) < 4096) ? -5 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-4) < 4096) ? -4 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-3) < 4096) ? -3 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-2) < 4096) ? -2 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,-1) < 4096) ? -1 :				\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,0) < 4096) ? 0 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,1) < 4096) ? 1 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,2) < 4096) ? 2 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,3) < 4096) ? 3 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,4) < 4096) ? 4 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,5) < 4096) ? 5 :					\
(_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,6) < 4096) ? 6 :					\
7 )

#define BSEL(f_cpu,baud)												\
	_BAUD_BSEL_FROM_BAUDSCALE(f_cpu,baud,_BSCALE(f_cpu,baud))

#define BSCALE(f_cpu,baud) ((_BSCALE(f_cpu,baud)<0) ? (16+_BSCALE(f_cpu,baud)) : _BSCALE(f_cpu,baud))


#endif /* XMEGA_BAUD_H_ */