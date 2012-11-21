/***************************************************************************
                          vtypes.cc  -  description
                             -------------------
    begin                : Wed Aug 7 2002
    copyright            : (C) 2002 by wilco
    email                : wilco@infra.kth.se
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "vtypes.h"
#include <algorithm>

Vtypes::Vtypes () 
{
 	random= new Random();

	
}


void Vtypes::initialize ()

 {
// do it here since random seed is not read at construction time of VTYPES
#ifndef _DETERMINISTIC_VTYPES
	if (randseed != 0)
	   random->seed(randseed);
	else
		random->randomize();
#else
	 if (randseed != 0)
	   random->seed(randseed);
	else
		random->seed(42);
#endif
/*

	// normalize the probabilities
	double sum = 0.0;
	vector <Vtype*>::iterator iter;
	for (iter=vtypes.begin();iter<vtypes.end();iter++)
    {
     	sum+=(*iter)->probability;    		
    }
	if (sum !=1 )
	{
		for (iter=vtypes.begin();iter<vtypes.end();iter++)
	    {
		 	(*iter)->probability=(*iter)->probability / sum ;    		
		}
	}
	// now sort according to probability
	*/
	sort(vtypes.begin(), vtypes.end());
 }

// Vclass 

Vclass::Vclass (int id_, string label_, double vot, double vod, double voct) : id(id_), label(label_), 
				value_of_time(vot), value_of_distance(vod), value_of_congestiontoll (voct)
{ 
	random = new Random();
	cum_prob=0.0;
}

void Vclass::initialize ()

{
// do it here since random seed is not read at construction time of VTYPES

	 if (randseed != 0)
	   random->seed(randseed);
	else
		random->seed(42);
}

Vclass::~Vclass ()
{
	delete random;
}

void Vclass::add_vtype (Vtype* const val, const double prob) 
{
	cum_prob+=prob;
	vtypes_ [cum_prob]=val;
	if (cum_prob>1.0)
	{
		eout << "WARNING: cumulative probability > 1.0 for vehicle types in vehicle class " << id << endl;
		
	}	
}

Vtype* Vclass::random_vtype() const
{
	double prob = random->urandom();
	map<double,Vtype*>::const_iterator vt;
	vt=vtypes_.lower_bound(prob); // first key value (cumulative prob) >= prob
	if (vt != vtypes_.end())
		return vt->second;
	else
	{
		eout << "ERROR: Vclass::random_vtype() NO type selected, returning NULL " << endl;
		return NULL;
	}

}

Vtype* Vclass::get_vtype( const int val) 
{
	map<double,Vtype*>::iterator vt;
	for (vt=vtypes_.begin(); vt != vtypes_.end(); ++vt)
	{
		if (vt->second->get_id() == val)
			return vt->second;
	}
	eout << "ERROR: Vclass::get_vtype(const int val) NO vtype found with id " << val << " returning NULL " << endl;
	return NULL;
}



