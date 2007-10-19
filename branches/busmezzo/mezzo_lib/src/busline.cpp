// busline.cpp: implementation of the busline class.
//
//////////////////////////////////////////////////////////////////////

#include "busline.h"
#include <math.h>
#include "MMath.h"
#include <sstream>

template<class T>
struct compare_pair
{
 compare_pair(int id_):id(id_) {}
 bool operator () (T thing)

 	{
 	 return (thing.first->get_id()==id);
 	}

 int id;
};

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
	for (vector<Timepoint*>::iterator tp = trips_timepoint.begin(); tp < trips_timepoint.end(); tp++)
	{
		(*tp)->second = false;
	}
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
	{
  		busv = bus;
		ok=true;
	}
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

/*
bool Bustrip::check_end_trip ()
{
	if (next_stop == stops.end())
	{
		return true;
	}
	else
	{
		return false;
	}
}
*/

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

bool Bustrip::timepoint_checker (Busstop* stop) // checks if a busstop is a time point for this trip
{
	return true;
	/*
	Timepoint tp;
	tp.first = stop;
	vector<Timepoint*>::iterator iter1;
	iter1 = find (trips_timepoint.begin(), trips_timepoint.end(), tp);
	if ( iter1 == trips_timepoint.end() ) 
	{ 
		return 0; // Meaning that busstop isn't a time point for this bus line
	}
	else
	{
		if ((*iter1)->second == false)
		{
			return false; // Meaning that busstop is a timepoint for this busline, but not for this bus trip
		}
		else
		{
			return true; // Meaning this busstop is a time point for this trip
		}
	}
	*/
}



// Busstop functions
Busstop::Busstop (int id_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_):
	id(id_), link_id(link_id_), position (position_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_)
{
	length = 20;
	position = 0;
	avaliable_length = length;
	nr_boarding = 0;
	nr_alighting = 0;
	has_bay = false;
	dwelltime = 12.5;

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

		buses_at_stop [time + dwelltime] = bus; 	// When time point will work - call calc_exiting_time()
		eventlist->add_event (time + dwelltime, this); // book an event for the time it exits the stop

		write_busstop_visit ("buslog_out.dat", bus->get_bustrip(), time); // document stop-related info
								// done BEFORE update_last_arrivals in order to calc the headway
		update_last_arrivals (bus->get_bustrip(), time); // in order to follow the headways
		bus->get_bustrip()->advance_next_stop();
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

	nr_waiting [trip->get_line()] += random -> poisson (((get_arrival_rates (trip)+10) * get_headway (trip, time)) / 60 );
				//the arrival process follows a poisson distribution and the lambda is relative to the headway
				// with arrival time and the headway as the duration
	if (curr_occupancy == 0) 
	{
		set_nr_alighting (0);
	}
	else
	{
	set_nr_alighting (random -> binrandom (curr_occupancy, get_alighting_rates (trip)+10)); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the alighting fraction
	}

	if (curr_occupancy > trip->get_busv()->get_number_seats())	// Calculating alighting standees 
	{ 
		alighting_standees = curr_occupancy - trip->get_busv()->get_number_seats();	
	}
	else	
	{
		alighting_standees = 0;
	}
	
	curr_occupancy -= get_nr_alighting(); 
	set_nr_boarding (Min(nr_waiting [trip->get_line()] , trip->get_busv()->get_capacity() - curr_occupancy));  // The number of boarding passengers is limited by the capacity
	nr_waiting [trip->get_line()] = Max(nr_waiting [trip->get_line()] - get_nr_boarding(), 0); // the number of waiting passenger is decreased by the number of passengers that went on the bus. 
	
	curr_occupancy += get_nr_boarding();

	trip->get_busv()->set_occupancy(curr_occupancy); // Updating the occupancy. 
	if (curr_occupancy > trip->get_busv()->get_number_seats()) // Calculating the boarding standess
	{
		boarding_standees = curr_occupancy - trip->get_busv()->get_number_seats();
	}
	else 
	{
		boarding_standees = 0;
	}
	loadfactor = get_nr_boarding() * alighting_standees + get_nr_alighting() * boarding_standees;
	dwelltime = (dwell_constant + boarding_coefficient*get_nr_boarding() + alighting_coefficient*get_nr_alighting() 
		+ crowdedness_coefficient*loadfactor + get_bay() * 4 + out_of_stop_coefficient*out_of_stop); // Lin&Wilson (1992) + out of stop effect. 
																			// IMPLEMENT: for articulated buses should be has_bay * 7
		// OUTPUT NOTE
	return dwelltime;
}

void Busstop::occupy_length (Bus* bus) // a bus arrived - decrease the left space at the stop
{
	double space_between_buses = 0.5; // the reasonable space between stoping buses, input parameter - IMPLEMENT: shouldn't be for first bus at the stop
	set_avaliable_length (get_avaliable_length() - bus->get_length() - space_between_buses); 
} 

void Busstop::free_length (Bus* bus) // a bus left - increase the left space at the stop
{
	double space_between_buses = 0.5; // the reasonable space between stoping buses
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
	last_arrivals [trip->get_line()] = time;
}

double Busstop::get_headway (Bustrip* trip, double time) // calculates the headway (current time minus the last ariival) 
{ // time - the current time, according to the simulation clock
	return time - last_arrivals[trip->get_line()];
}

double Busstop::calc_exiting_time (Bustrip* trip, double time)
{
	if (trip->timepoint_checker(this) == true)
	{
		return Max(trip->scheduled_arrival_time(this), time + dwelltime) ; // since it is a time-point stop, it will wait if neccesary till the scheduled time
	}
	else 
	{
		return time + dwelltime; // since it isn't a time-point stop, it will simply exit after dwell time
	}
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
		out << trip->scheduled_arrival_time (this) << time - trip->scheduled_arrival_time (this) << dwelltime << 
		time + dwelltime << get_headway (trip , time) << get_nr_alighting() << get_nr_boarding() << trip->get_busv()->get_occupancy() << endl; 
	}
	out.close();
}



