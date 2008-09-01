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
		virtual bool execute(Eventlist* eventlist, double time);
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
