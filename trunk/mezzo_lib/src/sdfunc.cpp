#include "sdfunc.h"
#include <math.h>
#include "parameters.h"

Sdfunc::Sdfunc(int id_, double vfree_, double vmin_, double romax_, double romin_): id(id_), vfree(vfree_), vmin(vmin_),
		romax(romax_), romin(romin_)
{
	if (min_speed>0) // overrides the min speed if set
		vmin=min_speed;
}
		
double Sdfunc::speed(double ro)
{
	if (ro <= romin)
		return vfree;
	else if (ro>romax)
		return vmin;
	return vmin+(vfree-vmin)*(1-((ro-romin)/(romax-romin)));
}

const int Sdfunc::get_id()
{
	return id;
}

double Sdfunc::get_romax()
{
 	return romax;
}


// DynamitSdfunc functions
/*	DynamitSdfunc, models the speed-density as in DYNAMIT. It is basically a GenSdfunc, with the addition of a kmin, a minimum
	density for the operation of the function. For densities under  romin, vfree is taken.

*/

DynamitSdfunc::DynamitSdfunc (int id_, double vfree_, double vmin_, double romax_, double romin_, double alpha_, double beta_):
	Sdfunc(id_,vfree_,vmin_,romax_,romin_)
{
	alpha=alpha_;
	beta =beta_;
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
