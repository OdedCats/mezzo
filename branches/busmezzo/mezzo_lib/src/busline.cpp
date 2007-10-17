// busline.cpp: implementation of the busline class.
//
//////////////////////////////////////////////////////////////////////

#include "busline.h"
#include <math.h>
#include "MMath.h"
#include <sstream>

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
		bool ok=next_trip->first->activate(time, busroute, vtype, odpair, eventlist); // activates the trip, generates bus etc.
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

bool Bustrip::advance_next_stop ()
{
	if (next_stop < stops.end())
	{
		next_stop++;
	}
	if (next_stop != stops.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool Bustrip::activate (double time, Route* route, Vtype* vehtype, ODpair* odpair, Eventlist* eventlist_)
{
	eventlist = eventlist_;
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

void Bustrip::book_stop_visit (double time, Bus* bus)
{ 
	((*next_stop)->first)->book_bus_arrival(eventlist,time,bus);
}


double Bustrip::scheduled_arrival_time (Busstop* stop) // finds the scheduled arrival time for a given bus stop
{
	for (vector<Visit_stop*>::iterator scheduled_time = stops.begin();scheduled_time < stops.end(); scheduled_time++)
	{
		if ((*scheduled_time)->first == stop)
		{	
			return (*scheduled_time)->second;
		}
	} 
	return 0; // if bus stop isn't on the trip's route
}

/*
 bool Bustrip::timepoint_checker (Busstop* stop) // checks if a busstop is a time point for this trip
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
Busstop::Busstop (int id_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_):
	id(id_), link_id(link_id_), position (position_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_)
{
	nr_waiting = 0;
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

void Busstop::book_bus_arrival(Eventlist* eventlist, double time, Bus* bus)
{
	expected_arrivals [time] = bus; // not sure if we really want to index them by time? maybe simply by bus id?
	eventlist->add_event(time,this);
	
} // add to expected arrivals

bool Busstop::execute(Eventlist* eventlist, double time) // is executed by the eventlist and means a bus needs to be processed
{
	
	if (buses_at_stop.count(time) > 0) // Search if this is for a bus exiting the stop
	{
		//BUS exiting stop
		Bus* bus = buses_at_stop [time];
		free_length (bus);
		buses_at_stop.erase(time);
	}
	else if (expected_arrivals.count(time) > 0) // Search if this is for a bus entering the stop
	{		
		// BUS entering stop
		Bus* bus = expected_arrivals [time];
		occupy_length (bus);
		dwelltime = calc_dwelltime(bus->get_bustrip(), time);
		buses_at_stop [time + dwelltime()] = bus; 	// When time point will work - call calc_exiting_time()
		eventlist->add_event (time + dwelltime(), this); // book an event for the time it exits the stop
		write_busstop_visit ("buslog_out.dat", bus->get_bustrip(), time); // document stop-related info
								// done BEFORE update_last_arrivals in order to calc the headway
		update_last_arrivals (bus->get_bustrip(), time); // in order to follow the headways
		expected_arrivals.erase(time);
		
		/*
		bool rest_of_trip;
		rest_of_trip = bus->get_bustrip()->advance_next_stop(); // advance the pointer to the next bus stop
		if (rest_of_trip == false) // If the bus reached it last stop - move the pointer to the next trip
		{
			bus->advance_curr_trip();
			eventlist->add_event (time + bus->calc_departure_time(time), this);
		}
		*/
	}
	return true;
}
double Busstop::calc_dwelltime (Bustrip* trip, double time) // calculates the dwelltime of each bus serving this stop
{
	int loadfactor; // bus crowdedness factor
	int curr_occupancy = trip->get_busv()->get_occupancy(); // pass. on the bus when entring the stop, the value will be imported from the bus object
	int boarding_standees;
	int alighting_standees;
	int number_seats = trip->get_busv()->get_number_seats(); // will be imported from the bus object

	double dwell_constant = 12.5; // Value of the constant component in the dwell time function. 
	double boarding_coefficient = 0.55;	// Should be read as an input parameter. Would be different for low floor for example.
	double alighting_coefficient = 0.23;
	double crowdedness_coefficient = 0.0078;
	double out_of_stop_coefficient = 3.0; // Taking in consideration the increasing dwell time when bus stops out of the stop
	bool out_of_stop = check_out_of_stop(trip->get_busv());
	
	set_nr_boarding (random -> poisson ((get_arrival_rates (trip) * get_headway (trip, time)) / 60 )); //the boarding process follows a poisson distribution and the lambda is relative to the headway
					// with arrival time and the headway as the duration
	set_nr_waiting (get_nr_waiting() + get_nr_boarding()); // the new comers are added to the waiting passengers
	set_nr_alighting (random -> binrandom (curr_occupancy, get_alighting_rates (trip))); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the alighting fraction
															
	if (curr_occupancy > trip->get_busv()->get_number_seats())	// Calculating alighting standees 
	{ 
		alighting_standees = curr_occupancy - trip->get_busv()->get_number_seats();	
	}
	else	
	{
		alighting_standees = 0;
	}
	curr_occupancy -= get_nr_alighting(); 


	if (nr_boarding > (trip->get_busv()->get_capacity() - curr_occupancy)) // The number of boarding passengers is limited by the capacity
	{
		set_nr_boarding (trip->get_busv()->get_capacity() - curr_occupancy); 
	}
	
	set_nr_waiting (Max(get_nr_waiting() - get_nr_boarding(), 0)); // the number of waiting passenger is decreased by the number of pass. that went on the bus. 
	// OUTPUT NOTE
	curr_occupancy += get_nr_boarding();

	trip->get_busv()->set_occupancy(curr_occupancy); // Updating the occupancy. OUTPUT NOTE
	
	if (curr_occupancy > trip->get_busv()->get_number_seats()) // Calculating the boarding standess
	{
		boarding_standees = curr_occupancy - trip->get_busv()->get_number_seats();
	}
	else 
	{
		boarding_standees = 0;
	}
	trip->get_busv()->set_occupancy(curr_occupancy);
	loadfactor = get_nr_boarding() * alighting_standees + get_nr_alighting() * boarding_standees;
	set_dwelltime (dwell_constant + boarding_coefficient*get_nr_boarding() + alighting_coefficient*get_nr_alighting() + crowdedness_coefficient*loadfactor + get_bay() * 4 + out_of_stop_coefficient*out_of_stop); // Lin&Wilson (1992) + out of stop effect. 
																			// IMPLEMENT: for articulated buses should be has_bay * 7
		// OUTPUT NOTE
	return dwelltime;
}

void Busstop::occupy_length (Bus* bus) // a bus arrived - decrease the left space at the stop
{
	double space_between_buses = 3.0; // the reasonable space between stoping buses, input parameter - IMPLEMENT: shouldn't be for first bus at the stop
	set_avaliable_length (get_avaliable_length() - bus->get_length() - space_between_buses); 
} 

void Busstop::free_length (Bus* bus) // a bus left - increase the left space at the stop
{
	double space_between_buses = 3.0; // the reasonable space between stoping buses
	set_avaliable_length  (get_avaliable_length() + bus->get_length() + space_between_buses);
} 

bool Busstop::check_out_of_stop (Bus* bus) // checks if there is any space left for the bus at the stop
{
	if (bus->get_length() > get_avaliable_length())
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

void Busstop::write_busstop_visit (string name, Bustrip* trip, double time)  // creates a log-file for stop-related info
{
	ofstream out(name.c_str(),ios_base::app);
	assert(out);
	out << trip->get_line()->get_id() <<  trip->get_id() << trip->get_busv()->get_id() << get_id() << time;
	if (trip->scheduled_arrival_time (this) == 0)
	{
		out << "Error : Busstop ID: " << get_id() << " is not on Bustrip ID: " << trip->get_id() << " route." << endl;
	}
	else
	{
		out << trip->scheduled_arrival_time (this) << time - trip->scheduled_arrival_time (this) << get_dwelltime() << 
		time + get_dwelltime() << get_headway (trip , time) << get_nr_alighting() << get_nr_boarding() << trip->get_busv()->get_occupancy() << endl; 
	}
	out.close();
}



