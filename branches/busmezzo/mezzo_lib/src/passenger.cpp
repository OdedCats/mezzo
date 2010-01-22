///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
	boarding_decision = false;
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

Passenger::~Passenger()
{
}


void Passenger::reset()
{
	boarding_decision = false;
	start_time = 0; 

}
void Passenger::init (int pass_id, double start_time_, ODstops* OD_stop_)
{
	passenger_id = pass_id;
	start_time = start_time_;
	original_origin = OD_stop_->get_origin();
	OD_stop = OD_stop_;
	boarding_decision = false;
}

bool Passenger:: make_boarding_decision (Bustrip* arriving_bus, double time) 
{
	boarding_decision = random ->brandom(OD_stop->calc_boarding_probability(arriving_bus->get_line()));
	OD_stop->record_passenger_boarding_decision (this, arriving_bus, time, boarding_decision);
	return boarding_decision;
}

Busstop* Passenger::make_alighting_decision (Bustrip* boarding_bus) // assuming that all passenger paths involve only direct trips
{
	// assuming that a pass. boards only paths from his path set
	map <Busstop*, double> candidate_transfer_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_transfer_stops_p; // the double value is the probability associated with the respective stop
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busline*>> alt_lines = (*path_iter)->get_alt_lines();
		vector<vector<Busline*>>::iterator first_leg = alt_lines.begin();
		for (vector <Busline*>::iterator first_leg_lines = (*first_leg).begin(); first_leg_lines < (*first_leg).end(); first_leg_lines++)
		{
			/*
			// currently alights at the first possible transfer stop
			if (boarding_bus->get_line()->get_id() == (*first_leg_lines)->get_id())
			{
				vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer__stops();
				vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin();
				stops_iter++;
				return (*stops_iter).front();
			}
			*/
			if (boarding_bus->get_line()->get_id() == (*first_leg_lines)->get_id())
			{
				vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer__stops();
				vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin();
				for (vector<Busstop*>::iterator first_transfer_stops = (*stops_iter).begin(); first_transfer_stops < (*stops_iter).end(); first_transfer_stops++)
				{
					map <Busstop*, ODstops*> od_stops = (*first_transfer_stops)->get_stop_as_origin();
					ODstops* left_od_stop = od_stops[this->get_OD_stop()->get_destination()];
					candidate_transfer_stops_u[(*first_transfer_stops)] = left_od_stop->calc_combined_set_utility (this);
					// note - this may be called several times, but result with the same calculation
				}
			}
		}
	}
	// calc MNL probabilities
	double MNL_denominator = 0.0;
	for (map <Busstop*, double>::iterator transfer_stops = candidate_transfer_stops_u.begin(); transfer_stops != candidate_transfer_stops_u.end(); transfer_stops++)
	{
		// calc denominator value
		MNL_denominator += exp((*transfer_stops).second);
	}
	for (map <Busstop*, double>::iterator transfer_stops = candidate_transfer_stops_u.begin(); transfer_stops != candidate_transfer_stops_u.end(); transfer_stops++)
	{
		candidate_transfer_stops_p[(*transfer_stops).first] = candidate_transfer_stops_u[(*transfer_stops).first] / MNL_denominator;
	}
	// perform choice
	vector<double> alighting_probs;
	for (map <Busstop*, double>::iterator stops_probs = candidate_transfer_stops_p.begin(); stops_probs != candidate_transfer_stops_p.end(); stops_probs++)
	{
		alighting_probs.push_back((*stops_probs).second);
	}
	int transfer_stop_position = random->mrandom(alighting_probs);
	int iter = 0;
	for (map <Busstop*, double>::iterator stops_probs = candidate_transfer_stops_p.begin(); stops_probs != candidate_transfer_stops_p.end(); stops_probs++)
	{
		iter++;
		if (iter == transfer_stop_position)
		{
			return ((*stops_probs).first);
		}
	}
	return candidate_transfer_stops_p.begin()->first;
}

 // PassengerRecycler procedures

PassengerRecycler::	~PassengerRecycler()
{
 	for (list <Passenger*>::iterator iter=pass_recycled.begin();iter!=pass_recycled.end();)
	{			
		delete (*iter); // calls automatically destructor
		iter=pass_recycled.erase(iter);	
	}
}

