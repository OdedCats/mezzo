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
	min_transfers = 100;
	active = false;
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

ODstops::~ODstops()
{
	delete random;
}

void ODstops::reset()
{
	min_transfers = 100;
	active = false;
	/*
	for (vector <Passenger*>::iterator iter_pass = waiting_passengers.begin(); iter_pass < waiting_passengers.end();)
	{
		pass_recycler.addPassenger(*iter_pass);
	}
	*/
	boarding_utility = 0;
	staying_utility = 0;
	waiting_passengers.clear();
	output_pass_decisions.clear();
}

bool ODstops::execute (Eventlist* eventlist, double curr_time) // generate passengers with this OD and books an event for next passenger generation
{
	if (active = true) // generate passenger from the second call, as first initialization call just set time to first passenger
	{	
		//Passenger* pass = pass_recycler.newPassenger();
		Passenger* pass = new Passenger;
		pid++; 
		pass->init (pid, curr_time, this);
		waiting_passengers.push_back (pass); // storage the new passenger at the list of waiting passengers with this OD
	}
	for (vector <Passenger*>::iterator wait_pass = waiting_passengers.begin(); wait_pass < waiting_passengers.end(); wait_pass++)
	{
		if ((*wait_pass)->get_OD_stop()->get_origin() != this->get_origin() || (*wait_pass)->get_OD_stop()->get_destination() != this->get_destination())
		{
			break;
		}
	}
	double headway_to_next_pass = random -> erandom (arrival_rate / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
	eventlist->add_event (curr_time + headway_to_next_pass, this);
	active = true;
	return true;
}

double ODstops::calc_boarding_probability (Busline* arriving_bus)
{
	// initialization
	boarding_utility = 0.0;
	staying_utility = 0.0;
	double accumlated_frequency_staying_alts = 0.0;
	vector<Busline*> first_leg_lines;
	bool in_alt = false; // indicates if the current arriving bus is included 
	
	// checks if the arriving bus is included as an option in the path set of this OD pair 
	for (vector <Pass_path*>::iterator path = path_set.begin(); path < path_set.end(); path ++)
	{
		if (in_alt == true)
		{
			break;
		}
		vector <vector <Busline*>> alt_lines = (*path)->get_alt_lines();
		vector <Busline*> first_lines = alt_lines.front(); // need to check only for the first leg
		for (vector <Busline*>::iterator line = first_lines.begin(); line < first_lines.end(); line++)
		{
			if ((*line)->get_id() == arriving_bus->get_id())
			{
				in_alt = true;
			}
		}
	}
	if (in_alt == true)
	{
		if (path_set.size() == 1) // if the choice-set includes only a single alternative of the arriving bus - then there is no choice left
		{
			boarding_utility = 10.0;
			staying_utility = -10.0;
			return 1;
		}
		for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
		{
			(*iter_paths)->set_arriving_bus_rellevant(false);
			first_leg_lines = (*iter_paths)->get_alt_lines().front();
			for(vector<Busline*>::iterator iter_first_leg_lines = first_leg_lines.begin(); iter_first_leg_lines < first_leg_lines.end(); iter_first_leg_lines++)
			{
				// has to be revised for non-direct paths, aggreagte over all options that include this bus
				if ((*iter_first_leg_lines)->get_id() == arriving_bus->get_id()) // if the arriving bus is a possible first leg for this path alternative
				{
					boarding_utility = (*iter_paths)->calc_arriving_utility(this); 
					(*iter_paths)->set_arriving_bus_rellevant(true);
					break;
				}
			}
			if ((*iter_paths)->get_arriving_bus_rellevant() == false) // if the arriving bus isn't a possible first leg for this path alternative
			{
				accumlated_frequency_staying_alts += (60 / (*iter_paths)->calc_curr_leg_headway(first_leg_lines)); 
				// aggregates the frequencies of all the options that does not include the arriving bus
			}
		}
		for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
		{
			if ((*iter_paths)->get_arriving_bus_rellevant() == false)
			{
				double relative_frequency = (60/(*iter_paths)->calc_curr_leg_headway(first_leg_lines)) / accumlated_frequency_staying_alts;
				// weighting factor for each path alternative that does not include the arriving bus
				staying_utility += relative_frequency * (*iter_paths)->calc_waiting_utility(this);
			}
		}
		return calc_binary_logit(boarding_utility, staying_utility); // calculate the probability to board
	}
	// what to do if the arriving bus is not included in any of the alternatives?
	// currently - will not board it
	else 
	{	
		boarding_utility = -10.0;
		staying_utility = 10.0;
		return 0;
	}
}

double ODstops::calc_binary_logit (double utility_i, double utility_j)
{
	return ((exp(utility_i)) / (exp(utility_i) + exp (utility_j)));
}

void ODstops::record_passenger_boarding_decision (Passenger* pass, Bustrip* trip, double time, bool boarding_decision)  // creates a log-file for stop-related info
{
	output_pass_decisions.push_back(Pass_boarding_decision(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), boarding_decision, boarding_utility, staying_utility)); 
}

void ODstops::write_output(ostream & out)
{
	for (list <Pass_boarding_decision>::iterator iter = output_pass_decisions.begin(); iter!=output_pass_decisions.end();iter++)
	{
		iter->write(out);
	}
}
