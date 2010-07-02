/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef _TIME_HH
#define _TIME_HH

#include <sys/timeb.h>
#include <time.h>

const double timestamp ()
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

const double msecs()
{
	timeb* tb=new timeb;
	ftime(tb);
   double ms=tb->millitm;
	delete tb;
	return ms;
}

const char* const timestring()
{
 time_t* tt=new time_t;
 time(tt);
 char* stri=new char [30];
 strftime (stri,30,"%Y-%m-%d\t%T",gmtime(tt));
 return stri;

}

#endif

