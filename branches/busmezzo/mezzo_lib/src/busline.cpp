///! busline.cpp: implementation of the busline class.

#include "busline.h"
#include <math.h>
#include "MMath.h"
#include <sstream>


PassengerRecycler pass_recycler; // Global passenger recycler

template<class T>
struct compare_pair
{
 compare_pair(int id_):id(id_) {}
 bool operator () (T thing)

 	{
 	 return (thing->first->get_id()==id);
 	}

 int id;
};

// ***** Busline functions *****

Busline::Busline ()
{
	active = false;
}

Busline::Busline (int id_, string name_, Busroute* busroute_, vector<Busstop*> stops_, Vtype* vtype_, ODpair* odpair_, int average_headway_, float ratio_headway_holding_, int holding_strategy_):
	id(id_), name(name_), busroute(busroute_), stops(stops_), vtype(vtype_), odpair(odpair_), average_headway(average_headway_), ratio_headway_holding(ratio_headway_holding_), holding_strategy(holding_strategy_) 

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
			curr_trip = trips.begin();
			double next_time = curr_trip->second;
			eventlist->add_event(next_time, this); // add itself to the eventlist, with the time the next trip is starting
			active = true; // now the Busline is active, there is a trip that will be activated at t=next_time
			return true;
		}		
	}
	else // if the Busline is active
	{
		curr_trip->first->activate(time, busroute, odpair, eventlist); // activates the trip, generates bus etc.
		curr_trip++; // now points to next trip
		if (curr_trip < trips.end()) // if there exists a next trip
		{
			double next_time = curr_trip->second;
			eventlist->add_event(next_time, this); // add itself to the eventlist, with the time the next trip is starting
			return true;
		}
	}
	return true;
}

bool Busline::is_line_timepoint (Busstop* stop)
{
	for (vector <Busstop*>::iterator tp = line_timepoint.begin(); tp < line_timepoint.end(); tp++ )
	{
		if (stop == *(tp))
		{
			return true;
		}
	}
return false;
}

bool Busline::check_first_stop (Busstop* stop)
{
	if (stop==*(stops.begin()))
	{
		return true;
	}
return false;
}

bool Busline::check_first_trip (Bustrip* trip)
{
	if (trip == trips.begin()->first)
	{
		return true;
	}
return false;
}

double Busline::calc_curr_line_headway ()
{
	if (curr_trip == trips.end()) 
	{
		return ((*(curr_trip-1)).second - (*(curr_trip-2)).second);
	}
	if (curr_trip == trips.begin())
	{
		return ((*(curr_trip+1)).second - (*curr_trip).second);
	}
	else
	{
		return ((*curr_trip).second - (*(curr_trip-1)).second);
	}
}

double Busline::calc_curr_line_ivt (Busstop* start_stop, Busstop* end_stop)
{
	vector<Visit_stop*>::iterator board_stop;
	vector<Visit_stop*>::iterator alight_stop;
	vector <Start_trip>::iterator check_trip;
	if	(curr_trip == trips.end())
	{
		check_trip = curr_trip-1;
	}
	else
	{
		check_trip = curr_trip;
	}
 	for (vector<Visit_stop*>::iterator stop = (*check_trip).first->stops.begin(); stop <(*check_trip).first->stops.end(); stop++)
	{
			if ((*stop)->first->get_id() == start_stop->get_id())
			{
				board_stop = stop;
			}
			if ((*stop)->first->get_id() == end_stop->get_id())
			{
				alight_stop = stop;
				break;
			}
	}
	return ((*alight_stop)->second - (*board_stop)->second);
}

// ***** Bustrip Functions *****

Bustrip::Bustrip ()
{
	init_occupancy = 0;
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

Bustrip::Bustrip (int id_, double start_time_): id(id_), starttime(start_time_)
{
	init_occupancy=0;
	random = new (Random);
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
	/*  will be relevant only when time points will be trip-specific
	for (map<Busstop*,bool>::iterator tp = trips_timepoint.begin(); tp != trips_timepoint.end(); tp++)
	{
		tp->second = false;
	} 
	*/
}

Bustrip::~Bustrip ()
{}

double Bustrip::calc_departure_time (double time) // calculates departure time from origin according to arrival time and schedule (including layover effect)
{
	double min_recovery = 60.00; 
	double mean_error_recovery = 60.00;
	double std_error_recovery = 60.00;
	// These three parameters should be used from the parameters input file
	vector <Start_trip*>::iterator curr_trip; // find the pointer to the current trip
	for (vector <Start_trip*>::iterator trip = driving_roster.begin(); trip < driving_roster.end(); trip++)
	{
		if ((*trip)->first == this)
		{
			curr_trip = trip;
			break;
		}
	}
	if (curr_trip == driving_roster.begin()) // if it is the first trip for this bus
	{
		return starttime;  // first dispatching is cuurently assumed to follow the schedule
	}
	else
	// if it is not the first trip for this bus then:
	// if the scheduled time is after arrival+recovery, it determines departure time. 
	// otherwise (bus arrived behind schedule) - delay at origin.
	// in any case - there is error factor.
	{
		return (Max (time + min_recovery , starttime) + random->lnrandom (mean_error_recovery, std_error_recovery)); // error factor following log normal distribution;
	}
}

bool Bustrip::advance_next_stop (double time, Eventlist* eventlist)
{
	if (busv->get_on_trip()== true && next_stop < stops.end()) // progress to the next stop, unless it is the last stop for this trip
	{
		next_stop++;
		return true;
	}
	if (busv->get_on_trip()== false && next_stop == stops.end()) // if it was the last stop for this trip
	{	
		vector <Start_trip*>::iterator curr_trip, next_trip; // find the pointer to the current and next trip
		for (vector <Start_trip*>::iterator trip = driving_roster.begin(); trip < driving_roster.end(); trip++)
		{
			if ((*trip)->first == this)
			{
				curr_trip = trip;
				break;
			}
				
		}
		next_trip = curr_trip +1;
		vector <Start_trip*>::iterator last_trip = driving_roster.end()-1;
		if (busv->get_curr_trip() != (*last_trip)->first) // if there are more trips for this vehicle
		{
			vid++;
			Bus* new_bus=recycler.newBus(); // then generate a new (chained) vehicle 
			new_bus->set_bustype_attributes ((*next_trip)->first->get_bustype());
			(*next_trip)->first->set_busv (new_bus);
			new_bus->set_curr_trip((*next_trip)->first);
		}
		busv->advance_curr_trip(time, eventlist); // progress the roster for the vehicle
		//int pass_id = busv->get_id();
		//recycler.addBus(busv);
		
		return false;
	}
	else
	{
		return true;
	}
}

bool Bustrip::activate (double time, Route* route, ODpair* odpair, Eventlist* eventlist_)
{
	// inserts the bus at the origin of the route
	// if the assigned bus isn't avaliable at the scheduled time, then the trip is activated by Bus::advance_curr_trip as soon as it is done with the previous trip
	eventlist = eventlist_;
	bool ok = false; // flag to check if all goes ok
	vector <Start_trip*>::iterator curr_trip, previous_trip; // find the pointer to the current and previous trip
	for (vector <Start_trip*>::iterator trip = driving_roster.begin(); trip < driving_roster.end(); trip++)
	{
		if ((*trip)->first == this)
		{
			curr_trip = trip;
			break;
		}
	}
	if (curr_trip == driving_roster.begin()) // if it is the first trip for this chain
	{
		vid++;
		Bus* new_bus=recycler.newBus(); // then generate a new vehicle
		new_bus->set_bustype_attributes (btype);
		busv =new_bus;
		busv->set_curr_trip(this);	
	}
	else // if it isn't the first trip for this chain 
	{
		previous_trip = curr_trip-1;
		if ((*previous_trip)->first->busv->get_on_trip() == true) // if the assigned bus isn't avaliable 
		{
			ok=false;
			return ok;
		}
	}	
	busv->init(busv->get_id(),4,busv->get_length(),route,odpair,time); // initialize with the trip specific details
	busv->set_on_trip (true);
	if ( (odpair->get_origin())->insert_veh(busv, calc_departure_time(time))) // insert the bus at the origin at the possible departure time
	{
  		busv->set_on_trip(true); // turn on indicator for bus on a trip
		ok = true;
	}
	else // if insert returned false
  	{
  		ok = false; 
  	}	
	return ok;
}

void Bustrip::book_stop_visit (double time, Bus* bus)
{ 
	// books an event for the arrival of a bus at a bus stop
	((*next_stop)->first)->book_bus_arrival(eventlist,time,bus);
}


bool Bustrip::check_end_trip ()
{
	// indicates if the trip doesn't have anymore stops on its route
	if (next_stop == stops.end())
	{
		return true;
	}
	else
	{
		return false;
	}
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

/* will be relevant only when time points will be trip-specific
bool Bustrip::is_trip_timepoint (Busstop* stop)
{
	 if (trips_timepoint.count(stop) > 0)
		return (int)trips_timepoint[stop];
	 else 
		return -1;
}
*/


// ***** Busstop functions *****

Busstop::Busstop()
{
	length = 20;
	position = 0;
	has_bay = false;
	dwelltime = 12.5;
}

Busstop::Busstop (int id_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_):
	id(id_), link_id(link_id_), position (position_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_)
{
	avaliable_length = length;
	nr_boarding = 0;
	nr_alighting = 0;
	is_origin = false;
	is_destination = false;
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
	// books an event for the arrival of a bus at a bus stop by adding it to the expected arrivals at the stop 
	expected_arrivals [time] = bus; // not sure if we really want to index them by time? maybe simply by bus id?
	eventlist->add_event(time,this);
} 

bool Busstop::execute(Eventlist* eventlist, double time) // is executed by the eventlist and means a bus needs to be processed
{
  	// progress the vehicle when entering or exiting a stop
	//	
	if (theParameters->demand_format == 3)
	{
		for (map <Busstop*, ODstops*>::iterator destination_stop = stop_as_origin.begin(); destination_stop != stop_as_origin.end(); destination_stop++)
		{
			passengers pass_waiting_od = (*destination_stop).second->get_waiting_passengers();		
			passengers::iterator check_pass = pass_waiting_od.begin();
			Passenger* next_pass;
			bool last_waiting_pass = false;
			while (check_pass < pass_waiting_od.end())
			{
					// progress each waiting passenger   
				if ((*check_pass)->get_OD_stop()->get_origin()->get_id() != this->get_id())
				{
					break;
				}
			check_pass++;
			}
		}
	}
	//
	if (buses_at_stop.count(time) > 0) 
	// if this is for a bus EXITING the stop:
	{
		Bus* bus = buses_at_stop [time]; // identify the relevant bus
		free_length (bus);
		buses_at_stop.erase(time);
		double relative_length;
		// calculate the updated exit time from the link	
		double ro =0.0;
		#ifdef _RUNNING     // if running segment is seperate density is calculated on that part only
			ro=bus->get_curr_link()->density_running(time);
		#else
			#ifdef _RUNNING_ONLY
				ro = bus->get_curr_link()->density_running_only(time);
			#else	
			ro=bus->get_curr_link()->density();
			#endif	//_RUNNING_ONLY
		#endif  //_RUNNING
	
		double speed = bus->get_curr_link()->get_sdfunc()->speed(ro);	
		double link_total_travel_time = bus->get_curr_link()->get_length()/speed ;

		#ifdef _USE_EXPECTED_DELAY
			double exp_delay = 1.44 * (queue->queue(time)) / bus->get_curr_link()->get_nr_lanes();
			exit_time = exit_time + exp_delay;
		#endif //_USE_EXPECTED_DELAY
		if (bus->get_curr_trip()->check_end_trip() == false) // if there are more stops on the bus's route
		{
			Visit_stop* next_stop1 = *(bus->get_curr_trip()->get_next_stop());
			if (bus->get_curr_link()->get_id() == (next_stop1->first->get_link_id())) // the next stop IS on this link
			{
				double stop_position = (next_stop1->first)->get_position();
				relative_length = (stop_position - position)/(bus->get_curr_link()->get_length()); // calculated for the interval between the two sequential stops
				double time_to_stop = time + link_total_travel_time * relative_length;
				bus->get_curr_trip()->book_stop_visit (time_to_stop, bus); // book  stop visit
			}
			else // the next stop is NOT on this link
			{
				Vehicle* veh =  (Vehicle*)(bus); // so we can do vehicle operations
				relative_length = (bus->get_curr_link()->get_length()-position)/ bus->get_curr_link()->get_length(); // calculated for the remaining part of the link
				double exit_time = time + link_total_travel_time * relative_length;
				veh->set_exit_time(exit_time);
				veh->get_curr_link()->get_queue()->enter_veh(veh);
			}
		}
		else // there are no more stops on this route
		{
			Vehicle* veh =  (Vehicle*)(bus); // so we can do vehicle operations
			relative_length = (bus->get_curr_link()->get_length()-position)/ bus->get_curr_link()->get_length(); // calculated for the remaining part of the link 
			double exit_time = time + link_total_travel_time * relative_length;
			veh->set_exit_time(exit_time);
			veh->get_curr_link()->get_queue()->enter_veh(veh);
			bus->set_on_trip(false); // indicate that there are no more stops on this route
			bus->get_curr_trip()->advance_next_stop(exit_time, eventlist); 
		}
	}
	// if this is for a bus ENTERING the stop:
	else if (expected_arrivals.count(time) > 0) 
	{		
		Bus* bus = expected_arrivals [time]; // identify the relevant bus
		bus->get_curr_trip()->set_enter_time (time);
		exit_time = calc_exiting_time(bus->get_curr_trip(), time); // get the expected exit time according to dwell time calculations and time point considerations
		occupy_length (bus);
		buses_at_stop [exit_time] = bus; 
		eventlist->add_event (exit_time, this); // book an event for the time it exits the stop
		record_busstop_visit ( bus->get_curr_trip(), bus->get_curr_trip()->get_enter_time()); // document stop-related info
								// done BEFORE update_last_arrivals in order to calc the headway
		bus->get_curr_trip()->advance_next_stop(exit_time, eventlist); 
		update_last_arrivals (bus->get_curr_trip(), bus->get_curr_trip()->get_enter_time()); // in order to follow the arrival times (AFTER dwell time is calculated)
		update_last_departures (bus->get_curr_trip(), exit_time); // in order to follow the departure times (AFTER the dwell time and time point stuff)
		expected_arrivals.erase(time);
	}
	return true;
}
double Busstop::passenger_activity_at_stop (Bustrip* trip, double time) //!< progress passengers at stop: waiting, boarding and alighting
{
	set_nr_boarding (0);
	set_nr_alighting (0);
	stops_rate stops_rate_dwell, stops_rate_coming, stops_rate_waiting;
	int starting_occupancy; // bus crowdedness factor
	
	// find out bus occupancy when entering the stop
	if (trip->get_next_stop() == trip->stops.begin()) // pass. on the bus when entring the stop
	{
		starting_occupancy = trip->get_init_occupancy();
	}
	else
	{
		starting_occupancy = trip->get_busv()->get_occupancy(); 
	}

	if (theParameters->demand_format == 1) // demand is given in terms of arrival rates and alighting fractions
	{
		// generate waiting pass. and alight passengers 
		nr_waiting [trip->get_line()] += random -> poisson (((get_arrival_rates (trip)) * get_time_since_arrival (trip, time)) / 3600.0 );
				//the arrival process follows a poisson distribution and the lambda is relative to the headway
				// with arrival time and the headway as the duration
		if (starting_occupancy > 0 && get_alighting_fractions (trip) > 0) 
		{
			set_nr_alighting (random -> binrandom (starting_occupancy, get_alighting_fractions (trip))); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the alighting fraction
		}
	}
	if (theParameters->demand_format == 2) // demand is given in terms of arrival rates per a pair of stops (no individual pass.)
	{
		// generate waiting pass. per pre-determined destination stop and alight passngers headed for this stop
		stops_rate_dwell = multi_arrival_rates[trip->get_line()];
		stops_rate_waiting = multi_nr_waiting[trip->get_line()];
		for (vector <Busstop*>::iterator destination_stop = trip->get_line()->stops.begin(); destination_stop < trip->get_line()->stops.end(); destination_stop++)
		{
			if (stops_rate_dwell[(*destination_stop)] != 0 )
			{
				stops_rate_coming[(*destination_stop)] = (random -> poisson ((stops_rate_dwell[(*destination_stop)] * get_time_since_arrival (trip, time)) / 3600.0 )); // randomized the number of new-comers to board that the destination stop
				trip->nr_expected_alighting[(*destination_stop)] += int (stops_rate_coming[(*destination_stop)]);
				stops_rate_waiting[(*destination_stop)] += int(stops_rate_coming[(*destination_stop)]); // the total number of passengers waiting for the destination stop is updated by adding the new-comers
				nr_waiting [trip->get_line()] += int(stops_rate_coming[(*destination_stop)]);
			}
		}
		multi_nr_waiting[trip->get_line()] = stops_rate_waiting;
		set_nr_alighting (trip->nr_expected_alighting[this]); // passengers heading for this stop alight
		trip->nr_expected_alighting[this] = 0; 
	}
		if (theParameters->demand_format < 3) // in the case of non-individual passengers - boarding progress for waiting passengers (capacity constraints)
	{	
		if (trip->get_busv()->get_capacity() - (starting_occupancy - get_nr_alighting()) < nr_waiting [trip->get_line()]) // if the capcity determines the boarding process
		{	
			if (theParameters->demand_format == 1)
			{
				set_nr_boarding(trip->get_busv()->get_capacity() - (starting_occupancy - get_nr_alighting()));
				nr_waiting [trip->get_line()] -= nr_boarding;
			}
			if (theParameters->demand_format == 2)
			{
				double ratio = double(nr_waiting [trip->get_line()])/(trip->get_busv()->get_capacity() - (starting_occupancy + get_nr_boarding() - get_nr_alighting()));
				for (vector <Busstop*>::iterator destination_stop = trip->get_line()->stops.begin(); destination_stop < trip->get_line()->stops.end(); destination_stop++)
				 // allow only the ratio between supply and demand for boarding equally for all destination stops
				{
					if (stops_rate_dwell[(*destination_stop)] != 0 )
					{	
						int added_expected_passengers = Round(stops_rate_waiting[(*destination_stop)]/ratio);
						set_nr_boarding(get_nr_boarding() + added_expected_passengers);
						trip->nr_expected_alighting[(*destination_stop)] += added_expected_passengers;
						trip->nr_expected_alighting[(*destination_stop)] -= int(stops_rate_coming[(*destination_stop)]);
						stops_rate_waiting[(*destination_stop)] -= added_expected_passengers;
						nr_waiting[trip->get_line()] -= added_expected_passengers;
					}
				}
				multi_nr_waiting[trip->get_line()] = stops_rate_waiting;
			}
		}
		else // all waiting passengers for this busline can board it
		{	
			set_nr_boarding(nr_waiting [trip->get_line()]);
			nr_waiting [trip->get_line()] = 0;
			if (theParameters->demand_format == 2)
			{
				// keep track of boarding and waiting passengers by destination
				for (vector <Busstop*>::iterator destination_stop = trip->get_line()->stops.begin(); destination_stop < trip->get_line()->stops.end(); destination_stop++)
				{
					trip->nr_expected_alighting[(*destination_stop)] += int(stops_rate_waiting[(*destination_stop)]);
					trip->nr_expected_alighting[(*destination_stop)] -= int(stops_rate_coming[(*destination_stop)]);
					stops_rate_waiting[(*destination_stop)] = 0;
				}
				multi_nr_waiting[trip->get_line()] = stops_rate_waiting;
			}
		}
	}
	if (theParameters->demand_format == 3)   // demand is given in terms of arrival rate of individual passengers per OD of stops (future - route choice)
	{	
		// * Alighting passengers *
		if (is_destination == true)
		{
			set_nr_alighting (trip->passengers_on_board[this].size()); 
			for (vector <Passenger*> ::iterator alighting_passenger = trip->passengers_on_board[this].begin(); alighting_passenger < trip->passengers_on_board[this].end(); alighting_passenger++)
			{
				if (this == (*alighting_passenger)->get_OD_stop()->get_destination()) // if this is the final destination of the passenger
				{
					pass_recycler.addPassenger(*alighting_passenger); // terminate passenger
					// in the future - create an output file for passenger activity
				}
				else // if this is an intermediate transfer stop on passenger route 
				{
					(*alighting_passenger)->set_ODstop(stop_as_origin[(*alighting_passenger)->get_OD_stop()->get_destination()]); // set this stop to his new origin (new OD)
					(*alighting_passenger)->get_OD_stop()->get_waiting_passengers().push_back (*alighting_passenger); // add him to the waiting queue on his new OD
				}
			}
			trip->passengers_on_board[this].clear(); 
		}
		// * Boarding passengers *
		if (is_origin == true)
		{
			for (map <Busstop*, ODstops*>::iterator destination_stop = stop_as_origin.begin(); destination_stop != stop_as_origin.end(); destination_stop++)
			{
				// going through all the stops that this stop is their origin on a given OD pair
				passengers pass_waiting_od = (*destination_stop).second->get_waiting_passengers();
				if (pass_waiting_od.empty() == false) // if there are waiting passengers with this destination
				{
					passengers::iterator check_pass = pass_waiting_od.begin();
					Passenger* next_pass;
					bool last_waiting_pass = false;
					while (check_pass < pass_waiting_od.end())
					{
						// progress each waiting passenger   
						if ((*check_pass)->get_OD_stop()->get_origin()->get_id() != this->get_id())
						{
							break;
						}
						if ((*check_pass)->boarding_decision(trip->get_line()) == true)
						{
							// if the passenger decided to board this bus
							if ((starting_occupancy + get_nr_boarding() - get_nr_alighting()) < trip->get_busv()->get_capacity()) 
							{
								// if the bus is not completly full - then the passenger board
								trip->passengers_on_board[(*check_pass)->alighting_decision()].push_back((*check_pass)); 
								set_nr_boarding (get_nr_boarding()+1);
								if (check_pass < pass_waiting_od.end()-1)
								{
									check_pass++;
									next_pass = (*check_pass);
									pass_waiting_od.erase(check_pass-1);
									check_pass = find(pass_waiting_od.begin(),pass_waiting_od.end(),next_pass);
								}
								else
								{
									last_waiting_pass = true;
									break;
								}	
							}
							else 
							{
								// if the bus is already completly full - then the passenger cannot board, even though he wants to
							}
						}
						else
						{
							// if the passenger decides he does NOT want to board this bus
								if (check_pass < pass_waiting_od.end()-1)
								{
									check_pass++;
								}
								else
								{
									last_waiting_pass = true;
									break;
								}	
						}
					}
					if (last_waiting_pass == true)
					{
						pass_waiting_od.erase(pass_waiting_od.end()-1);
					}
				}
			}
		}	
	}

	trip->get_busv()->set_occupancy(starting_occupancy + get_nr_boarding() - get_nr_alighting()); // updating the occupancy
	return calc_dwelltime (trip); 
}

double Busstop::calc_dwelltime (Bustrip* trip)  //!< calculates the dwelltime of each bus serving this stop. currently includes: passenger service times ,out of stop, bay/lane		
{
	// Dwell time parameters according to TCQSM
	double dwell_constant = 5.0; 
	double out_of_stop_coefficient = 3.0;
	double bay_coefficient = 7.0;
	double boarding_coefficient = 3.0;	
	double alighting_front_coefficient = 2.8;
	double alighting_rear_coefficient = 1.5;
	double percent_alighting_front = 0.25;
	int boarding_standees;
	int alighting_standees;
	bool crowded = 0;
	double std_error = 2.0;
	/* Lin & Wilson version of dwell time function
	double dwell_constant = 12.5; // Value of the constant component in the dwell time function. 
	double boarding_coefficient = 0.55;	// Should be read as an input parameter. Would be different for low floor for example.
	double alighting_coefficient = 0.23;
	double crowdedness_coefficient = 0.0078;
	double out_of_stop_coefficient = 3.0; // Taking in consideration the increasing dwell time when bus stops out of the stop
	double bay_coefficient = 4.0;
	*/
	if (trip->get_busv()->get_occupancy() > trip->get_busv()->get_number_seats())	// Calculating alighting standees 
	{ 
		alighting_standees = trip->get_busv()->get_occupancy() - trip->get_busv()->get_number_seats();	
	}
	else	
	{
		alighting_standees = 0;
	}
	
	if (trip->get_busv()->get_occupancy() > 45)
	{
		crowded = 1;
	}
	if (trip->get_busv()->get_occupancy() > trip->get_busv()->get_number_seats()) // Calculating the boarding standess
	{
		boarding_standees = trip->get_busv()->get_occupancy() - trip->get_busv()->get_number_seats();
	}
	else 
	{
		boarding_standees = 0;
	}        

	/* Lin & Wilson version of dwell time function
	loadfactor = get_nr_boarding() * alighting_standees + get_nr_alighting() * boarding_standees;
	dwelltime = (dwell_constant + boarding_coefficient*get_nr_boarding() + alighting_coefficient*get_nr_alighting() 
		+ crowdedness_coefficient*loadfactor + get_bay() * bay_coefficient + out_of_stop_coefficient*out_of_stop); // Lin&Wilson (1992) + out of stop effect. 
	*/	
	
	double time_front_door = boarding_coefficient * get_nr_boarding() + alighting_front_coefficient * percent_alighting_front * get_nr_alighting() + 0.5 * crowded * get_nr_boarding();
	double time_rear_door = alighting_rear_coefficient * (1-percent_alighting_front) * get_nr_alighting();
	dwelltime = get_bay()* bay_coefficient + out_of_stop_coefficient * check_out_of_stop(trip->get_busv()) + 
			max(time_front_door, time_rear_door) + dwell_constant + random->nrandom(0, std_error);
	return max (dwelltime, dwell_constant);
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

void Busstop::update_last_arrivals (Bustrip* trip, double time) // everytime a bus ENTERS a stop it should be updated, 
// in order to keep an updated vector of the last arrivals from each line. 
// the time paramater which is sent is the enter_time, cause headways are defined as the differnece in time between sequential arrivals
{ 
	last_arrivals [trip->get_line()] = time;
}

void Busstop::update_last_departures (Bustrip* trip, double time) // everytime a bus EXITS a stop it should be updated, 
// in order to keep an updated vector of the last deparures from each line. 
// the time paramater which is sent is the exit_time, cause headways are defined as the differnece in time between sequential arrivals
{ 
	last_departures [trip->get_line()] = time;
}

double Busstop::get_time_since_arrival (Bustrip* trip, double time) // calculates the headway (between arrivals)
{  
	double time_since_arrival = time - last_arrivals[trip->get_line()];
	// the headway is defined as the differnece in time between sequential arrivals
	return time_since_arrival;
}

double Busstop::get_time_since_departure (Bustrip* trip, double time) // calculates the headway (between departures)
{  
	if (trip->get_line()->check_first_trip(trip)==true && trip->get_line()->check_first_stop(this)==true) // for the first stop on the first trip on that line - use the planned headway value
	{
		return trip->get_line()->get_average_headway();
	}
	double time_since_departure = time - last_departures[trip->get_line()];
	// the headway is defined as the differnece in time between sequential departures
	return time_since_departure;
}

double Busstop::calc_exiting_time (Bustrip* trip, double time)
{
	dwelltime = passenger_activity_at_stop (trip,time);
	double ready_to_depart = time + dwelltime;
	switch (trip->get_line()->get_holding_strategy())
	{
		// for no control:
		case 0:
			return time + dwelltime; // since it isn't a time-point stop, it will simply exit after dwell time
		// for headway based:
		case 1:
			if (trip->get_line()->is_line_timepoint(this) == true) // if it is a time point
			{
				double holding_departure_time = last_departures[trip->get_line()] + (trip->get_line()->get_average_headway() * trip->get_line()->get_ratio_headway_holding());  
				
				// account for passengers that board while the bus is holded at the time point
				double holding_time = last_departures[trip->get_line()] - time - dwelltime;
				int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
				nr_boarding += additional_boarding;
				int curr_occupancy = trip->get_busv()->get_occupancy();  
				trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy

				return max(ready_to_depart, holding_departure_time);
			}

		// for schedule based:
		case 2:

			if (trip->get_line()->is_line_timepoint(this) == true) // if it is a time point
			{
				double scheduled_departure_time = trip->scheduled_arrival_time(this);  
				// account for passengers that board while the bus is holded at the time point
				double holding_time = last_departures[trip->get_line()] - time - dwelltime;
				int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
				nr_boarding += additional_boarding;
				int curr_occupancy = trip->get_busv()->get_occupancy();  
				trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy

				return max(ready_to_depart, scheduled_departure_time);
			}

		default:
			return time + dwelltime;
	}
}


void Busstop::record_busstop_visit ( Bustrip* trip, double enter_time)  // creates a log-file for stop-related info
{
	
	output_stop_visits.push_back(Busstop_Visit(trip->get_line()->get_id(), trip->get_id() , trip->get_busv()->get_bus_id() , get_id() , enter_time,
			trip->scheduled_arrival_time (this),dwelltime,(enter_time - trip->scheduled_arrival_time (this)), exit_time, get_time_since_arrival (trip , enter_time),
			get_time_since_departure (trip , exit_time),get_nr_alighting() , get_nr_boarding() , trip->get_busv()->get_occupancy(), get_nr_waiting(trip),exit_time-enter_time-dwelltime)); 
	
}

void Busstop::write_output(ostream & out)
{
	for (list <Busstop_Visit>::iterator iter = output_stop_visits.begin(); iter!=output_stop_visits.end();iter++)
	{
		iter->write(out);
	}
}




