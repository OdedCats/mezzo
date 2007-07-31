//-*-c++-*------------------------------------------------------------
// FILE: Random.cc
// AUTH: Qi Yang
// DATE: Wed Oct 18 23:09:54 1995
//--------------------------------------------------------------------

#include "Random.h"
#include "MMath.h"
//#include "gettime.h"
#include <sys/timeb.h>
#include <time.h>
#include <new>

vector<Random*> theRandomizers;
unsigned int Random::flags_ = 0;

void Random::create(int n) {
  for (int i = 0; i < n; i ++) {
	theRandomizers.push_back(new Random);
	theRandomizers[i]->randomize();
  }
}

Random::Random()
   : seed_(0L)
{
  static int counter = 0;
  signature_ = counter;
  counter ++;
}


// Set random seed based on clock time.

long int Random::randomize()
{


  unsigned int s = 0xFF << (signature_ * 8);
  if (!(seed_ = (flags_ & s))) {
	//seed_ = (time(0)*20000)^time(0);
	timeb* tb=new timeb;
	ftime(tb);
	double seconds=double(tb->time);
	double ms=tb->millitm;
	seed_=static_cast <int> (seconds) * static_cast <int> (ms);
  }
  return seed_;
}


// Prime Modulus Multiplicative Linear Congruential Generator
//
// Modified version of the Random number generator proposed by Park &
// Miller in "Random Number Generators: Good Ones Are Hard to Find"
// CACM October 1988, Vol 31, No. 10
//
//  - Modifications proposed by Park to provide better statistical
//    properties (i.e. more "random" - less correlation between sets of
//    generated numbers)
//
//  - generator is of the form
//        x = ( x * A) % M
//
//  - Choice of A & M can radically modify the properties of the generator
//    the current values were chosen after followup work to the original
//    paper mentioned above.
//
//  - The generator has a period of 2^31 - 1 with numbers generated in the
//    range of 0 < x < M
//
//  - The generator can run on any machine with a 32-bit integer, without
//    overflow.
//
// John Burton                                                          
// G & A Technical Software, Inc                                        
// 28 Research Drive                                                    
// Hampton, Va. 23666                                                   
// 
// jcburt@cs.wm.edu                                                     
// jcburt@gatsibm.larc.nasa.gov
// burton@asdsun.larc.nasa.gov                                          

double
Random::urandom(void)
{
   // Constants for linear congruential random number generator.

   const long int M = 2147483647;  // M = modulus (2^31)
   const long int A = 48271;       // A = multiplier (was 16807)
   const long int Q = M / A;
   const long int R = M % A;

   seed_ = A * (seed_ % Q) - R * (seed_ / Q);
   seed_ = (seed_ > 0) ? (seed_) : (seed_ + M);

   return (double)seed_ / (double)M;
}


// Returns a standard normal distributed random number

double
Random::nrandom()
{
   double r1 = urandom(), r2 = urandom();
   double r = - 2.0 * log(r1);
   if (r > 0.0) return sqrt(r) * sin(TWO_PI * r2);
   else return 0.0;
}


double
Random::nrandom_trunc(double r)
{
   double x = nrandom();
   if (x >= -r && x < r) return x;
   else return urandom(-r, r);
}


// Returns normally distributed random number with parameters mean and
// stddev.

double
Random::nrandom(double mean, double stddev)
{
   double r1 = urandom(), r2 = urandom();
   double r = - 2.0 * log(r1);
   if (r > 0.0) return (mean + stddev * sqrt(r) * sin(TWO_PI * r2));
   else return (mean);
}

double
Random::nrandom_trunc(double mean, double stddev, double r)
{
   double x = nrandom(mean, stddev);
   double dx = r * stddev;
   if (x >= mean - dx && x <= mean + dx) {
      return x;
   } else {
     return mean + urandom(-dx, dx);
   }
}



 // lognormal with mean m and stddev v
 // generates a normal random variate with mean m' and stddev v'

double Random::lnrandom(double m, double v)
{
 double mu_=log(m*m/sqrt((m*m)+(v*v)));
 double sd_=sqrt(log(1+(v*v)/(m*m)));
 double result=exp(nrandom(mu_,sd_));
 return result;
}



// Generates an integer random number that is uniformly distributed
// between [0, n).

int
Random::urandom(int n)
{
   return ((int)(urandom() * (double)n));
}


// Returns 1 with probability p and 0 with probability 1-p.

int
Random::brandom(double prob)
{
   if (urandom() < prob) return (1);
   else return 0;
}


// Generates a random number uniformly distributed between (a, b].

double
Random::urandom(double a, double b)
{
   return a + (b-a) * urandom();
}


// Generates a random number of negative exponential distribution
// with paramenter lambda.

double
Random::erandom(double lambda)
{
   return -log(urandom()) / lambda;
}


double
Random::rrandom(double one_by_lambda)
{
   return -log(urandom()) * one_by_lambda;
}


// Generates an integer random number between [0, n) based on the CDF
// table.

int
Random::drandom(int n, double cdf[])
{
   register int i;
   double r = urandom();
   for (n = n - 1, i = 0; i < n && r > cdf[i]; i ++);
   return i;
}

int
Random::drandom(int n, float cdf[])
{
   register int i;
   double r = urandom();
   for (n = n - 1, i = 0; i < n && r > cdf[i]; i ++);
   return (i);
}


// Given as input the numbers: 0..N-1 it returns a random permutation
// of those numbers This is achieved in a single pass

void
Random::permute(int n, int *perm)
{
   register int i;
   int r, tmp;

   for (i = 0; i < n; i ++)  perm[i] = i;

   for (i = n - 1; i >= 0; i --) {
      r = urandom(i);
      tmp = perm[i];
      perm[i] = perm[r];
      perm[r] = tmp;
   }
}
