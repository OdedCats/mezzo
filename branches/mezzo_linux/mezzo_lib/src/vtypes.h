/*****************************************************************************
Vtypes are the Vehicle Types that are available to the simulation. They are 
read into the vtypes class, and have each a given id, label, probability (or 
proportion) and length.

The random_vtype member function of Vtypes returns a random Vehicle type
according to the proportions given.
*******************************************************************************/
#ifndef VTYPES
#define VTYPES
#include <vector>
#include "parameters.h"
#include <string>
using namespace std;

class Vtype
{
public:
	Vtype (int id_, string label_, double prob_, double len_) : id(id_), label(label_), probability(prob_), length(len_) {}
	
	int get_id() {return id;}
	int id;
	string label;
	double probability;
	double length;
};

struct vtype_less
{
 
 bool operator () (Vtype* t1, Vtype* t2)

 	{
 	 return (t1->probability < t2->probability);
 	}
};

class Vtypes
{
public:
 Vtypes () ;

 Vtype* random_vtype () {
 	double sum=0.0;
 	double r = random->urandom();
 	for (vector <Vtype*>::iterator iter=vtypes.begin();iter<vtypes.end();iter++)
    {
     	sum+=(*iter)->probability;
     	if (sum >= r)
     		return (*iter);     		
    }
    return NULL;
 }
 void set_seed(long int seed_) {random->seed(seed_);}
 void initialize ();
 
 Random* random;
 vector <Vtype*> vtypes;
};

#endif
