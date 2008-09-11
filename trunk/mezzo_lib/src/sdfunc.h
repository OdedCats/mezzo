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
  	 Sdfunc(int id_,int vfree_, int vmin_, int romax_);
	 virtual double speed(double ro);     // speed in m/s everywhere...
	 const int get_id();
	 int get_romax();
  protected:
  	 int id;
 	 int vfree;
 	 int vmin;
	 int romax;
};

class GenSdfunc : public Sdfunc
/*
   Generic Speed-density function that can be derived from car-following relation ships.
   V=Vfree*(1-(ro / romax)^alpha)^beta
   alpha and beta can be calculated from the car-following relationship. See May 1990
   current values are 1.5 and 5.0 for alpha and beta


*/

{
    public:
		GenSdfunc (int id_, int vfree_, int vmin_, int romax_, double alpha_=1.5, double beta_=5.0);
		virtual double speed (double ro);
	protected:
		double alpha;
		double beta;
} ;

class DynamitSdfunc : public GenSdfunc
{
	public:
		DynamitSdfunc (int id_, int vfree_, int vmin_, int romax_, int romin_, double alpha_=0.5, double beta_=1.5);
		double speed (double ro);
	private:
		int romin;		
};


#endif

