#include "eventlist.h"



// dummy implementation of  virtual execute (...,...) otherwise the linking gives
// problems: no virtual table generated
bool Action::execute(Eventlist*, double)        // unused Eventlist* eventlist, double time
{
	return true;
}
	
