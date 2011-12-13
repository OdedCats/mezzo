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
