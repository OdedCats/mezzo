///! odstops.cpp: implementation of the odstops class.
#include "od_stops.h"
#include <math.h>
#include "MMath.h"

ODstops::ODstops ()
{
}

ODstops::ODstops (Busstop* origin_stop_, Busstop* destination_stop_, double arrival_rate_)
{
	origin_stop = origin_stop_;
	destination_stop = destination_stop_;
	arrival_rate = arrival_rate_;
	random = new (Random);
	active = false;
	if (randseed != 0)
		{
		random->seed(randseed);
		}
	else
	{
		random->randomize();
	}
}

ODstops::~ODstops()
{
	delete random;
}

bool ODstops::execute (Eventlist* eventlist, double curr_time) // generate passengers with this OD and books an event for next passenger generation
{
	if (active = true) // generate passenger from the second call, as first initialization call just set time to first passenger
	{	
		Passenger* pass = pass_recycler.newPassenger();
		pid++; 
		pass->init (pid, curr_time, this);
		waiting_passengers.push_back (pass); // storaged the new passenger at the list of waiting passengers with this OD
	}
	double headway_to_next_pass = random -> erandom (arrival_rate / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
	eventlist->add_event (curr_time + headway_to_next_pass, this);
	active = true;
	return true;

}