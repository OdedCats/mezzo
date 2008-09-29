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

const double LinkTime::cost(const double time)
{

      /* ORIGINAL
      if (time > nrperiods*periodlength)  // if it is later than the last period, return the
  			//return 0.0;
           return 0.1; // NEVER RETURN 0
  		else
  		{
  			int i=static_cast <int> (time/periodlength);
  			if (static_cast<int>(times.size()) < i)
  				//return 0.0;
              return 0.1; // NEVER RETURN 0
  			else	
	  			return times[i];
		}
    */
      // NEW
/*
    if (times.size() == 0) // if there are no link times registered
        return 0.1; // never return 0
    else
    {
      int i; // the period to return   the time of
      if (time > nrperiods*periodlength)  // if it is later than the last period, return the
        i=nrperiods;     // last period
      else
        i=static_cast <int> (time/periodlength); // return the current period
      if (static_cast<int>(times.size()) < i) // extra check that i is not out of bounds
        return 0.1; // NEVER RETURN 0
      else
        return times[i];
    }
*/    
	// FASTER..
	unsigned int i = static_cast <int> (time/periodlength); // return the current period
	if (times.size()==0) 
        return 0.1; // never return 0
    else
	{
		if (i < (times.size()-1))
			return times[i];
		else
			return times.back();
	}
    
}

const double LinkTime::mean ()
{
	totaltime=0.0;
	vector<double>::iterator t_iter = times.begin();
	for (t_iter;t_iter<times.end();t_iter++)
	{
		totaltime+=(*t_iter);	
	}
	return (totaltime / nrperiods);

}

const double LinkTimeInfo::mean()
{	
	double totaltime = 0.0;
	map <int,LinkTime*>::iterator iter = times.begin();
	for (iter;iter!=times.end();iter++)
	{
		totaltime+=(*iter).second->mean();
	}
	return (totaltime / times.size()); // we can do this since all linktimes have same nr of periods

}

const double LinkTimeInfo::cost (const int i, const double time)  // to be repaired. It caused crashes in the Graph.cc routines (which contain some archaic C-style array magic)


{
 	//vector <LinkTime*>::iterator iter=find_if (times.begin(),times.end(), compare <LinkTime> (i) );
	map <int,LinkTime*>::iterator iter = times.find (i);
 	if (iter!=times.end())
 		return (*iter).second->cost(time);
 	else
 	{	
 		cout << "LinkTimeInfo:: cost  : Error, can't find the link i = " << i << endl;
 		return 0.1; // NEVER RETURN 0
   }
}


   

