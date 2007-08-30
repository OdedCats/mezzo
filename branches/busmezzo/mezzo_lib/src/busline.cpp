// busline.cpp: implementation of the busline class.
//
//////////////////////////////////////////////////////////////////////

#include "busline.h"
#include <math.h>

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
	bus->set_bustrip (this);
	if ( (odpair->get_origin())->insert_veh(bus,time)) // insert the bus at the origin.
  		ok=true;
	else // if insert returned false
  	{
  		ok=false; 
		recycler.addBus(bus);
  	}	
	return ok;
}

 /*bool Bustrip::timepoint_checker (Busstop* stop) // checks if a busstop is a time point for this trip
{
	Timepoint tp;
	tp.first = stop;
	vector<Timepoint*>::iterator iter;
	iter = find (trips_timepoint.begin(), trips_timepoint.end(), tp);
	if ( iter1 == trips_timepoint.end() ) 
	{ 
		return 0; // Meaning that busstop isn't a time point for this line
	}
	else
	{
		if (iter->second == 0)
		{
			return 0; // Meaning that busstop is a timepoint for this busline, but not for this bus trip
		}
		else
		{
			return 1; // Meaning this busstop is a time point for this trip
		}
	}
}
*/


// Busstop functions
Busstop::Busstop (int id_, int link_id_, double length_, bool has_bay_, double dwelltime_):
	id(id_), link_id(link_id_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_)
{
	random = new (Random);
	if (randseed != 0)
		{
		random->seed(randseed);
		}
	else
	{
		random->randomize();
	}
}

Busstop::~Busstop ()
{
	delete random;
}

double Busstop::calc_dwelltime (Bustrip* trip, double time) // calculates the dwelltime of each bus serving this stop
{
	int loadfactor = 0; // bus crowdedness factor
	int nr_boarding = 0;// pass. boarding
	int nr_alighting= 0; // pass alighting
	int curr_occupancy = trip->get_busv()->get_occupancy(); // pass. on the bus when entring the stop, the value will be imported from the bus object
	int boarding_standees = 0;
	int alighting_standees = 0;
	int number_seats = trip->get_busv()->get_number_seats(); // will be imported from the bus object

	double dwell_constant = 12.5; // Value of the constant component in the dwell time function. 
	double boarding_coefficient = 0.55;	// Should be read as an input parameter. Would be different for low floor for example.
	double alighting_coefficient = 0.23;
	double crowdedness_coefficient = 0.0078;
	double out_of_stop_coefficient = 3.0; // Taking in consideration the increasing dwell time when bus stops out of the stop
	bool out_of_stop = check_out_of_stop(trip);
	
	nr_boarding = random -> poisson ((get_arrival_rates (trip) * get_headway (trip, time)) / 60 ); //the boarding process follows a poisson distribution and the lambda is relative to the headway
					// with arrival time and the headway as the duration
	nr_alighting = random -> binrandom (curr_occupancy, get_alighting_rates (trip)); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the alighting fraction
															
	if (curr_occupancy > trip->get_busv()->get_number_seats())	// Calculating alighting standees 
	{ 
		alighting_standees = curr_occupancy - trip->get_busv()->get_number_seats();	
	}
	else	
	{
		alighting_standees = 0;
	}
	curr_occupancy -= nr_alighting; 

	if (nr_boarding > (trip->get_busv()->get_capacity() - curr_occupancy)) // The number of boarding passengers is limited by the capacity
	{
		set_nr_waiting (nr_boarding + curr_occupancy - trip->get_busv()->get_capacity()); // The over-capacity will be added to the waiting passengers at the busstop
		nr_boarding = trip->get_busv()->get_capacity() - curr_occupancy; 
	}

	curr_occupancy += nr_boarding;
	trip->get_busv()->set_occupancy(curr_occupancy); // Updating the occupancy. OUTPUT NOTE
	
	if (curr_occupancy > trip->get_busv()->get_number_seats()) // Calculating the boarding standess
	{
		boarding_standees = curr_occupancy - trip->get_busv()->get_number_seats();
	}
	else 
	{
		boarding_standees = 0;
	}

	loadfactor = nr_boarding * alighting_standees + nr_alighting * boarding_standees;
	set_dwelltime (dwell_constant + boarding_coefficient*nr_boarding + alighting_coefficient*nr_alighting + crowdedness_coefficient*loadfactor + get_bay() * 4 + out_of_stop_coefficient*out_of_stop); // Lin&Wilson (1992) + out of stop effect. 
																			// IMPLEMENT: for articulated buses should be has_bay * 7
		// OUTPUT NOTE
	return dwelltime;
}

void Busstop::occupy_length (Bustrip* trip) // a bus arrived - decrease the left space at the stop
{
	double space_between_buses = 3.0; // the reasonable space between stoping buses, input parameter - IMPLEMENT: shouldn't be for first bus at the stop
	set_avaliable_length (get_avaliable_length() - trip->get_busv()->get_length() - space_between_buses); 
} 

void Busstop::free_length (Bustrip* trip) // a bus left - increase the left space at the stop
{
	double space_between_buses = 3.0; // the reasonable space between stoping buses
	set_avaliable_length  (get_avaliable_length() + trip->get_busv()->get_length() + space_between_buses);
} 

bool Busstop::check_out_of_stop (Bustrip* trip) // checks if there is any space left for the bus at the stop
{
	if (trip->get_busv()->get_length() > get_avaliable_length())
	{
		return true; // no left space for the bus at the stop. IMPLEMENT: generate incidence (capacity reduction)
	}
	else
	{
		return false; // there is left space for the bus at the stop

	}
}

void Busstop::update_last_arrivals (Bustrip* trip, double time) // everytime a bus EXITS a stop it should be updated, 
// in order to keep an updated vector of the last arrivals from each line. ONLY after the dwell time had been calaculated.
// perhaps should be merges with get_headway
// time - the current time, according to the simulation clock
{ 
	Busline_arrival line1;
	line1.first = trip->get_line();
	vector<Busline_arrival>::iterator last_arrival;
	last_arrival = find (last_arrivals.begin(), last_arrivals.end(), line1); // find the line that this trip serves at the vector
	last_arrival->second = time; 
}

double Busstop::get_headway (Bustrip* trip, double time) // calculates the headway (current time minus the last ariival) 
{ // time - the current time, according to the simulation clock
	Busline_arrival line1;
	line1.first = trip->get_line();
	double headway;
	vector<Busline_arrival>::iterator last_arrival;
	last_arrival = find (last_arrivals.begin(), last_arrivals.end(), line1);  // find the line that this trip serves at the vector
	headway = time - last_arrival->second; // OUTPUT NOTE
	return headway;
}

double Busstop::get_arrival_rates (Bustrip* trip)
{
	Busline_arrival line1;
	line1.first = trip->get_line();
	vector<Busline_arrival>::iterator iter1;
	iter1 = find (arrival_rates.begin(), arrival_rates.end(), line1);
	return iter1->second;   
}

double Busstop::get_alighting_rates (Bustrip* trip)
{
	Busline_arrival line1;
	line1.first = trip->get_line();
	vector<Busline_arrival>::iterator iter1;
	iter1 = find (alighting_rates.begin(), alighting_rates.end(), line1);
	return iter1->second;
}

