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

/* Defines the eventlist for processing and storing events. Each event is a pair of
	(time, Action*). the eventlist steps through the list and calls the function
	with the time it's called as parameter.
	Action is an abstract class from which the components using the eventlist for
	scheduling their events derive their own specific actions.   In these derived
	classes bool execute() is overwritten to do the real work.

*/


#ifndef EVENTLIST_HH
#define EVENTLIST_HH
#include <list>
#include <map>
#include <assert.h>
#include <algorithm>

using namespace std;
class Eventlist;


class Action
{	
	public:
		virtual const bool execute(Eventlist* eventlist, const double time);
	private:
};


typedef pair <const double , Action*> Valtype;


class Eventlist
{
  public:
	 Eventlist () {lastupdate=thelist.end();}
	 ~Eventlist ();
	 void reset (); // resets the eventlist, throws out all the Actions
  	 inline const bool add_event(const double time_, Action* action)
				{ 
						lastupdate = thelist.insert (lastupdate, Valtype (time_,action));
  						return true;
				}
  	inline const double next ()
  						{
						double temp = thelist.begin()->first;
						(thelist.begin()->second)->execute(this,temp);
						
						thelist.erase (thelist.begin());
   						return temp;}	
  						
 private:
  multimap <double, Action*> :: iterator lastupdate; // to give add_event a guess where to insert the new value
  Valtype value;
  	multimap <double, Action*> thelist;
};


#endif
