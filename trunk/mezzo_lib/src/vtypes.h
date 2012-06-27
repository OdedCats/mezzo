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
	 vector <Vtype*>::iterator iter=vtypes.begin();
 	
	if (vtypes.size() == 1)
		return (*iter);

	double sum=0.0;
	double r = random->urandom();

 	for (iter;iter!=vtypes.end();iter++)
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

class Vclass // contains the vehicle/user classes. This combines demand segments and user preferences.
	// Each vehicle class contains its own list of types.
{
public:
	Vclass (int id_, string label_, Vtypes* vtypes_, double vot, double vod) : id(id_), label(label_), vtypes(vtypes_),
				value_of_time(vot), value_of_distance(vod) {}
	virtual ~Vclass ();
	Vtype* get_vtype() const {return vtypes->random_vtype();}
	const int get_id() const {return id;}
	const string get_label() const {return label;}
	const double get_vot() const {return value_of_time;}
	void set_vot(const double val) {value_of_time=val;}
	const double get_vod() const {return value_of_distance;}
	void set_vod(const double val) {value_of_distance=val;}

private:
	int id;
	string label;

	Vtypes* vtypes; // each vclass has its own vtype
	
	// other attributes for vehicles class.
	double value_of_time; //!< value of time (in chosen currency)
	double value_of_distance; //!< value of traveled distance (in chosen currency)
};

#endif
