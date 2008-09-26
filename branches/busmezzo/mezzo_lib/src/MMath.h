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

//-*-c++-*------------------------------------------------------
// FILE: Math.h
// NOTE: Some useful math functions
// AUTH: Qi Yang
// DATE: Thu Sep 21 22:09:16 1995
//--------------------------------------------------------------

#ifndef MATH_HEADER
#define MATH_HEADER

#include <limits.h>

#ifdef RogueWave
#include <rw/math.h>
#else
#include <math.h>
#endif

const double	AproxEpsilon = 1.0E-5;
#ifndef M_PI
const double M_PI=3.14159;    // for the Borland C++ 5.5.1 compiler under win32
#endif
const double	TWO_PI  = M_PI * 2.0;
const double	ONE_PI  = M_PI;
const double	HALF_PI = M_PI * 0.5;

const double	RADIAN_PER_DEGREE = M_PI / 180.0;
const double	DEGREE_PER_RADIAN = 180.0 / M_PI;

const int	INT_INF = INT_MAX / 3;
//const float	FLT_INF = FLT_MAX / 3.0;
//const double	DBL_INF = DBL_MAX / 3.0;
const float	FLT_INF = float(INT_MAX / 3.0);
const double	DBL_INF = INT_MAX / 3.0;

#ifndef FLT_EPSILON
const double	FLT_EPSILON = 1.0 / FLT_INF;
#endif

#ifndef DBL_EPSILON
const double	DBL_EPSILON = 1.0 / DBL_INF;
#endif

#ifndef MACRO

template <class T>
inline T Sign(T e)
{
   if (e > 0) return 1;
   else if (e < 0) return -1;
   else return 0;
}

template <class T>
inline T Abs(T e)
{
   return ((e > 0) ? e : -e);
}

template <class T>
inline T Max(T a, T b)
{
   return ((a > b) ? a : b);
}

template <class T>
inline T Min(T a, T b)
{
   return ((a < b) ?  a : b);
}

template <class T>
inline int Round(T x) 
{
   return (int) (x + .5);
}

template <class T>
inline T Fix(T x, T d) 
{
   return  ((long int) (x / d + .5)) * d;
}

template <class T>
inline int Comp(T a, T b)
{
   if (a < b) return -1;
   else if (a > b) return 1;
   else return 0;
}

template <class T>
inline int AproxEqual(T a, T b, double epsilon = AproxEpsilon) 
{
   return (Abs(a-b) < epsilon);
}

template <class T>
inline int Smaller(T a, T b, double epsilon = AproxEpsilon)
{
   return (a + epsilon < b);
}

template <class T>
inline int Greater(T a, T b, double epsilon = AproxEpsilon)
{
   return (a > b + epsilon);
}

inline long unsigned int factorial (long unsigned int a)
{
	long unsigned int result = 1;
	if (a < 2)
	{
		return result;
	}
	for (long unsigned int i=1; i<=a; i++)
	{
		result = result * i;
	}
	return result;
}

// Ceil finds the smallest number which is not less than d and rounded
// at step.  For example:
//
//     Ceil(22.3, 5.0) = 25.0
//
//     Ceil(2.6, 0.5) = 3.0

template <class T>
inline T Ceil(T d, T step)
{
   return ceil(d / step) * step;
}

// Floor finds the largest number which is not greater than d and
// rounded at step.  For example:
//
//     Floor(22.3, 5.0) = 20.0
//
//     Floor(2.6, 0.5) = 2.5

template <class T>
inline T Floor(T d, T step)
{
   return floor(d / step) * step;
}

inline double Radian(double degree)
{
   return degree * RADIAN_PER_DEGREE;
}

inline double Degree(double radian)
{
   return radian * DEGREE_PER_RADIAN;
}

#else  // MACRO

#define Sign(e)		(((e) > 0) ? 1 : (((e) == 0) ? : 0 : -1))
#define Abs(e)		(((e) > 0) ? (e) : -(e))
#define Max(a, b)	(((a) > (b)) ? (a) : (b))
#define Min(a, b)	(((a) < (b)) ? (a) : (b))

#define Round(x) 	((int) ((x) + .5))
#define Fix(x, d)	(((long int) ((x) / (d) + .5)) * (d))

#define Comp(a, b)	(((a) > (b)) ? 1 : (((a) == (b)) ? 0 : -1))
#define Ceil(d, s)	(ceil((d) / (s)) * (s))
#define Floor(d, s)	(floor((d) / (s)) * (s))
#define Radian(d)	((d) * RADIAN_PER_DEGREE)
#define Degree(r)	((r) * DEGREE_PER_RADIAN)

#define AproxEqual(a, b) (Abs((a)-(b)) < AproxEpsilon)
#define Smaller(a, b)	((a) + AproxEpsilon < (b))
#define Greater(a, b)	((a) > (b) + AproxEpsilon)

#endif // MACRO

#endif // BasicMath
