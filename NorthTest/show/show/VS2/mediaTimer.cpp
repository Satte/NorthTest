/*
* Copyright (c) 1995 Regents of the University of California.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
* 1. Redistributions of source code must retain the above copyright
*    notice, this list of conditions and the following disclaimer.
* 2. Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the
*    documentation and/or other materials provided with the distribution.
* 3. All advertising materials mentioning features or use of this software
*    must display the following acknowledgement:
*	This product includes software developed by the Computer Systems
*	Engineering Group at Lawrence Berkeley Laboratory.
* 4. Neither the name of the University nor of the Laboratory may be used
*    to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*/
#include <sys/timeb.h>
#include <math.h>
#include <sys/time.h>
#include "mediaTimer.h"

/*
* Default media timestamp -- convert unix system clock
* into a 90Khz timestamp.  Grabbers override this virtual
* method if they can provide their own time base.
*
* XXX
* We save the corresponding unix time stamp to handle the
* unix_ts() call the transmitter will make to get the correspondence
* between the media timestamp & unix time.
*/


uint32_t MediaTimer::GetTickCount(void)
{
    struct timeval tv;
    if(gettimeofday(&tv, NULL) != 0)
        return 0;

    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}

unsigned long MediaTimer::media_ts()
{
	timeval tv;
	gettimeofday(&tv, 0);
	unsigned long u = tv.tv_usec;
	u = (u << 3) + u; /* x 9 */
	/* sec * 90Khz + (usec * 90Khz) / 1e6 */
	u = tv.tv_sec * 90000 + (u / 100);
	return u;
}

/*
* compute media time corresponding to the current unix time.
* in this generic routine, this is the same as media_ts() but,
* if a grabber has hardware or kernel timestamping, this routine
* must compute the correspondence between the hardware timestamp
* and the unix clock and appropriately offset the timestamp to
* correspond to the current clock.  (This information if vital
* for cross-media synchronization.)
*/


unsigned long MediaTimer::gettimeofday(struct timeval *tv, struct timezone *z)
{
	struct timeb t;

	ftime(&t);
	tv->tv_sec = t.time;
	tv->tv_usec = t.millitm * 1000;
	return 0;
}

double MediaTimer::gettimeofday()
{
	timeval tv;
	gettimeofday(&tv, 0);
	return (1e6 * double(tv.tv_sec) + double(tv.tv_usec));
}

timeval MediaTimer::unixtime()
{
	timeval tv;
	gettimeofday(&tv,0);
	return (tv);
}

unsigned int MediaTimer::usec2ntp(unsigned int usec)
{
	unsigned int t = (usec * 1825) >> 5;
	return ((usec << 12) + (usec << 8) - t);
}

ntp64 MediaTimer::ntp64time(timeval tv)
{
	ntp64 n;
	n.upper = (unsigned int)tv.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
	n.lower = usec2ntp((unsigned int)tv.tv_usec);
	return (n);
}

unsigned int MediaTimer::ntptime(timeval t)
{
	unsigned int s = (unsigned int)t.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
	return (s << 16 | usec2ntp((unsigned int)t.tv_usec) >> 16);
}

