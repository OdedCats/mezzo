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
	double boarding_utility;
	double staying_utility = 0.0;
	double accumlated_frequency_staying_alts = 0.0;
	vector<Busline*> first_leg_lines;
	bool in_alt = false; // indicates if the current arriving bus is included 
	for (vector <Pass_path*>::iterator path = path_set.begin(); path < path_set.end(); path ++)
	{
		if (in_alt == true)
		{
			break;
		}
		vector <vector <Busline*>> alt_lines = (*path)->get_alt_lines();
		vector <Busline*> first_lines = alt_lines.front();
		for (vector <Busline*>::iterator line = first_lines.begin(); line < first_lines.end(); line++)
		{
			if ((*line)->get_id() == arriving_bus->get_id())
			{
				in_alt = true;
			}
		}
	}
	for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
	{
		(*iter_paths)->set_arriving_bus_rellevant(false);
		first_leg_lines = (*iter_paths)->get_alt_lines().front();
		for(vector<Busline*>::iterator iter_first_leg_lines = first_leg_lines.begin(); iter_first_leg_lines < first_leg_lines.end(); iter_first_leg_lines++)
		{
			if ((*iter_first_leg_lines)->get_id() == arriving_bus->get_id())
			{
				 boarding_utility = (*iter_paths)->calc_arriving_utility(this);
				 (*iter_paths)->set_arriving_bus_rellevant(true);
				 break;
			}
		}
		if ((*iter_paths)->get_arriving_bus_rellevant() == false)
		{
			accumlated_frequency_staying_alts = (60 / (*iter_paths)->calc_curr_leg_headway(first_leg_lines));
		}
	}
	for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
	{
		if ((*iter_paths)->get_arriving_bus_rellevant() == false)
		{
			double relative_frequency = (60/(*iter_paths)->calc_curr_leg_headway(first_leg_lines)) / (60/accumlated_frequency_staying_alts);
			staying_utility += relative_frequency * (*iter_paths)->calc_waiting_utility(this);
		}
	}
	if (in_alt == true)
	{
		return calc_binary_logit(boarding_utility, staying_utility);
	}
	else return 0.5;
}

double ODstops::calc_binary_logit (double utility_i, double utility_j)
{
	return ((exp(utility_i)) / exp(utility_i + utility_j));
}