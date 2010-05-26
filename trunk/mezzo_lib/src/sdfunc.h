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

/* The standard Sdfunc defines a linear speed-density function from vfree  to vmin depending on
	the density ro. For ro< romin then v=vfree and for ro>romax , v=vmin
	for ro between romin and romax, V=Vmin + (Vfree-Vmin)*(1-((ro-rmin)/(romax-romin)))
*/

#ifndef SDFUNC_HH
#define SDFUNC_HH

class Sdfunc
{
  public:
  	 Sdfunc(const int id_,const double vfree_, const double vmin_, const double romax_=140, const double romin_ = 0);
	 virtual const double speed(const double ro);     // speed in m/s everywhere...
	 const int get_id();
	 const double get_romax();
  protected:
  	 int id;
 	 double vfree;
 	 double vmin;
	 double romax;
	 double romin;
};


class GenericSdfunc : public Sdfunc
	/*
   Generic Speed-density function that derived from car-following relation ships.
   V=Vmin + (Vfree-Vmin)*(1-((ro-rmin)/(romax-romin))^alpha)^beta
   alpha and beta can be calculated from the car-following relationship. See May 1990
 */
{
	public:
		GenericSdfunc (const int id_, const double vfree_, const double vmin_, const double romax_, const double romin_, const double alpha_=0.5, const double beta_=1.5);
		const double speed (const double ro);
	private:
		double alpha;
		double beta;	
};

class DummySdfunc : public Sdfunc
{
	public:
		DummySdfunc (const int id_, const double vfree_) : Sdfunc(id_,vfree_, vfree_) {}
		const double speed(const double ) {return vfree;}
private:
};


#endif

