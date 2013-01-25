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
		 disturbances [i] = (rand->urandom() -0.5)*theParameters->linktime_disturbance*times_[i];
	 
}

void LinkTime::zero_disturbances()
{
	disturbances.clear();
	for (unsigned int i = 0;i < times_.size(); i++)
		 disturbances [i] = 0.0;
}


const double LinkTime::cost(const double time)
{
	unsigned int i = static_cast <int> (time/periodlength); // return the current period
	if (times_.empty()) 
        return 0.1; // never return 0
    else
	{
		if (times_.count(i)) // if exists (should always be true, but still)
		{
			if (disturbances.count(i))
				return (times_[i] + disturbances [i]);
			else
				return times_[i];
		}
		else
			// fallback
			return (--times_.end())->second;
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


// LinkCostInfo methods

LinkCostInfo::LinkCostInfo():value_of_time(1.0),value_of_distance(0.0)
{


}

LinkCostInfo::~LinkCostInfo ()
{
	// Linktimes* get cleaned up elsewhere
	times_.clear();
}

const double LinkCostInfo::sum()
{
	double totaltime = 0.0;
	map <int,LinkTime*>::iterator iter = times_.begin();
	for (iter;iter!=times_.end();iter++)
	{
		totaltime+=(*iter).second->mean();
	}
	return totaltime ; // we can do this since all linktimes have same nr of periods


}

void LinkCostInfo::generate_disturbances ()
{
	map <int,LinkTime*>::iterator iter = times_.begin();
	for (iter; iter != times_.end(); iter++)
		iter->second->generate_disturbances();
}

void LinkCostInfo::zero_disturbances ()
{
	map <int,LinkTime*>::iterator iter = times_.begin();
	for (iter; iter != times_.end(); iter++)
		iter->second->zero_disturbances();
}



const double LinkCostInfo::mean()
{	

	return sum()/ times_.size(); // we can do this since all linktimes have same nr of periods

}

const double LinkCostInfo::cost (const int i, const double time)  


{ 
	double ltime=1.0, ltoll=0.0, ldistance=0.0;
	
	map <int,LinkTime*>::iterator iter1 = times_.find (i);
 	if (iter1!=times_.end())
 		ltime=(*iter1).second->cost(time);
 	else
 	{	
 		eout << "LinkCostInfo:: cost  : Error, can't find the link i = " << i << endl;
 		ltime= 0.1; // NEVER RETURN 0
   }
	map <int,double>::iterator iter2 = distances_.find (i);
	if (iter2!=distances_.end())
		ldistance=(*iter2).second;
	map <int,double>::iterator iter3 = tolls_.find (i);
	if (iter3!=tolls_.end())
		ltoll=(*iter3).second;
	double cost=value_of_time*(ltime/3600)+value_of_distance*(ldistance/1000)+ltoll; // Cost is in SEK, with VoT in kr/hr and VoD in kr/km
	return cost;
	/* 2012 - try new general structure
 	map <int,LinkTime*>::iterator iter = times_.find (i);
 	if (iter!=times_.end())
 		return (*iter).second->cost(time);
 	else
 	{	
 		eout << "LinkCostInfo:: cost  : Error, can't find the link i = " << i << endl;
 		return 0.1; // NEVER RETURN 0
   }
   */
}


const double LinkCostInfo::graph_cost (const int i, const double time)  


{
	int l_i = graphlink_to_link [i];
	return cost(l_i,time);
	/*
 	map <int,LinkTime*>::iterator iter = times_.find (graph_i);
 	if (iter!=times_.end())
 		return (*iter).second->cost(time);
 	else
 	{	
 		eout << "LinkCostInfo:: graph_cost  : Error, can't find the link i = " << i << endl;
 		return 0.1; // NEVER RETURN 0
   }*/
}


   

