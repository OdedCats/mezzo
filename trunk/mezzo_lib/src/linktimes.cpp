/***************************************************************************
                          linktimes.cc  -  description
                             -------------------
    begin                : Fri Aug 9 2002
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

 #include "linktimes.h"
 #include <algorithm>
 #include <iostream>
#include "parameters.h"


 template<class T>
struct compare
{
 compare(int id_):id(id_) {}
 bool operator () (T* thing)

 	{
 	 return (thing->get_id()==id);
 	}

 int id;
};


void LinkTime::generate_disturbances ()
{	
	disturbances.clear();
	for (unsigned int i = 0; i < times_.size(); i++)
		 disturbances [i] = (rand->urandom() -0.5)*2*theParameters->linktime_disturbance*times_[i];;
	 
}


const double LinkTime::cost(const double time)
{
	unsigned int i = static_cast <int> (time/periodlength); // return the current period
	if (times_.size()==0) 
        return 0.1; // never return 0
    else
	{
		if (times_.count(i)) // if exists (should always be true, but still)
		{
			if (disturbances.count(i))
				return times_[i] + disturbances [i];
			else
				return times_[i];
		}
		else
			return 0.1;
	}
    
}

const double LinkTime::mean ()
{

	return (sum() / times_.size());

}

const double LinkTime::sum()
{
	totaltime=0.0;
	map<int,double>::iterator t_iter = times_.begin();
	for (t_iter;t_iter!=times_.end();t_iter++)
	{
		totaltime+=(*t_iter).second;	
	}
	return totaltime;

}

const double LinkTimeInfo::sum()
{
	double totaltime = 0.0;
	map <int,LinkTime*>::iterator iter = times.begin();
	for (iter;iter!=times.end();iter++)
	{
		totaltime+=(*iter).second->mean();
	}
	return totaltime ; // we can do this since all linktimes have same nr of periods


}

void LinkTimeInfo::generate_disturbances ()
{
	map <int,LinkTime*>::iterator iter = times.begin();
	for (iter; iter != times.end(); iter++)
		iter->second->generate_disturbances();
}



const double LinkTimeInfo::mean()
{	

	return sum()/ times.size(); // we can do this since all linktimes have same nr of periods

}

const double LinkTimeInfo::cost (const int i, const double time)  


{
 	map <int,LinkTime*>::iterator iter = times.find (i);
 	if (iter!=times.end())
 		return (*iter).second->cost(time);
 	else
 	{	
 		eout << "LinkTimeInfo:: cost  : Error, can't find the link i = " << i << endl;
 		return 0.1; // NEVER RETURN 0
   }
}


const double LinkTimeInfo::graph_cost (const int i, const double time)  


{
	int graph_i = graphlink_to_link [i];
 	map <int,LinkTime*>::iterator iter = times.find (graph_i);
 	if (iter!=times.end())
 		return (*iter).second->cost(time);
 	else
 	{	
 		eout << "LinkTimeInfo:: graph_cost  : Error, can't find the link i = " << i << endl;
 		return 0.1; // NEVER RETURN 0
   }
}


   

