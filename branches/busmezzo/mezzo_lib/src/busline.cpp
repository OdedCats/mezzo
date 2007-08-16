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

bool Busline::execute(Eventlist* eventlist, double time)
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
	bus->init(vid,vehtype->id, vehtype->length,route,odpair,time);  // initialise the variables of the bus
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

double Busstop::calc_dwelltime ()
{
	int loadfactor; // bus crowdedness factor
	int nr_boarding;// pass. boarding
	int nr_alighting; // pass alighting
	int curr_occupancy = 20; // pass. on the bus when entring the stop, the value will be imported from the bus object
	int boarding_standees = 0;
	int alighting_standees = 0;
	int number_seats = 36; // will be imported from the bus object

	double dwell_constant = 12.5; // Value of the constant component in the dwell time function. 
	double boarding_coefficient = 0.55;	// Should be read as an input parameter. Would be different for low floor for example.
	double alighting_coefficient = 0.23;
	double crowdedness_coefficient = 0.0078;
	double out_of_stop_coefficient = 3.0; // Taking in consideration the increasing dwell time when bus stops out of the stop
	bool out_of_stop = check_out_of_stop();	
//	nr_boarding = Random.poisson(arrival_rate)
//	nr_alighting = Random.poisson(ali_fraction * curr_occupancy);
	
	if (curr_occupancy > number_seats)	// Calculating alighting standees 
	{ 
		alighting_standees = curr_occupancy - number_seats;	
	}
	else	
	{
		alighting_standees=0;
	}
	curr_occupancy -= nr_alighting; // Updating the occupancy
	curr_occupancy += nr_boarding;
	
	if (curr_occupancy > number_seats) // Calculating the boarding standess
	{
		boarding_standees=curr_occupancy-number_seats;
	}
	else 
	{
		boarding_standees=0;
	}

	loadfactor = nr_boarding * alighting_standees + nr_alighting * boarding_standees;
	dwelltime = dwell_constant + boarding_coefficient*nr_boarding + alighting_coefficient*nr_alighting + crowdedness_coefficient*loadfactor + out_of_stop_coefficient*out_of_stop; // Lin&Wilson (1992) + out of stop effect. 
	return dwelltime;
}

void Busstop::occupy_length () // a bus arrived- decrease the left space at the stop
{
	double space_between_buses = 3.0; // the reasonable space between stoping buses, input parameter - IMPLEMENT: shouldn't be for first bus at the stop
	length = 10.0; // will be imported from the bus object
	avaliable_length = avaliable_length - length - space_between_buses; 
} 

void Busstop::free_length () // a bus left- increase the left space at the stop
{
	double buffer = 3.0; // the reasonable space between stoping buses
	length = 10.0; // will be imported from the bus object
	avaliable_length = avaliable_length + length + buffer;
} 

bool Busstop::check_out_of_stop ()
{
	length = 10.0; // will be imported from the bus object
	
	if (length > avaliable_length)
	{
		return true; // no left space for the bus at the stop. IMPLEMENT: generate incidence (capacity reduction)
	}
	else
	{
		return false; // there is left space for the bus at the stop

	}
}

/*void Busstop::update_last_arrivals (Busline* line)
{
	vector<stopping_lines>::iterator last_arrival;
	last_arrival = find (last_arrivals.begin(), last_arrivals.end(), line);
	last_arrival->second = time;
}
*/