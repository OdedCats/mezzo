#include "sdfunc.h"
#include <math.h>
#include "parameters.h"

Sdfunc::Sdfunc(const int id_, const double vfree_, const double vmin_, const double romax_, const double romin_): id(id_), vfree(vfree_), vmin(vmin_),
		romax(romax_), romin(romin_)
{
	if (min_speed>0) // overrides the min speed if set
		vmin=min_speed;
}
		
const double Sdfunc::speed(const double ro)
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

const double Sdfunc::get_romax()
{
 	return romax;
}


// GenericSdfunc functions
/*	GenericSdfunc, models the speed-density as in DYNAMIT. It is basically a GenSdfunc, with the addition of a kmin, a minimum
	density for the operation of the function. For densities under  romin, vfree is taken.

*/

GenericSdfunc::GenericSdfunc (const int id_, const double vfree_, const double vmin_, const double romax_, const double romin_, const double alpha_, const double beta_):
	Sdfunc(id_,vfree_,vmin_,romax_,romin_)
{
	alpha=alpha_;
	beta =beta_;
}
		
const double GenericSdfunc::speed (const double ro)
{
	if (ro <= romin)
		return vfree;
	else if (ro>romax)
		return vmin;
	else	
       return vmin+(vfree-vmin)*pow((1-pow(((ro-romin)/(romax-romin)),alpha)),beta);
}
