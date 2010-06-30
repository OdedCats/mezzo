///! busline.cpp: implementation of the busline class.

#include "busline.h"
#include <math.h>
#include "MMath.h"
#include <sstream>
#include <stddef.h>

template<class T>
struct compare
{
	compare(int id_):id(id_) {}
	bool operator () (T* thing)

	{
		return (thing->get_id()==id);
	}

	int id;
};

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

Busline::Busline (int id_, int opposite_id_, string name_, Busroute* busroute_, vector<Busstop*> stops_, Vtype* vtype_, ODpair* odpair_, int holding_strategy_, float ratio_headway_holding_, int init_occupancy_):
	id(id_), opposite_id(opposite_id_), name(name_), busroute(busroute_), stops(stops_), vtype(vtype_), odpair(odpair_), holding_strategy(holding_strategy_), ratio_headway_holding(ratio_headway_holding_), init_occupancy(init_occupancy_)
{
	active=false;
}

Busline::~Busline()
{}

void Busline::reset ()
{
	active = false;
	curr_trip = trips.begin();
}

void Busline::reset_curr_trip ()
{
	curr_trip = trips.begin();
}

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

vector<Busstop*>::iterator Busline::get_stop_iter (Busstop* stop)
{
	for (vector<Busstop*>::iterator stop_iter = stops.begin(); stop_iter < stops.end(); stop_iter++)
	{
		if ((*stop_iter)->get_id() == stop->get_id())
		{
			return stop_iter;
		}
	}
	return stops.end();
}

void Busline::add_disruptions (Busstop* from_stop, Busstop* to_stop, double disruption_travel_time)
{
	pair<Busstop*,Busstop*> pair_stops;
	pair_stops.first = from_stop;
	pair_stops.second = to_stop;
	disruption_times[pair_stops] = disruption_travel_time;
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

bool Busline::check_last_trip (Bustrip* trip)
{
	if (trip == (trips.end()-1)->first)
	{
		return true;
	}
return false;
}

/*
double Busline::calc_next_scheduled_arrival_at_stop (Busstop* stop, double time)
{
	vector <Visit_stop*> line_stops;
	if (curr_trip == trips.begin())
	{
		line_stops = (*curr_trip).first->stops;
	}
	else
	{
		line_stops = (*(curr_trip-1)).first->stops;
	}	
	vector <Visit_stop*>::iterator decision_stop;
	for (vector <Visit_stop*>::iterator stop_iter = line_stops.begin(); stop_iter < line_stops.end(); stop_iter++)
	{
		Busstop* check_stop = (*stop_iter)->first;
		if (stop->get_id() == check_stop->get_id())
		{
			decision_stop = stop_iter;
			break;
		}
	}
	return (*decision_stop)->second - time;
}
*/

double Busline::find_time_till_next_scheduled_trip_at_stop (Busstop* stop, double time)
{
	for (vector <Start_trip>::iterator trip_iter = trips.begin(); trip_iter < trips.end(); trip_iter++)
	{
		map <Busstop*, double> stop_time = (*trip_iter).first->stops_map;
		if (stop_time[stop] > time)
		// assuming that trips are stored according to their chronological order
		{
			return ((*trip_iter).second - time);
		}
	}
	// currently - in case that there is no additional trip scheduled - return the time till simulation end
	return theParameters->running_time - time;
}

vector<Start_trip>::iterator Busline::find_next_expected_trip_at_stop (Busstop* stop)
{
	if (stop->get_had_been_visited(this) == false)
	{
		return trips.begin(); // if no trip had visited the stop yet then the first trip is the expected next arrival
	}
	Bustrip* next_trip = stop->get_last_trip_departure(this);
	if (next_trip->get_id() == trips.back().first->get_id())
	{
		return trips.end()-1;
	}
	for (vector <Start_trip>::iterator trip_iter = trips.begin(); trip_iter < trips.end(); trip_iter++)
	{
		if ((*trip_iter).first->get_id() == next_trip->get_id())
		{
			return (trip_iter+1); // the trip following the most recent arrival is expected to arrive next
		}
	}
	return trips.end()-1;
}

double Busline::time_till_next_arrival_at_stop_after_time (Busstop* stop, double time)
{
	if (stops.front()->get_had_been_visited(this) == false) 
	// in case no trip started yet - according to time table of the first trip
	{
		return (trips.front().first->stops_map[stop] - time);
	}
	// find the iterator for this stop
	vector<Busstop*>::iterator this_stop;
	for (vector <Busstop*>::iterator stop_iter = stops.begin(); stop_iter < stops.end(); stop_iter++)
	{
		if ((*stop_iter)->get_id() == stop->get_id())
		{
			this_stop = stop_iter;
			break;
		}
	}
	if (stop->get_last_departure(this) > time)
	{
		return stop->get_last_departure(this) - time;
	}
	bool another_trip = false;
	Busstop* last_visited_stop;
	Bustrip* last_dispatched_trip;
	// find the last trip that had visited a downstream stop
	for (vector<Busstop*>::iterator stop_iter = this_stop; stop_iter > stops.begin(); stop_iter--)
	{
		if ((*stop_iter)->get_had_been_visited(this) == true)
		{
			if ((*this_stop)->get_had_been_visited(this) == true)
			{
				if ((*this_stop)->get_last_trip_departure(this)->get_id() == stops.front()->get_last_trip_departure(this)->get_id())
				// a shortcut in case that no later trip had started yet
				{
					last_dispatched_trip = (*this_stop)->get_last_trip_departure(this);
					break;
				}
				if ((*stop_iter)->get_last_trip_departure(this)->get_id() != (*this_stop)->get_last_trip_departure(this)->get_id())
				{
					last_visited_stop = (*stop_iter);
					last_dispatched_trip = last_visited_stop->get_last_trip_departure(this);
					another_trip = true;
					break;
				}
			}
			else // the most recent stop to be first visited
			{
				last_visited_stop = (*stop_iter);
				last_dispatched_trip = last_visited_stop->get_last_trip_departure(this);
				another_trip = true;
				break;
			}	
		}
	} 
	double time_till_next_visit;
	if (another_trip == false)// find the next trip and calc. according to schedule
	{
		vector<Start_trip>::iterator next_trip_to_start = trips.end();	
		for (vector<Start_trip>::iterator trip_iter = trips.begin(); trip_iter < trips.end(); trip_iter++)
		{
			if ((*trip_iter).first->get_id() == stop->get_last_trip_departure(this)->get_id())
			{
				next_trip_to_start = trip_iter+1;
				break;
			}
		}
		if (next_trip_to_start == trips.end()) // no more scheduled trips
		{
			return 100000;
		}
		time_till_next_visit = (*next_trip_to_start).first->stops_map[(*this_stop)] - time; // acoording to the schedule
		while (time_till_next_visit < 0) // if the next trip is planned to visit the stop before the relevant time, then look for the next trip
		{
			next_trip_to_start++;
			if (next_trip_to_start == trips.end()) // no more scheduled trips
			{
				return 100000;
			}
			time_till_next_visit = (*next_trip_to_start).first->stops_map[(*this_stop)] - time; // acoording to the schedule
		}
	}
	else  // in case the next trip have started already
	{
		// find the iterator for last_dispatched_trip
		vector<Start_trip>::iterator last_dispatched_trip_iter;	
		for (vector<Start_trip>::iterator trip_iter = trips.begin(); trip_iter < trips.end(); trip_iter++)
		{
			if ((*trip_iter).first->get_id() == last_dispatched_trip->get_id())
			{
				last_dispatched_trip_iter = trip_iter;
				break;
			}
		}		
		time_till_next_visit = last_visited_stop->get_last_departure(this) + last_dispatched_trip->stops_map[(*this_stop)] - last_dispatched_trip->stops_map[last_visited_stop] - time;
		// calc the expected arrival time at stop according to arrival time at last stop plus shceduled travel time to this stop	
		while (time_till_next_visit < 0)
		{
			last_dispatched_trip_iter++; 
			if (last_dispatched_trip_iter == trips.end()) // no more scheduled trips
			{
				return 100000;
			}
			Visit_stop* last_stop_visit =  *((*last_dispatched_trip_iter).first->get_next_stop());
			if (last_stop_visit->first->get_id() == stops.front()->get_id())
			// if the next trip have not started yet
			{
				time_till_next_visit = (*last_dispatched_trip_iter).first->stops_map[(*this_stop)] - time; // acoording to the schedule
			}
			else
			{
				last_stop_visit =  *((*last_dispatched_trip_iter).first->get_next_stop()-1);
				last_visited_stop = last_stop_visit->first;
				time_till_next_visit = last_visited_stop->get_last_departure(this) + last_dispatched_trip->stops_map[(*this_stop)] - last_dispatched_trip->stops_map[last_visited_stop] - time;
			}
		}
	}
	return time_till_next_visit;
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

double Busline::calc_curr_line_headway_forward ()
{
	if (curr_trip == trips.end()) 
	{
		return ((*(curr_trip-1)).second - (*(curr_trip-2)).second);
	}
	if (curr_trip == trips.begin())
	{
		return ((*(curr_trip+1)).second - (*curr_trip).second);
	}
	if (curr_trip == trips.end()-1) 
	{
		return ((*curr_trip).second - (*(curr_trip-1)).second);
	}
	else
	{
		return ((*(curr_trip+1)).second - (*curr_trip).second);
	}
}

double Busline:: calc_max_headway ()
{
	double max_headway = 0.0;
	for (vector <Start_trip>::iterator start_trip_iter = trips.begin(); start_trip_iter < trips.end()-1; start_trip_iter++)
	{
		max_headway = max (max_headway, (*(start_trip_iter+1)).second - (*(start_trip_iter)).second);
	}
	return max_headway;
}

Bustrip* Busline::get_next_trip (Bustrip* reference_trip) //!< returns the trip after the reference trip on the trips vector
{
	for (vector <Start_trip>::iterator trips_iter = trips.begin(); trips_iter < trips.end(); trips_iter++)
	{
		if ((*trips_iter).first->get_id() == reference_trip->get_id())
		{
			return ((*(trips_iter+1)).first);
		}
	}
	return trips.back().first;
}

Bustrip* Busline::get_previous_trip (Bustrip* reference_trip) //!< returns the trip before the reference trip on the trips vector
{
	for (vector <Start_trip>::iterator trips_iter = trips.begin(); trips_iter < trips.end(); trips_iter++)
	{
		if ((*trips_iter).first->get_id() == reference_trip->get_id())
		{
			return ((*(trips_iter-1)).first);
		}
	}
	return trips.front().first;
}

double Busline::calc_curr_line_ivt (Busstop* start_stop, Busstop* end_stop, int rti)
{
	if (rti == 3)
	{
		pair <Busstop*, Busstop*> pair_stops;
		pair_stops.first = start_stop;
		pair_stops.second = end_stop;
		if (disruption_times.count(pair_stops) > 0)
		{
			return disruption_times[pair_stops];
		}
	}
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
	return ((*alight_stop)->second - (*board_stop)->second); // in seconds
}

void Busline::calculate_sum_output_line()
{
	// initialize all the measures
	output_summary.line_avg_headway = 0;
	output_summary.line_avg_DT = 0;
	output_summary.line_avg_abs_deviation = 0;
	output_summary.line_avg_waiting_per_stop = 0;
	output_summary.line_total_boarding = 0;
	output_summary.line_sd_headway = 0; // average standard deviation over line's stops
	output_summary.line_sd_DT = 0; // average standard deviation over line's stops
	output_summary.line_on_time = 0;
	output_summary.line_early = 0;
	output_summary.line_late = 0;
	
	// accumulating the measures over line's stops
	for (vector<Busstop*>::iterator stop = stops.begin(); stop < stops.end(); stop++)
	{
		output_summary.line_avg_headway += (*stop)->get_output_summary(id).stop_avg_headway;
		output_summary.line_avg_DT += (*stop)->get_output_summary(id).stop_avg_DT;
		output_summary.line_avg_abs_deviation += (*stop)->get_output_summary(id).stop_avg_abs_deviation;
		output_summary.line_avg_waiting_per_stop += (*stop)->get_output_summary(id).stop_avg_waiting_per_stop;
		output_summary.line_total_boarding += (*stop)->get_output_summary(id).stop_total_boarding;
		output_summary.line_sd_headway += (*stop)->get_output_summary(id).stop_sd_headway; 
		output_summary.line_sd_DT += (*stop)->get_output_summary(id).stop_sd_DT;
		output_summary.line_on_time += (*stop)->get_output_summary(id).stop_on_time;
		output_summary.line_early += (*stop)->get_output_summary(id).stop_early;
		output_summary.line_late += (*stop)->get_output_summary(id).stop_late;
	}
	// dividing by the number of stops for average measures
	output_summary.line_avg_headway = output_summary.line_avg_headway / stops.size();
	output_summary.line_avg_DT = output_summary.line_avg_DT / stops.size();
	output_summary.line_avg_abs_deviation = output_summary.line_avg_abs_deviation / stops.size();
	output_summary.line_avg_waiting_per_stop = output_summary.line_avg_waiting_per_stop / stops.size();
	output_summary.line_sd_headway = output_summary.line_sd_headway / stops.size();
	output_summary.line_sd_DT = output_summary.line_sd_DT / stops.size();
	output_summary.line_on_time = output_summary.line_on_time / stops.size();
	output_summary.line_early = output_summary.line_early / stops.size();
	output_summary.line_late = output_summary.line_late / stops.size();
}

void Busline::calc_line_assignment()
{
	for (vector <Busstop*>::iterator stop_nr = stops.begin(); stop_nr < stops.end() - 1; stop_nr++) // initialize
	{
		stop_pass [(*stop_nr)] = 0;
	}
	for (vector <Start_trip>::iterator trip_iter = trips.begin(); trip_iter < trips.end(); trip_iter++) // calculating
	{
		vector <Busstop*>::iterator line_stop = stops.begin();
		list <Bustrip_assign> list_ass = (*trip_iter).first->get_output_passenger_load();
		for (list <Bustrip_assign>::iterator stop_iter = list_ass.begin(); stop_iter != list_ass.end(); stop_iter++)
		{
			stop_pass [(*line_stop)] += (*stop_iter).passenger_load;
			line_stop++;
		}
	}
	for (vector <Busstop*>::iterator stop_nr = stops.begin(); stop_nr < stops.end() - 1; stop_nr++) // recording
	{
		record_passenger_loads(stop_nr);
	}
}

void Busline::record_passenger_loads (vector<Busstop*>::iterator stop_iter)
{
	output_line_assign.push_back(Busline_assign(id, (*stop_iter)->get_id() , (*(stop_iter+1))->get_id()  ,stop_pass[(*stop_iter)]));
}


void Busline::write_assign_output(ostream & out)
{
	for (list <Busline_assign>::iterator iter = output_line_assign.begin(); iter != output_line_assign.end(); iter++)
	{
		(*iter).write(out);
	}
}
// ***** Bustrip Functions *****

Bustrip::Bustrip ()
{
	init_occupancy = line->get_initial_occupancy();
	random = new (Random);
	next_stop = stops.begin();
	for (vector <Visit_stop*>::iterator visit_stop_iter = stops.begin(); visit_stop_iter < stops.end(); visit_stop_iter++)
	{
		assign_segements[(*visit_stop_iter)->first] = 0;
	}
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
}

Bustrip::Bustrip (int id_, double start_time_, Busline* line_): id(id_), starttime(start_time_), line(line_)
{
	init_occupancy = line->get_initial_occupancy();
	random = new (Random);
	next_stop = stops.begin();
	for (vector<Visit_stop*>::iterator visit_stop_iter = stops.begin(); visit_stop_iter < stops.end(); visit_stop_iter++)
	{
		assign_segements[(*visit_stop_iter)->first] = 0;
	}
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

void Bustrip::reset ()
{
	init_occupancy = line->get_initial_occupancy();
	passengers_on_board.clear();
	enter_time = 0;
	next_stop = stops.begin();
	convert_stops_vector_to_map();	
}

void Bustrip::convert_stops_vector_to_map ()
{
	for (vector <Visit_stop*>::iterator stop_iter = stops.begin(); stop_iter < stops.end(); stop_iter++)
	{
		stops_map[(*stop_iter)->first] =(*stop_iter)->second;
	}
}

double Bustrip::calc_departure_time (double time) // calculates departure time from origin according to arrival time and schedule (including layover effect)
{
	double min_recovery = 30.00; 
	double mean_error_recovery = 10.00;
	double std_error_recovery = 10.00;
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
		if (line->get_holding_strategy() == 6 || line->get_holding_strategy() == 7)
		{
			if (stops.front()->first->get_time_since_departure(this,time) < line->calc_curr_line_headway())
			{
				return (time + min_recovery + random->lnrandom (mean_error_recovery, std_error_recovery));
			}
		}
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
	next_stop = stops.begin();
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
		//vid++;
		//Bus* new_bus=recycler.newBus(); // then generate a new vehicle
		//new_bus->set_bustype_attributes (btype);
		//busv =new_bus;
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
	busv->set_occupancy(init_occupancy);
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

void Bustrip::write_assign_segments_output(ostream & out)
{
	for (list <Bustrip_assign>::iterator iter = output_passenger_load.begin(); iter != output_passenger_load.end(); iter++)
	{
		(*iter).write(out);
	}
}

void Bustrip::record_passenger_loads (vector <Visit_stop*>::iterator start_stop)
{	
	output_passenger_load.push_back(Bustrip_assign(id, line->get_id() , busv->get_id(), (*start_stop)->first->get_id() , (*(start_stop+1))->first->get_id()  ,assign_segements[(*start_stop)->first]));
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
	rti = 0;
}

Busstop::Busstop (int id_, string name_, int link_id_, double position_, double length_, bool has_bay_, double dwelltime_, int rti_):
	id(id_), name(name_), link_id(link_id_), position (position_), length(length_), has_bay(has_bay_), dwelltime(dwelltime_), rti (rti_)
{
	avaliable_length = length;
	nr_boarding = 0;
	nr_alighting = 0;
	is_origin = false;
	is_destination = false;
	last_arrivals.clear();
	last_departures.clear();
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

void Busstop::reset()
{
	avaliable_length = 0;
	nr_boarding = 0;
	nr_alighting = 0;
	is_origin = false;
	is_destination = false;
	dwelltime = 0;
	exit_time = 0;
	expected_arrivals.clear();
	buses_at_stop.clear();
	last_arrivals.clear();
	last_departures.clear();
	multi_nr_waiting.clear();
	nr_waiting.clear();
	output_stop_visits.clear();
	output_summary.clear();
}
void Busstop::book_bus_arrival(Eventlist* eventlist, double time, Bus* bus)
{
	// books an event for the arrival of a bus at a bus stop by adding it to the expected arrivals at the stop 
	double book_time;
	if (expected_arrivals[time] == NULL)
	{
		expected_arrivals [time] = bus; // not sure if we really want to index them by time? maybe simply by bus id?
		book_time = time;
	}
	else
	{
		book_time = time + 0.01* random->urandom();
		expected_arrivals [book_time] = bus;
	}
	eventlist->add_event(book_time,this);
} 

bool Busstop::execute(Eventlist* eventlist, double time) // is executed by the eventlist and means a bus needs to be processed
{
  	// progress the vehicle when entering or exiting a stop
	
	if (theParameters->demand_format == 3)
	{
		for (map <Busstop*, ODstops*>::iterator destination_stop = stop_as_origin.begin(); destination_stop != stop_as_origin.end(); destination_stop++)
		{
			//if (id != destination_stop->first->get_id())
			//{
				passengers pass_waiting_od = (*destination_stop).second->get_waiting_passengers();		
				passengers::iterator check_pass = pass_waiting_od.begin();
				//Passenger* next_pass;
				//bool last_waiting_pass = false;
				while (check_pass < pass_waiting_od.end())
				{
					// progress each waiting passenger   
					if ((*check_pass)->get_OD_stop()->get_origin()->get_id() != this->get_id())
					{
						break;
					}
					check_pass++;
				}
			//}
		}
	}
	
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
		exit_time = calc_exiting_time(eventlist, bus->get_curr_trip(), time); // get the expected exit time according to dwell time calculations and time point considerations
		occupy_length (bus);
		buses_at_stop [exit_time] = bus; 
		eventlist->add_event (exit_time, this); // book an event for the time it exits the stop
		record_busstop_visit ( bus->get_curr_trip(), bus->get_curr_trip()->get_enter_time()); // document stop-related info
								// done BEFORE update_last_arrivals in order to calc the headway
		bus->record_busvehicle_location (bus->get_curr_trip(), this, bus->get_curr_trip()->get_enter_time());
		bus->get_curr_trip()->advance_next_stop(exit_time, eventlist); 
		update_last_arrivals (bus->get_curr_trip(), bus->get_curr_trip()->get_enter_time()); // in order to follow the arrival times (AFTER dwell time is calculated)
		update_last_departures (bus->get_curr_trip(), exit_time); // in order to follow the departure times (AFTER the dwell time and time point stuff)
		set_had_been_visited (bus->get_curr_trip()->get_line(), true);
		expected_arrivals.erase(time);
	}
	return true;
}
double Busstop::passenger_activity_at_stop (Eventlist* eventlist, Bustrip* trip, double time) //!< progress passengers at stop: waiting, boarding and alighting
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
	if (theParameters->demand_format == 10) // format 1 with time-dependent demand slices
	{
		// find the time that this slice started
		vector<double>::iterator curr_slice_start;
		bool simulation_start_slice = true;
		for (vector<double>::iterator iter_slices = update_rates_times[trip->get_line()].begin(); iter_slices != update_rates_times[trip->get_line()].end(); iter_slices++)
		{
			if (iter_slices == update_rates_times[trip->get_line()].begin() && time > (*iter_slices))
				// it is before the first slice
			{
				simulation_start_slice = false;
			}
			if ((iter_slices+1) == update_rates_times[trip->get_line()].end() && time > (*iter_slices))
				// it is the last slice
			{
				curr_slice_start = iter_slices;
				break;
			}
			if ((*iter_slices)< time && (*(iter_slices+1)) >= time)
			{
				curr_slice_start = iter_slices;
				break;
			}
		}
		if (simulation_start_slice == false && (time - get_time_since_arrival(trip,time) < (*curr_slice_start))) 
		// if the previous bus from this line arrives on the previous slice - then take into account previous rate
		{
			double time_previous_slice = ((*curr_slice_start) - (time - get_time_since_arrival(trip,time)));
			double time_current_slice = time - (*curr_slice_start);
			nr_waiting [trip->get_line()] += random -> poisson ((previous_arrival_rates[trip->get_line()] * time_previous_slice) / 3600.0 ) + random -> poisson (((get_arrival_rates (trip)) * time_current_slice) / 3600.0 );
			// sum of two poisson processes - pass. arriving during the leftover of the previous lsice plus pass. arriving during the begining of this slice
			double weighted_alighting_fraction = (get_alighting_fractions (trip) * time_current_slice + previous_alighting_fractions[trip->get_line()] * time_previous_slice) / (time_current_slice + time_previous_slice);
			if (starting_occupancy > 0 && weighted_alighting_fraction > 0)
			{
				set_nr_alighting (random -> binrandom (starting_occupancy, weighted_alighting_fraction)); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the weighted alighting fraction
			}
		}
		else // take into account only current rates
		{
			nr_waiting [trip->get_line()] += random -> poisson (((get_arrival_rates (trip)) * get_time_since_arrival (trip, time)) / 3600.0 );
				//the arrival process follows a poisson distribution and the lambda is relative to the headway
				// with arrival time and the headway as the duration
			if (starting_occupancy > 0 && get_alighting_fractions (trip) > 0) 
			{
				set_nr_alighting (random -> binrandom (starting_occupancy, get_alighting_fractions (trip))); // the alighting process follows a binominal distribution 
					// the number of trials is the number of passengers on board with the probability of the alighting fraction
			}
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
	if (theParameters->demand_format == 1 || theParameters->demand_format == 2 || theParameters->demand_format == 10) // in the case of non-individual passengers - boarding progress for waiting passengers (capacity constraints)
	{	
		if (trip->get_busv()->get_capacity() - (starting_occupancy - get_nr_alighting()) < nr_waiting [trip->get_line()]) // if the capcity determines the boarding process
		{	
			if (theParameters->demand_format == 1 || theParameters->demand_format == 10)
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
		set_nr_alighting (trip->passengers_on_board[this].size()); 
		for (vector <Passenger*> ::iterator alighting_passenger = trip->passengers_on_board[this].begin(); alighting_passenger < trip->passengers_on_board[this].end(); alighting_passenger++)
		{
			(*alighting_passenger)->add_to_selected_path_stop(this);
			Busstop* next_stop;	
			if (id == (*alighting_passenger)->get_OD_stop()->get_destination()->get_id()) // if this stop is passenger's destination
			{
				// passenger has no further conection choice
				next_stop = this;
				(*alighting_passenger)->add_to_selected_path_stop(this);
				(*alighting_passenger)->set_end_time(time);
				pass_recycler.addPassenger(*alighting_passenger); // terminate passenger
			}
			else
			{
				// if this stop is not passenger's final destination then make a connection decision
				(*alighting_passenger)->set_ODstop(stop_as_origin[(*alighting_passenger)->get_OD_stop()->get_destination()]); // set the connected stop as passenger's new origin (new OD)
				next_stop =(*alighting_passenger)->make_connection_decision(time);
				// set connected_stop as the new origin
				ODstops* new_od = next_stop->get_stop_od_as_origin_per_stop((*alighting_passenger)->get_OD_stop()->get_destination());
				(*alighting_passenger)->set_ODstop(new_od); // set the connected stop as passenger's new origin (new OD)
				ODstops* odstop = (*alighting_passenger)->get_OD_stop();
				if (odstop->get_waiting_passengers().size() != 0)
				{
					if (next_stop->get_id() == this->get_id())  // pass stays at the same stop
					{
						passengers wait_pass = odstop->get_waiting_passengers(); // add passanger's to the waiting queue on the new OD
						wait_pass.push_back (*alighting_passenger);
						odstop->set_waiting_passengers(wait_pass);
					}
					else  // pass walks to another stop
					{
						// booking an event to the arrival time at the new stop
						(*alighting_passenger)->execute(eventlist, time + distances[next_stop] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
					}
				}
				else
				{
					passengers wait_pass;
					wait_pass.push_back(*alighting_passenger);
					odstop->set_waiting_passengers(wait_pass);
				}
			}
		}
		trip->passengers_on_board[this].clear(); 
		// * Boarding passengers *

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
					if ((*check_pass)->make_boarding_decision(trip, time) == true)
					{
						// if the passenger decided to board this bus
						if ((starting_occupancy + get_nr_boarding() - get_nr_alighting()) < trip->get_busv()->get_capacity()) 
						{
							// if the bus is not completly full - then the passenger boards
							trip->passengers_on_board[(*check_pass)->make_alighting_decision(trip, time)].push_back((*check_pass)); 
							// currently - alighting decision is made when boarding
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
								pass_waiting_od.erase(check_pass);
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
				(*destination_stop).second->set_waiting_passengers(pass_waiting_od); // updating the waiting list at the ODstops object (deleting boarding pass.)
			}
		}	
	}	
	trip->get_busv()->set_occupancy(starting_occupancy + get_nr_boarding() - get_nr_alighting()); // updating the occupancy
	if (id != trip->stops.back()->first->get_id()) // if it is not the last stop for this trip
	{
		trip->assign_segements[this] = trip->get_busv()->get_occupancy();
		trip->record_passenger_loads(trip->get_next_stop());
	}
	return calc_dwelltime (trip); 
}

double Busstop::calc_dwelltime (Bustrip* trip)  //!< calculates the dwelltime of each bus serving this stop. currently includes: passenger service times ,out of stop, bay/lane		
{
	// Dwell time parameters according to SL
	double dwell_constant = 0.0; 
	double out_of_stop_coefficient = 2.0;
	double bay_coefficient = 2.0;
	double boarding_coefficient = 2.0;	
	double alighting_front_coefficient = 1.0;
	double alighting_rear_coefficient = 1.0;
	double percent_alighting_front = 0.0;
	int boarding_standees;
	int alighting_standees;
	bool crowded = 0;
	double std_error = 1.0;
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
	pair <Bustrip*, double> trip_time;
	trip_time.first = trip;
	trip_time.second = time;
	last_arrivals [trip->get_line()] = trip_time;
}

void Busstop::update_last_departures (Bustrip* trip, double time) // everytime a bus EXITS a stop it should be updated, 
// in order to keep an updated vector of the last deparures from each line. 
// the time paramater which is sent is the exit_time, cause headways are defined as the differnece in time between sequential arrivals
{ 
	pair <Bustrip*, double> trip_time;
	trip_time.first = trip;
	trip_time.second = time;
	last_departures [trip->get_line()] = trip_time;
}

double Busstop::get_time_since_arrival (Bustrip* trip, double time) // calculates the headway (between arrivals)
{  
	double time_since_arrival = time - last_arrivals[trip->get_line()].second;
	// the headway is defined as the differnece in time between sequential arrivals
	return time_since_arrival;
}

double Busstop::get_time_since_departure (Bustrip* trip, double time) // calculates the headway (between departures)
{  
	if (trip->get_line()->check_first_trip(trip)==true && trip->get_line()->check_first_stop(this)==true) // for the first stop on the first trip on that line - use the planned headway value
	{
		return trip->get_line()->calc_curr_line_headway();
	}
	double time_since_departure = time - last_departures[trip->get_line()].second;
	// the headway is defined as the differnece in time between sequential departures
	return time_since_departure;
}

double Busstop::calc_exiting_time (Eventlist* eventlist, Bustrip* trip, double time)
{
	dwelltime = passenger_activity_at_stop (eventlist, trip,time);
	double ready_to_depart = time + dwelltime;
	switch (trip->get_line()->get_holding_strategy())
	{
		// for no control:
		case 0:
			return time + dwelltime; // since it isn't a time-point stop, it will simply exit after dwell time
		// for headway based (looking on headway from preceding bus):
		case 1:
			if (trip->get_line()->is_line_timepoint(this) == true) // if it is a time point
			{
				double holding_departure_time = last_departures[trip->get_line()].second + (trip->get_line()->calc_curr_line_headway() * trip->get_line()->get_ratio_headway_holding());  
				
				// account for passengers that board while the bus is holded at the time point
				double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
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
				double scheduled_departure_time = time + (trip->scheduled_arrival_time(this)-time)* trip->get_line()->get_ratio_headway_holding();  
				// account for passengers that board while the bus is holded at the time point
				double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
				int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
				nr_boarding += additional_boarding;
				int curr_occupancy = trip->get_busv()->get_occupancy();  
				trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy

				return max(ready_to_depart, scheduled_departure_time);
			}
		// for headway based (looking on headway from proceeding bus)
		case 3:
			if (trip->get_line()->is_line_timepoint(this) == true && trip->get_line()->check_last_trip(trip) == false) // if it is a time point and it is not the last trip
			{
				Bustrip* next_trip = trip->get_line()->get_next_trip(trip);
				vector <Visit_stop*> :: iterator& next_trip_next_stop = next_trip->get_next_stop();
				if ((*next_trip_next_stop)->first != trip->get_line()->stops.front()) // in case the next trip already started wise- no holding (otherwise - no base for holding)
				{
					vector <Visit_stop*> :: iterator curr_stop; // hold the scheduled time for this trip at this stop
					for (vector <Visit_stop*> :: iterator trip_stops = next_trip->stops.begin(); trip_stops < next_trip->stops.end(); trip_stops++)
					{
						if ((*trip_stops)->first->get_id() == this->get_id())
						{
							curr_stop = trip_stops;
							break;
						}
					}
					double expected_next_arrival = (*(next_trip_next_stop-1))->first->get_last_departure(trip->get_line()) + (*curr_stop)->second - (*(next_trip_next_stop-1))->second; // time at last stop + scheduled travel time between stops
					double holding_departure_time = expected_next_arrival - (trip->get_line()->calc_curr_line_headway_forward() * (2 - trip->get_line()->get_ratio_headway_holding())); // headway ratio means here how tolerant we are to exceed the gap (1+(1-ratio)) -> 2-ratio
				
					// account for passengers that board while the bus is holded at the time point
					double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
					int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
					nr_boarding += additional_boarding;
					int curr_occupancy = trip->get_busv()->get_occupancy();  
					trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy

					return max(ready_to_depart, holding_departure_time);
				}
			}
			// for headway-based (looking both backward and forward and averaging)
			case 4:
			if (trip->get_line()->is_line_timepoint(this) == true && trip->get_line()->check_last_trip(trip) == false && trip->get_line()->check_first_trip(trip) == false) // if it is a time point and it is not the first or last trip
			{
					Bustrip* next_trip = trip->get_line()->get_next_trip(trip);
					Bustrip* previous_trip = trip->get_line()->get_previous_trip(trip);
					vector <Visit_stop*> :: iterator& next_trip_next_stop = next_trip->get_next_stop();
					if (next_trip->check_end_trip() == false && (*next_trip_next_stop)->first != trip->get_line()->stops.front()&& previous_trip->check_end_trip() == false) // in case the next trip already started and the previous trip hadn't finished yet(otherwise - no base for holding)
					{
						vector <Visit_stop*> :: iterator curr_stop; // hold the scheduled time for this trip at this stop
						for (vector <Visit_stop*> :: iterator trip_stops = next_trip->stops.begin(); trip_stops < next_trip->stops.end(); trip_stops++)
						{
							if ((*trip_stops)->first->get_id() == this->get_id())
							{
								curr_stop = trip_stops;
								break;
							}
						}
						double expected_next_arrival = (*(next_trip_next_stop-1))->first->get_last_departure(trip->get_line()) + (*curr_stop)->second - (*(next_trip_next_stop-1))->second; // time at last stop + scheduled travel time between stops
						double average_curr_headway = ((expected_next_arrival - time) + (time - last_departures[trip->get_line()].second))/2; // average of the headway in front and behind
						// double average_planned_headway = (trip->get_line()->calc_curr_line_headway_forward() + trip->get_line()->calc_curr_line_headway())/2;
						double holding_departure_time = last_departures[trip->get_line()].second + average_curr_headway; // headway ratio means here how tolerant we are to exceed the gap (1+(1-ratio)) -> 2-ratio
				
						// account for passengers that board while the bus is holded at the time point
						double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
						int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
						nr_boarding += additional_boarding;
						int curr_occupancy = trip->get_busv()->get_occupancy();  
						trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy
		
						return max(ready_to_depart, holding_departure_time);
					}
			}
			// for headway-based (looking both backward and forward and averaging) with max. according to planned headway
			case 5:
			if (trip->get_line()->is_line_timepoint(this) == true && trip->get_line()->check_last_trip(trip) == false && trip->get_line()->check_first_trip(trip) == false) // if it is a time point and it is not the first or last trip
			{
					Bustrip* next_trip = trip->get_line()->get_next_trip(trip);
					Bustrip* previous_trip = trip->get_line()->get_previous_trip(trip);
					vector <Visit_stop*> :: iterator& next_trip_next_stop = next_trip->get_next_stop();
					if (next_trip->check_end_trip() == false && (*next_trip_next_stop)->first != trip->get_line()->stops.front()&& previous_trip->check_end_trip() == false) // in case the next trip already started and the previous trip hadn't finished yet(otherwise - no base for holding)
					{
						vector <Visit_stop*> :: iterator curr_stop; // hold the scheduled time for this trip at this stop
						for (vector <Visit_stop*> :: iterator trip_stops = next_trip->stops.begin(); trip_stops < next_trip->stops.end(); trip_stops++)
						{
							if ((*trip_stops)->first->get_id() == this->get_id())
							{
								curr_stop = trip_stops;
								break;
							}
						}
						double expected_next_arrival = (*(next_trip_next_stop-1))->first->get_last_departure(trip->get_line()) + (*curr_stop)->second - (*(next_trip_next_stop-1))->second; // time at last stop + scheduled travel time between stops
						double average_curr_headway = ((expected_next_arrival - time) + (time - last_departures[trip->get_line()].second))/2; // average of the headway in front and behind
						// double average_planned_headway = (trip->get_line()->calc_curr_line_headway_forward() + trip->get_line()->calc_curr_line_headway())/2;
						double holding_departure_time = min(last_departures[trip->get_line()].second + average_curr_headway, last_departures[trip->get_line()].second + (trip->get_line()->calc_curr_line_headway() * trip->get_line()->get_ratio_headway_holding())); // headway ratio means here how tolerant we are to exceed the gap (1+(1-ratio)) -> 2-ratio
				
						// account for passengers that board while the bus is holded at the time point
						double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
						int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
						nr_boarding += additional_boarding;
						int curr_occupancy = trip->get_busv()->get_occupancy();  
						trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy
		
						return max(ready_to_depart, holding_departure_time);
				}
			}
			// for headway-based (looking both backward and forward and averaging)
			case 6:
			if (trip->get_line()->is_line_timepoint(this) == true && trip->get_line()->check_last_trip(trip) == false && trip->get_line()->check_first_trip(trip) == false) // if it is a time point and it is not the first or last trip
			{
					Bustrip* next_trip = trip->get_line()->get_next_trip(trip);
					Bustrip* previous_trip = trip->get_line()->get_previous_trip(trip);
					vector <Visit_stop*> :: iterator& next_trip_next_stop = next_trip->get_next_stop();
					if (next_trip->check_end_trip() == false && (*next_trip_next_stop)->first != trip->get_line()->stops.front()&& previous_trip->check_end_trip() == false) // in case the next trip already started and the previous trip hadn't finished yet(otherwise - no base for holding)
					{
						vector <Visit_stop*> :: iterator curr_stop; // hold the scheduled time for this trip at this stop
						for (vector <Visit_stop*> :: iterator trip_stops = next_trip->stops.begin(); trip_stops < next_trip->stops.end(); trip_stops++)
						{
							if ((*trip_stops)->first->get_id() == this->get_id())
							{
								curr_stop = trip_stops;
								break;
							}
						}
						double expected_next_arrival = (*(next_trip_next_stop-1))->first->get_last_departure(trip->get_line()) + (*curr_stop)->second - (*(next_trip_next_stop-1))->second; // time at last stop + scheduled travel time between stops
						double average_curr_headway = ((expected_next_arrival - time) + (time - last_departures[trip->get_line()].second))/2; // average of the headway in front and behind
						// double average_planned_headway = (trip->get_line()->calc_curr_line_headway_forward() + trip->get_line()->calc_curr_line_headway())/2;
						double holding_departure_time = last_departures[trip->get_line()].second + average_curr_headway; // headway ratio means here how tolerant we are to exceed the gap (1+(1-ratio)) -> 2-ratio
				
						// account for passengers that board while the bus is holded at the time point
						double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
						int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
						nr_boarding += additional_boarding;
						int curr_occupancy = trip->get_busv()->get_occupancy();  
						trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy
		
						return max(ready_to_depart, holding_departure_time);
					}
			}
			// for headway-based (looking both backward and forward and averaging) with max. according to planned headway
			case 7:
			if (trip->get_line()->is_line_timepoint(this) == true && trip->get_line()->check_last_trip(trip) == false && trip->get_line()->check_first_trip(trip) == false) // if it is a time point and it is not the first or last trip
			{
					Bustrip* next_trip = trip->get_line()->get_next_trip(trip);
					Bustrip* previous_trip = trip->get_line()->get_previous_trip(trip);
					vector <Visit_stop*> :: iterator& next_trip_next_stop = next_trip->get_next_stop();
					if (next_trip->check_end_trip() == false && (*next_trip_next_stop)->first != trip->get_line()->stops.front()&& previous_trip->check_end_trip() == false) // in case the next trip already started and the previous trip hadn't finished yet(otherwise - no base for holding)
					{
						vector <Visit_stop*> :: iterator curr_stop; // hold the scheduled time for this trip at this stop
						for (vector <Visit_stop*> :: iterator trip_stops = next_trip->stops.begin(); trip_stops < next_trip->stops.end(); trip_stops++)
						{
							if ((*trip_stops)->first->get_id() == this->get_id())
							{
								curr_stop = trip_stops;
								break;
							}
						}
						double expected_next_arrival = (*(next_trip_next_stop-1))->first->get_last_departure(trip->get_line()) + (*curr_stop)->second - (*(next_trip_next_stop-1))->second; // time at last stop + scheduled travel time between stops
						double average_curr_headway = ((expected_next_arrival - time) + (time - last_departures[trip->get_line()].second))/2; // average of the headway in front and behind
						// double average_planned_headway = (trip->get_line()->calc_curr_line_headway_forward() + trip->get_line()->calc_curr_line_headway())/2;
						double holding_departure_time = min(last_departures[trip->get_line()].second + average_curr_headway, last_departures[trip->get_line()].second + (trip->get_line()->calc_curr_line_headway() * trip->get_line()->get_ratio_headway_holding())); // headway ratio means here how tolerant we are to exceed the gap (1+(1-ratio)) -> 2-ratio
				
						// account for passengers that board while the bus is holded at the time point
						double holding_time = last_departures[trip->get_line()].second - time - dwelltime;
						int additional_boarding = random -> poisson ((get_arrival_rates (trip)) * holding_time / 3600.0 );
						nr_boarding += additional_boarding;
						int curr_occupancy = trip->get_busv()->get_occupancy();  
						trip->get_busv()->set_occupancy(curr_occupancy + additional_boarding); // Updating the occupancy
		
						return max(ready_to_depart, holding_departure_time);
				}
			}
		default:
			return time + dwelltime;
	}
}

int Busstop::calc_total_nr_waiting ()
{
	int total_nr_waiting = 0;
	// formats 1 and 2
	if (theParameters->demand_format == 1 || theParameters->demand_format == 2)
	{
		for (vector<Busline*>::iterator lines_at_stop = lines.begin(); lines_at_stop < lines.end(); lines_at_stop++)
		{
			total_nr_waiting += nr_waiting[(*lines_at_stop)];
		}
	}
	// format 3
	else if (theParameters->demand_format ==3)
	{
		for (map <Busstop*, ODstops*>::iterator destination_stop = stop_as_origin.begin(); destination_stop != stop_as_origin.end(); destination_stop++)
				// going through all the stops that this stop is their origin on a given OD pair
		{
			total_nr_waiting += (*destination_stop).second->get_waiting_passengers().size();
		}
	}
return total_nr_waiting;
}

void Busstop::record_busstop_visit (Bustrip* trip, double enter_time)  // creates a log-file for stop-related info
{
	output_stop_visits.push_back(Busstop_Visit(trip->get_line()->get_id(), trip->get_id() , trip->get_busv()->get_bus_id() , get_id() , enter_time,
			trip->scheduled_arrival_time (this),dwelltime,(enter_time - trip->scheduled_arrival_time (this)), exit_time, get_time_since_arrival (trip , enter_time),
			get_time_since_departure (trip , exit_time),get_nr_alighting() , get_nr_boarding() , trip->get_busv()->get_occupancy(), calc_total_nr_waiting() ,exit_time-enter_time-dwelltime)); 
}

void Busstop::write_output(ostream & out)
{
	for (list <Busstop_Visit>::iterator iter = output_stop_visits.begin(); iter!=output_stop_visits.end();iter++)
	{
		iter->write(out);
	}
}

void Busstop::calculate_sum_output_stop_per_line(int line_id)
{
	int counter = 0;
	// initialize all output measures
	Busline* bl=(*(find_if(lines.begin(), lines.end(), compare <Busline> (line_id) )));
	vector<Start_trip> trips = bl->get_trips();
	output_summary[line_id].stop_avg_headway = 0;
	output_summary[line_id].stop_avg_DT = 0;
	output_summary[line_id].stop_avg_abs_deviation = 0;
	output_summary[line_id].stop_total_boarding = 0;
	output_summary[line_id].stop_avg_waiting_per_stop = 0;
	output_summary[line_id].stop_sd_headway = 0;
	output_summary[line_id].stop_sd_DT = 0;
	output_summary[line_id].stop_on_time = 0;
	output_summary[line_id].stop_early = 0;
	output_summary[line_id].stop_late = 0;
	for (list <Busstop_Visit>::iterator iter1 = output_stop_visits.begin(); iter1!=output_stop_visits.end();iter1++)
	{
		if ((*iter1).line_id == line_id)
			{
				// accumulating all the measures
				counter++; // should equal the total number of trips for this bus line passing at this bus stop
				if (trips.size()>2)
				{
					vector<Start_trip>::iterator iter = trips.begin();
					if ((*iter1).trip_id != (*iter).first->get_id())
					{
						iter++;
						if ((*iter1).trip_id != (*iter).first->get_id())
						{
							output_summary[line_id].stop_avg_headway += (*iter1).time_since_dep;
						}
					}
				}
				else 
				{
					output_summary[line_id].stop_avg_headway += (*iter1).time_since_dep;
				}
				output_summary[line_id].stop_avg_DT += (*iter1).dwell_time;
				output_summary[line_id].stop_avg_abs_deviation += abs((*iter1).lateness);
				output_summary[line_id].stop_total_boarding += (*iter1).nr_boarding;
				output_summary[line_id].stop_avg_waiting_per_stop += (*iter1).nr_waiting;
				if ((*iter1).lateness > 300)
				{
					output_summary[line_id].stop_late ++;
				}
				else if ((*iter1).lateness < -60)
				{
					output_summary[line_id].stop_early ++;
				}
				else 
				{
					output_summary[line_id].stop_on_time ++;
				}
			}
	}
	// dividing all the average measures by the number of records	
	if (trips.size()>2)	
	{
		output_summary[line_id].stop_avg_headway = output_summary[line_id].stop_avg_headway/(counter-2);
	}
	else
	{
		output_summary[line_id].stop_avg_headway = output_summary[line_id].stop_avg_headway/(counter);
	}
	output_summary[line_id].stop_avg_DT = output_summary[line_id].stop_avg_DT/counter;
	output_summary[line_id].stop_avg_abs_deviation = output_summary[line_id].stop_avg_abs_deviation/counter;
	output_summary[line_id].stop_total_boarding = output_summary[line_id].stop_total_boarding/counter;
	output_summary[line_id].stop_avg_waiting_per_stop = output_summary[line_id].stop_avg_waiting_per_stop/counter;
	output_summary[line_id].stop_on_time = output_summary[line_id].stop_on_time/counter;
	output_summary[line_id].stop_early = output_summary[line_id].stop_early/counter;
	output_summary[line_id].stop_late = output_summary[line_id].stop_late/counter;

	// now go over again for SD calculations
	for (list <Busstop_Visit>::iterator iter1 = output_stop_visits.begin(); iter1!=output_stop_visits.end();iter1++)
	{
		if ((*iter1).line_id == line_id)
		{
			vector<Start_trip>::iterator iter = trips.begin();
			if (trips.size()>2)
			{
				if ((*iter1).trip_id != (*iter).first->get_id())
				{
					iter++;
					if ((*iter1).trip_id != (*iter).first->get_id())
					{
						output_summary[line_id].stop_sd_headway += pow ((*iter1).time_since_dep - output_summary[line_id].stop_avg_headway, 2);
					}
				}
			}
			else 
			{
				output_summary[line_id].stop_sd_headway += pow ((*iter1).time_since_dep - output_summary[line_id].stop_avg_headway, 2);
			}
			output_summary[line_id].stop_sd_DT += pow ((*iter1).dwell_time - output_summary[line_id].stop_avg_DT, 2);
		}
	}
	// finish calculating all the SD measures 
	if (trips.size()>2)	
	{
		output_summary[line_id].stop_sd_headway = sqrt(output_summary[line_id].stop_sd_headway/(counter-3));
	}
	else
	{
		output_summary[line_id].stop_sd_headway = sqrt(output_summary[line_id].stop_sd_headway/(counter-1));
	}
	output_summary[line_id].stop_sd_DT = sqrt(output_summary[line_id].stop_sd_DT/(counter-1));
}

const bool Busstop::check_walkable_stop ( Busstop* const & stop)
{
	if (distances.count(stop) > 0)
	{
		return true;
	}
	return false;
}

bool Busstop::check_destination_stop (Busstop* stop)
{
	if (stop_as_origin.count(stop) > 0)
	{
		return true;
	}
	return false;
}

Change_arrival_rate::Change_arrival_rate(double time)
{
	loadtime = time;	
}

void Change_arrival_rate::book_update_arrival_rates (Eventlist* eventlist, double time)
{
	eventlist->add_event(time,this);
}

bool Change_arrival_rate::execute()
{		
	for (TD_demand::iterator stop_iter = arrival_rates_TD.begin(); stop_iter != arrival_rates_TD.end(); stop_iter++)
	{
		map<Busline*,double> td_line = (*stop_iter).second;
		(*stop_iter).first->save_previous_arrival_rates ();
		for (map<Busline*,double>::iterator line_iter = td_line.begin(); line_iter != td_line.end(); line_iter++)
		{
			(*stop_iter).first->add_line_nr_boarding((*line_iter).first,(*line_iter).second);
		}
	}
	for (TD_demand::iterator stop_iter = alighting_fractions_TD.begin(); stop_iter != alighting_fractions_TD.end(); stop_iter++)
	{
		map<Busline*,double> td_line = (*stop_iter).second;
		(*stop_iter).first->save_previous_alighting_fractions ();
		for (map<Busline*,double>::iterator line_iter = td_line.begin(); line_iter != td_line.end(); line_iter++)
		{
			(*stop_iter).first->add_line_nr_alighting((*line_iter).first,(*line_iter).second);
		}
	}
	return true;
}
