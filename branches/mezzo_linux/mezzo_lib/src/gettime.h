#ifndef _TIME_HH
#define _TIME_HH

#include <sys/timeb.h>
#include <time.h>

double timestamp ()
/*
	After some hours of digging through the UNIX and LINUX legacy of time
	standards and functions, i used the simplest in sys/timeb.h and converted it
	to return the number of seconds and msecs since 00:00:00 january 1 1970.
	It returns thus a double with a timestamp (seconds).
*/
{
	timeb* tb=new timeb;
	ftime(tb);
	double seconds=double (tb->time);
	double ms=tb->millitm;
	delete tb;
	return (seconds*1000 + ms)/1000;
}

double msecs()
{
	timeb* tb=new timeb;
	ftime(tb);
   double ms=tb->millitm;
	delete tb;
	return ms;
}

char* timestring()
{
 time_t* tt=new time_t;
 time(tt);
 char* stri=new char [30];

// ctime_r(tt,stri);

 strftime (stri,30,"%Y-%m-%d\t%T",gmtime(tt));
 return stri;

}

#endif

