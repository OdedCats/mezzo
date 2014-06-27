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

  // returns 1 with probability p (Bernoulli generator)

  bool brandom(double p);

  // binomial generator - n trials with probability p 

  int binrandom (int n, double p); // using a cdf inverse
  int binrandom1 (int n, double p); // using calls to Bernoulli

  // decision from a set of alternatives 
  int mrandom (vector<double> probs);

  // exponential with parameter r

  double erandom(double r);
  double rrandom(double one_by_r);

  // normal with mean m and stddev v

  double nrandom();
  double nrandom_trunc(double r);
  double nrandom(double m, double v);
  double nrandom_trunc(double m, double v, double r);

  // lognormal with mean m and stddev v

  double lnrandom(double m, double v);
  double lnrandom1(double m, double v);

  // loglogistic with scale parameter alpha and shape parameter beta
  double loglogisticrandom(double alpha, double beta);

  // discrete random number in [0, n) with given CDF

  int drandom(int n, double cdf[]);
  int drandom(int n, float cdf[]);
  
  //poission generator with parameter lambda (rate)
	
  int poisson (double relative_lambda); // using a cdf inverse. The duration is externally calculated into relative_lambda.
  int poisson1 (double lambda, double duration); // using calls to erandom() according to a given duration

  int inverse_gamma (double k_shape, int theta_scale); // k_shape is the number of replications and theta_scale is the lambda parameter

  // randomly permute an array

  void permute(int n, int* perm);
};

extern vector<Random*> theRandomizers;

#endif
