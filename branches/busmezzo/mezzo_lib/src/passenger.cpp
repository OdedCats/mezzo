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
	boarding_decision = random ->brandom(OD_stop->calc_boarding_probability(arriving_bus->get_line(), time));
	OD_stop->record_passenger_boarding_decision (this, arriving_bus, time, boarding_decision);
	return boarding_decision;
}

Busstop* Passenger::make_alighting_decision (Bustrip* boarding_bus, double time) // assuming that all passenger paths involve only direct trips
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
				vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin() + 1;
				for (vector<Busstop*>::iterator first_transfer_stops = (*stops_iter).begin(); first_transfer_stops < (*stops_iter).end(); first_transfer_stops++)
				{
					map <Busstop*, ODstops*> od_stops = (*first_transfer_stops)->get_stop_as_origin();
					ODstops* left_od_stop = od_stops[this->get_OD_stop()->get_destination()];
					
					if ((*first_transfer_stops)->get_id() == OD_stop->get_destination()->get_id())
					// in case it is the final destination for this passeneger
					{
						candidate_transfer_stops_u[(*first_transfer_stops)] = theParameters->in_vehicle_time_coefficient * (boarding_bus->get_line()->calc_curr_line_ivt(OD_stop->get_origin(),OD_stop->get_destination()))/60;
						// the only utility component is the IVT till the destination
					} 
					else
					// in case it is an intermediate transfer stop
					{
						candidate_transfer_stops_u[(*first_transfer_stops)] = left_od_stop->calc_combined_set_utility (this, boarding_bus, time);
						// the utility is combined for all paths from this transfer stop (incl. travel time till their and transfer penalty)
					}
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
		candidate_transfer_stops_p[(*transfer_stops).first] = exp(candidate_transfer_stops_u[(*transfer_stops).first]) / MNL_denominator;
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
			// constructing a structure for output
			map<Busstop*,pair<double,double>> alighting_MNL; // utility followed by probability per stop
			for (map <Busstop*, double>::iterator iter_u = candidate_transfer_stops_u.begin(); iter_u != candidate_transfer_stops_u.end(); iter_u++)
			{
				alighting_MNL[(*iter_u).first].first = (*iter_u).second;
			}
			for (map <Busstop*, double>::iterator iter_p = candidate_transfer_stops_p.begin(); iter_p != candidate_transfer_stops_p.end(); iter_p++)
			{
				alighting_MNL[(*iter_p).first].second = (*iter_p).second;
			}
			OD_stop->record_passenger_alighting_decision(this, boarding_bus, time, (*stops_probs).first, alighting_MNL);
			return ((*stops_probs).first); // rerurn the chosen stop by MNL choice model
		}
	}
	return candidate_transfer_stops_p.begin()->first; // arbitary choice in case something failed
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

