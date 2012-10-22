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
