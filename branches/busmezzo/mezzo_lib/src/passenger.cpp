///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
	boarding_decision = false;
	already_walked = false;
	end_time = 0;
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
	already_walked = false;
	start_time = 0; 
	end_time = 0;

}
void Passenger::init (int pass_id, double start_time_, ODstops* OD_stop_)
{
	passenger_id = pass_id;
	start_time = start_time_;
	end_time = 0;
	original_origin = OD_stop_->get_origin();
	OD_stop = OD_stop_;
	boarding_decision = false;
	already_walked = false;
}
bool Passenger::execute(Eventlist *eventlist, double time)
// called every time passengers choose to walk to another stop (origin/transfer)
// puts the passenger at the waiting list at the right timing
// passengers are already assigned with the connection stop they choose as their origin
{
	if (already_walked == false) // just book the event of arriving at the next stop (origin)
	{
		already_walked = true;
		eventlist->add_event(time, this);
	}
	else
	{
		if (OD_stop->get_origin()->get_id() == OD_stop->get_destination()->get_id()) // this may happend if the passenger walked to his final stop
		{
			end_time = time;
			pass_recycler.addPassenger(this); // terminate passenger
		}
		else // push passenger at the waiting list of his OD
		{
			OD_stop->add_pass_waiting (this);
		}
		already_walked = false;
	}
return true;
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
		if (alt_lines.empty() == false) // in case it is not a walking-only alternative
		{
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
					vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin() + 2; // pointing to the third place - the first transfer stop
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
							candidate_transfer_stops_u[(*first_transfer_stops)] = left_od_stop->calc_combined_set_utility_for_alighting (this, boarding_bus, time);
							// the utility is combined for all paths from this transfer stop (incl. travel time till there and transfer penalty)
						}
						// note - this may be called several times, but result with the same calculation
					}
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
			selected_path_stops.push_back((*stops_probs).first);
			return ((*stops_probs).first); // rerurn the chosen stop by MNL choice model
		}
	}
	selected_path_stops.push_back(candidate_transfer_stops_p.begin()->first);
	return candidate_transfer_stops_p.begin()->first; // arbitary choice in case something failed
}

Busstop* Passenger::make_connection_decision (double time)
{
	if (OD_stop->get_origin()->get_id() == OD_stop->get_destination()->get_id())
		// in case pass. alighted at this final destination stop
	{
		selected_path_stops.push_back(OD_stop->get_destination());
		return (OD_stop->get_destination());
	}
	map <Busstop*, double> candidate_connection_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_connection_stops_p; // the double value is the probability associated with the respective stop
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer__stops();
		vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin()+1;
		for (vector<Busstop*>::iterator connected_stop = (*stops_iter).begin(); connected_stop < (*stops_iter).end(); connected_stop++)
		// going over all the stops at the second (connected) set
		{
			if (candidate_connection_stops_u[(*connected_stop)] == NULL)
			{
				map <Busstop*, ODstops*> od_stops = (*connected_stop)->get_stop_as_origin();
				ODstops* left_od_stop = od_stops[this->get_OD_stop()->get_destination()];
				if ((*connected_stop)->get_id() == OD_stop->get_destination()->get_id())
				// in case it is the final destination for this passeneger
				{
					candidate_connection_stops_u[(*connected_stop)] = theParameters->walking_time_coefficient * (*path_iter)->get_walking_distances().front() / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
					// the only utility component is the CT (walking time) till the destination
				}	 
				else
				// in case it is an intermediate transfer stop
				{
					candidate_connection_stops_u[(*connected_stop)] = left_od_stop->calc_combined_set_utility_for_connection (this, (*path_iter)->get_walking_distances().front(), time);
					// the utility is combined for all paths from this transfer stop (incl. walking time to the connected stop)
				}
			}
		}
	}
	// calc MNL probabilities
	double MNL_denominator = 0.0;
	for (map <Busstop*, double>::iterator transfer_stops = candidate_connection_stops_u.begin(); transfer_stops != candidate_connection_stops_u.end(); transfer_stops++)
	{
		// calc denominator value
		MNL_denominator += exp((*transfer_stops).second);
	}
	for (map <Busstop*, double>::iterator transfer_stops = candidate_connection_stops_u.begin(); transfer_stops != candidate_connection_stops_u.end(); transfer_stops++)
	{
		candidate_connection_stops_p[(*transfer_stops).first] = exp(candidate_connection_stops_u[(*transfer_stops).first]) / MNL_denominator;
	}
	// perform choice
	vector<double> alighting_probs;
	for (map <Busstop*, double>::iterator stops_probs = candidate_connection_stops_p.begin(); stops_probs != candidate_connection_stops_p.end(); stops_probs++)
	{
		alighting_probs.push_back((*stops_probs).second);
	}
	int transfer_stop_position = random->mrandom(alighting_probs);
	int iter = 0;
	for (map <Busstop*, double>::iterator stops_probs = candidate_connection_stops_p.begin(); stops_probs != candidate_connection_stops_p.end(); stops_probs++)
	{
		iter++;
		if (iter == transfer_stop_position)
		{
			// constructing a structure for output
			map<Busstop*,pair<double,double>> alighting_MNL; // utility followed by probability per stop
			for (map <Busstop*, double>::iterator iter_u = candidate_connection_stops_u.begin(); iter_u != candidate_connection_stops_u.end(); iter_u++)
			{
				alighting_MNL[(*iter_u).first].first = (*iter_u).second;
			}
			for (map <Busstop*, double>::iterator iter_p = candidate_connection_stops_p.begin(); iter_p != candidate_connection_stops_p.end(); iter_p++)
			{
				alighting_MNL[(*iter_p).first].second = (*iter_p).second;
			}
			selected_path_stops.push_back((*stops_probs).first);
			return ((*stops_probs).first); // return the chosen stop by MNL choice model
		}
	}
	selected_path_stops.push_back(candidate_connection_stops_p.begin()->first);
	return candidate_connection_stops_p.begin()->first; // arbitary choice in case something failed
}

void Passenger::write_selected_path(ostream& out)
{
	out << passenger_id << '\t'<< original_origin->get_id() << '\t' << OD_stop->get_destination()->get_id() << '\t' << start_time << '\t' << end_time << '\t';
	for (vector <Busstop*>::iterator stop_iter = selected_path_stops.begin(); stop_iter < selected_path_stops.end(); stop_iter++)
	{
		out << (*stop_iter)->get_id() << '\t';
	}
	out << endl;
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

