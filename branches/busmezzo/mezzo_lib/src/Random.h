//-*-c++-*------------------------------------------------------------
// FILE: Random.h
// AUTH: Qi Yang
// MODIFIED BY: Wilco Burghout 19-7-2001
// DATE: Wed Oct 18 23:03:53 1995
//--------------------------------------------------------------------

#ifndef RANDOM_HEADER
#define RANDOM_HEADER

#include <vector>
using namespace std;

class Random
{
 // friend class CmdArgsParser;

protected:

  static unsigned int flags_;
  unsigned int signature_;
  long int seed_;

public:

  enum {
	Misc      = 0,
	Departure = 1,
	Routing   = 2,
	Behavior  = 3
  };

  static void create(int n);	// create random numbers

  Random();
  virtual ~Random() { }

  static inline unsigned int flags() { return flags_; }
  static inline void flags(unsigned int s) { flags_ = s; }

  long int seed() { return seed_; }
  void seed(long int s) { seed_ = s; }

  // Set random seed

  long int randomize();

  // uniform (0, 1]

  double urandom(void);

  // uniform [0, n)

  int urandom(int n);

  // uniform (a, b]

  double urandom(double a, double b);

  // returns 1 with probability p

  int brandom(double p);

  // exponential with parameter r

  double erandom(double r);
  double rrandom(double one_by_r);

  // normal with mean m and stddev v

  double nrandom();
  double nrandom_trunc(double r);
  double nrandom(double m, double v);
  double nrandom_trunc(double m, double v, double r = 3.0);

  // lognormal with mean m and stddev v

  double lnrandom(double m, double v);

  // discrete random number in [0, n) with given CDF

  int drandom(int n, double cdf[]);
  int drandom(int n, float cdf[]);
  
  //poission with parameter lambda
	
  int poisson (double lambda);

  // randomly permute an array

  void permute(int n, int* perm);
};

//extern vector<Random*> theRandomizers;

#endif
