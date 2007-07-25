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

