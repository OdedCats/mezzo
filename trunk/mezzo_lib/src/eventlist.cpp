#include "eventlist.h"



// dummy implementation of  virtual execute (...,...) otherwise the linking gives
// problems: no virtual table generated
bool Action::execute(Eventlist*, double)        // unused Eventlist* eventlist, double time
{
	return true;
}

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
	
