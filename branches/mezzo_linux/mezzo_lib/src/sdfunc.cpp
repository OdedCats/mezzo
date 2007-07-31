#include "sdfunc.h"
#include <math.h>
#include "parameters.h"

Sdfunc::Sdfunc(int id_, int vfree_, int vmin_, int romax_): id(id_), vfree(vfree_), vmin(vmin_),
		romax(romax_)
{
	if (min_speed>0)
		vmin=min_speed;
}
		
double Sdfunc::speed(double ro)
{
	return (vmin+(((romax-ro)/romax)*(vfree-vmin)) );
}

const int Sdfunc::get_id()
{
	return id;
}

int Sdfunc::get_romax()
{
 	return romax;
}

// GenSdfunc functions


GenSdfunc::GenSdfunc (int id_, int vfree_, int vmin_, int romax_, double alpha_, double beta_):Sdfunc(id_,vfree_,vmin_,romax_)
{
	alpha=alpha_;
	beta=beta_;
}
		
		
		
double GenSdfunc::speed (double ro)
{
 return vmin+(vfree-vmin)*pow((1-pow((ro/romax),alpha)),beta);
}

// DynamitSdfunc functions
/*	DynamitSdfunc, models the speed-density as in DYNAMIT. It is basically a GenSdfunc, with the addition of a kmin, a minimum
	density for the operation of the function. For densities under  romin, vfree is taken.

*/

DynamitSdfunc::DynamitSdfunc (int id_, int vfree_, int vmin_, int romax_, int romin_, double alpha_, double beta_):
	GenSdfunc(id_,vfree_,vmin_,romax_,alpha_,beta_)
{
 	romin=romin_;
}
		
double DynamitSdfunc::speed (double ro)
{
	if (ro <= romin)
		return vfree;
	else if (ro>romax)
		return vmin;
	else	
       return vmin+(vfree-vmin)*pow((1-pow(((ro-romin)/(romax-romin)),alpha)),beta);
}
