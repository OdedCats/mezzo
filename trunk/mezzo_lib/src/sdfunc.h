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

/* Defines a linear speed-density function from vfree  to vmin depending on
	the density ro. For ro=0 v=vfree and for ro=romax (jamdensity) v=vmin
*/

#ifndef SDFUNC_HH
#define SDFUNC_HH

class Sdfunc
{
  public:
  	 Sdfunc(int id_,double vfree_, double vmin_, double romax_, double romin_);
	 virtual double speed(double ro);     // speed in m/s everywhere...
	 const int get_id();
	 double get_romax();
  protected:
  	 int id;
 	 double vfree;
 	 double vmin;
	 double romax;
	 double romin;
};


class DynamitSdfunc : public Sdfunc
	/*
   Generic Speed-density function that derived from car-following relation ships.
   V=Vmin + (Vfree-Vmin)*(1-((ro-rmin)/(romax-romin))^alpha)^beta
   alpha and beta can be calculated from the car-following relationship. See May 1990
 */
{
	public:
		DynamitSdfunc (int id_, double vfree_, double vmin_, double romax_, double romin_, double alpha_=0.5, double beta_=1.5);
		double speed (double ro);
	private:
		double alpha;
		double beta;
		
};


#endif

