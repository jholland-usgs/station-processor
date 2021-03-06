#include "../../include/dcc_std.h"
#include "../../include/dcc_time.h"

#include <time.h>
#include <math.h>

extern UBYTE _dmsize[12];

_SUB long ST_GetUnix(STDTIME instd)
{

  LONG unixinst,curdat,cval;

  STDTIME unixdat = { 1970, 1, 0,0,0,0 };

  unixinst = ST_GetJulian(unixdat);
  curdat = ST_GetJulian(instd);

  if (instd.year<1970) return(0);

  cval = curdat - unixinst;
  cval *= 86400;
  cval += instd.hour * (60*60);
  cval += instd.minute * (60);
  cval += instd.second;

  return(cval);
	
}

_SUB long ST_GetUnixTest(STDTIME instd)
{

	struct tm intime;
	time_t tloc;
	WORD mon,day,t;

	if (instd.year<1970) return(0);

	intime.tm_year = instd.year - 1900;
	
	intime.tm_hour = instd.hour;
	intime.tm_min = instd.minute;
	intime.tm_sec = instd.second;

	ST_CnvJulToCal(ST_GetJulian(instd),
		       &t,&mon,&day,&t);

	intime.tm_mday = day;
	intime.tm_mon = mon - 1;

	tloc = mktime(&intime);

	return((long) tloc);

}

_SUB double ST_GetDblUnix(STDTIME instd)
{

  double rval,ms;

  rval = ST_GetUnix(instd);
  ms = instd.msec;
  ms /= 1000.0;
  rval += ms;

  return(rval);

}

_SUB STDTIME ST_CnvUnixtoSTD(long intim)
{

  struct tm *intime;
  time_t tloc;

  STDTIME rettime;
  WORD i,ct,mon,day;

  tloc = (time_t) intim;

  intime = gmtime(&tloc);

  rettime.year = intime->tm_year + 1900;
  mon = intime->tm_mon + 1;
  day = intime->tm_mday;
  rettime.hour = intime->tm_hour;
  rettime.minute = intime->tm_min;
  rettime.second = intime->tm_sec;
  rettime.msec = 0;

  _dmsize[1] = _tleap(rettime.year)?29:28;

  ct=0;
  for (i=0; i<(mon-1); i++) ct+=_dmsize[i];

  ct += day;

  rettime.day = ct;

  return(rettime);

}

_SUB STDTIME ST_CnvUnixDbltoSTD(double intim)
{

  STDTIME rtim;
  double secpart;

  rtim = ST_CnvUnixtoSTD((long) intim);
  
  secpart = intim - floor(intim);

  secpart *= 1000.0;

  rtim.msec = secpart + .5;	/* Round */

  return(rtim);

}
