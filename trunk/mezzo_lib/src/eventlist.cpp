#include "eventlist.h"


/*
// dummy implementation of  virtual execute (...,...) otherwise the linking gives
// problems: no virtual table generated
const bool Action::execute(Eventlist*, const double)        // unused Eventlist* eventlist, double time
{
	return true;
}
*/
// Eventlist implementation
Eventlist::~Eventlist()

{
	
	multimap <double, Action*> :: iterator iter = thelist.begin();
	for (iter; iter != thelist.end(); iter)
	{
		
			//delete (*iter).second; // All the objects clean up their own actions
			iter = thelist.erase(iter);
	}

}

void Eventlist::reset()
{
	multimap <double, Action*> :: iterator iter = thelist.begin();
	for (iter; iter != thelist.end(); iter)
	{
		// DO NOT delete the Actions, since they are simply reset (in Network), NOT re-created
		iter = thelist.erase(iter);
	}
	lastupdate=thelist.end();
}

const bool Eventlist::move_event (const double time, const double new_time,  Action* action)
{
	if (!(thelist.count(time))) // if there is  nosuch an event
		return false;
	multimap <double, Action*>::iterator iter ;
	pair <multimap <double, Action*>::iterator,multimap <double, Action*>::iterator> ret;
	ret = thelist.equal_range(time); // find all events at this time
	for (iter=ret.first; iter != ret.second; iter++)
	{
		if ((*iter).second == action) // if the correct action is found
		{	
			if (new_time > 0)
				add_event(new_time, action); // add event at new time
			thelist.erase(iter); // remove current event
			return true;	 // break from loop and return
		}
	}
	return false;

}
