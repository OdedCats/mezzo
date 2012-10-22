// busline.cpp: implementation of the busline class.
//
//////////////////////////////////////////////////////////////////////

#include "busline.h"

// Busline functions

Busline::Busline ()
{
	active = false;
}

Busline::Busline (int id_, string name_, Busroute* busroute_, Vtype* vtype_, ODpair* odpair_):
	id(id_), name(name_), busroute(busroute_), vtype(vtype_), odpair(odpair_)

{
	active=false;
}


Busline::~Busline()
{}

const bool Busline::execute(Eventlist* eventlist, const double time)
{
	if (!active) // first time this function is called. no active trips yet
	{
		if (trips.size() == 0)
		{
			return true; // if no trips, return 0
		}
		else
		{
			next_trip= trips.begin();
			double next_time = next_trip->second;
			eventlist->add_event(next_time, this); // add itself to the eventlist, with the time the next trip is starting
			active = true; // now the Busline is active, there is a trip that will be activated at t=next_time
			return true;
		}		
	}
	else // if the Busline is active
	{
		bool ok=next_trip->first->activate(time, busroute, vtype, odpair); // activates the trip, generates bus etc.
		if (!ok)
			cout << "next_trip returns false. " << endl;
		next_trip++; // now points to next trip
		if (next_trip < trips.end()) // if there exists a next trip
		{
			double next_time = next_trip->second;
			eventlist->add_event(next_time, this); // add itself to the eventlist, with the time the next trip is starting
			return true;
		}
	}
	return true;
}

// Bustrip Functions

Bustrip::Bustrip ()
{
	init_occupancy=0;

}

Bustrip::Bustrip (int id_, double start_time_): id(id_), starttime(start_time_)
{
	init_occupancy=0;
}


Bustrip::~Bustrip ()
{}



bool Bustrip::activate (double time, Route* route, Vtype* vehtype, ODpair* odpair)
{
	bool ok = false; // flag to check if all goes ok
	// generate the Bus vehicle
	vid++; // increment the veh id counter, buses are vehicles too
	Bus* bus=recycler.newBus(); // get a bus vehicle
	// !!! init should be modified to reflect the extra vars of the bus !!!
	// TODO: fix veh class for buses
	bus->init(vid,NULL, vehtype,route,odpair,time);  // initialise the variables of the bus
	if ( (odpair->get_origin())->insert_veh(bus,time)) // insert the bus at the origin.
  		ok=true;
	else // if insert returned false
  	{
  		ok=false; 
		recycler.addBus(bus);
  	}	
	return ok;
}


// Busstop functions
Busstop::Busstop (int id_, int link_id_, double length_, bool has_bay_, double dwelltime_):
	id(id_), link_id(link_id_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_)
{

}