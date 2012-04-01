///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
	boarding_decision = false;
	at_stop = true;
	changed_stop = false;
	end_time = 0;
	walking_time = 0;
	waiting_time = 0;
	in_vehicle_time = 0;
	nr_boardings = 0;
	elapsed_overpassed_time = 0;
	random = new (Random);
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
	has_network_rti = random->brandom(theParameters->ratio_network_information);
}

Passenger::~Passenger()
{
}


void Passenger::reset()
{
	boarding_decision = false;
	at_stop = true;
	changed_stop = false;
	start_time = 0; 
	end_time = 0;
	nr_boardings = 0;
	walking_time = 0;
	waiting_time = 0;
	in_vehicle_time = 0;
	elapsed_overpassed_time = 0;
	this_is_the_last_stop = false;
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
	has_network_rti = random->brandom(theParameters->ratio_network_information);
}
void Passenger::init (int pass_id, double start_time_, ODstops* OD_stop_)
{
	passenger_id = pass_id;
	start_time = start_time_;
	at_stop = true;
	changed_stop = false;
	end_time = 0;
	walking_time = 0;
	waiting_time = 0;
	in_vehicle_time = 0;
	nr_boardings = 0;
	original_origin = OD_stop_->get_origin();
	last_arrival_time_at_stop = start_time_;
	OD_stop = OD_stop_;
	boarding_decision = false;
	elapsed_overpassed_time = 0;
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
	has_network_rti = random->brandom(theParameters->ratio_network_information);
}

void Passenger::init_zone (int pass_id, double start_time_, ODzone* origin_, ODzone* destination_)
{
	passenger_id = pass_id;
	start_time = start_time_;
	end_time = 0;
	nr_boardings = 0;
	o_zone = origin_;
	d_zone = destination_;
	boarding_decision = false;
	this_is_the_last_stop = false;
}

bool Passenger::execute(Eventlist *eventlist, double time)
// called every time passengers choose to walk to another stop (origin/transfer)
// puts the passenger at the waiting list at the right timing
// passengers are already assigned with the connection stop they choose as their origin
{
	/*
	if (time == elapsed_overpassed_time) // call to check if elapsed waiting time is exhausted
	{
		// reconsider in case pass. is still waiting at the same stop
		if (at_stop == true && elapsed_waiting->get_id() == selected_path_stops.back()->get_id())
		{
			Busstop* connected_stop = make_connection_decision(time);
			if (connected_stop->get_id() != OD_stop->get_origin()->get_id())
			{
				selected_path_stops.push_back(connected_stop);
				selected_path_stops.push_back(connected_stop); // twice- to maintain path structure
				double extra_walking_time = OD_stop->get_origin()->get_walking_distance_stop(connected_stop) * 60 / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
				ODstops* new_od = connected_stop->get_stop_od_as_origin_per_stop(OD_stop->get_destination());
				set_ODstop(new_od); // set the connected stop as passenger's new origin (new OD)
				// booking an event to the arrival time at the new stop
				waiting_time += time - last_arrival_time_at_stop;
				walking_time += extra_walking_time*60;
				eventlist->add_event (time + extra_walking_time, this);	
				return true;
			}
			else // staying at the same stop - allow reconsideration
			{
				double time_for_overpassed_waiting;
				bool has_rti = false; 
				// check need to reconsideration due to RTI conflicting with expectations
				if (has_network_rti > 0 || OD_stop->get_origin()->get_rti() > 0) // has RTI?
				{
					has_rti = true;
				}
				if (has_rti == true)
				{
					time_for_overpassed_waiting = get_min_waiting_time_by_RTI(OD_stop->get_origin(), time);
				//}
				
				else
				{
					time_for_overpassed_waiting = get_min_waiting_time_by_headway(OD_stop->get_origin(), time);
				}
				
					elapsed_overpassed_time = time + time_for_overpassed_waiting + theParameters->expectations_diff;
					eventlist->add_event (elapsed_overpassed_time, this);	
					elapsed_waiting = OD_stop->get_origin();
					return true;
				}
			}
		}
		// the elapsed waiting time is not relevant
		else
		{
			return true;
		}
	}
	else
	{
	*/
		if (OD_stop->get_origin()->get_id() == OD_stop->get_destination()->get_id() || this_is_the_last_stop == true) 
		// this may happend if the passenger walked to his final stop or final destination (zonal)
		{
			end_time = time;
			pass_recycler.addPassenger(this); // terminate passenger
		}
		else // push passengers at the waiting list of their OD
		{
			double time_for_overpassed_waiting;
			bool has_rti = false; 
			// check need to reconsideration due to RTI conflicting with expectations
			if (has_network_rti > 0 || OD_stop->get_origin()->get_rti() > 0) // has RTI?
			{
				has_rti = true;
			}
			if (has_rti == true)
			{
				time_for_overpassed_waiting = get_min_waiting_time_by_RTI(OD_stop->get_origin(), time);
				if (changed_stop == false)
				{
			//	if (expected_waiting_times[OD_stop->get_origin()] + theParameters->expectations_diff < time_for_overpassed_waiting )
				//{
					Busstop* connected_stop = make_connection_decision(time);
					if (connected_stop->get_id() != OD_stop->get_origin()->get_id())
					{
						selected_path_stops.push_back(connected_stop);
						selected_path_stops.push_back(connected_stop); // twice- to maintain path structure
						changed_stop = true;
						//waiting_time += time - last_arrival_time_at_stop;
						double extra_walking_time = OD_stop->get_origin()->get_walking_distance_stop(connected_stop) * 60 / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
						ODstops* new_od = connected_stop->get_stop_od_as_origin_per_stop(OD_stop->get_destination());
						set_ODstop(new_od); // set the connected stop as passenger's new origin (new OD)
						// booking an event to the arrival time at the new stop
						walking_time += extra_walking_time;
						last_arrival_time_at_stop = time + extra_walking_time;
						eventlist->add_event (time + extra_walking_time, this);
						return true;
					}
				}
			//	}
			}
			/*
			else
			{
				time_for_overpassed_waiting = get_min_waiting_time_by_headway(OD_stop->get_origin(), time);
			}
			*/
			bool already_on_the_list = false;
			vector<Passenger*> waiting_pass = OD_stop->get_waiting_passengers();
			for (vector<Passenger*>::iterator pass_iter = waiting_pass.begin(); pass_iter < waiting_pass.end(); pass_iter++)
			{
				if ((*pass_iter)->get_id() == passenger_id)
				{
					already_on_the_list = true;
					break;
				}
			}
			if (already_on_the_list == false)
			{
				OD_stop->add_pass_waiting (this); // if you got here it means that the pass. didn't walk to another stop - add to list of waiting pass.
				//elapsed_overpassed_time = time + time_for_overpassed_waiting + theParameters->expectations_diff;
				//eventlist->add_event (elapsed_overpassed_time, this);	
				//elapsed_waiting = OD_stop->get_origin();
			}
		}
	//}
	return true;
}

double Passenger::get_min_waiting_time_by_RTI (Busstop* stop, double time)
{
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	bool first = true;
	double minimum_RTI;
	// find the minimum waiting time at this stop among all relevant lines
	for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
	{
		bool relevant_path = false;
		// check only for paths that do not involve walking to another stop
		vector<vector<Busstop*>> stops_set = (*iter_paths)->get_alt_transfer_stops();
		vector<vector<Busstop*>>::iterator stops_set_iter = stops_set.begin();
		stops_set_iter++;
		for (vector<Busstop*>::iterator stops_iter = (*stops_set_iter).begin(); stops_iter < (*stops_set_iter).end(); stops_iter++)
		{
			if ((*stops_iter)->get_id() == stop->get_id())
			{
				relevant_path = true;
			}
		}
		if (relevant_path == true)
		{
			// for walk only paths
			if ((*iter_paths)->get_alt_lines().empty() == true)
			{
				return 0.0;
			}
			vector<Busline*> leg_lines = (*iter_paths)->get_alt_lines().front(); // first leg lines
			for (vector<Busline*>::iterator iter_lines = leg_lines.begin(); iter_lines < leg_lines.end(); iter_lines++)
			{
				if (first == true) 
				{
					minimum_RTI = (*iter_lines)->time_till_next_arrival_at_stop_after_time(stop, time); 
					first = false;
				}
				else
				{
					minimum_RTI = min (minimum_RTI, (*iter_lines)->time_till_next_arrival_at_stop_after_time(stop, time));
				}
			}
		}
	}
	return max(0.0, minimum_RTI);
}

double Passenger::get_min_waiting_time_by_headway (Busstop* stop, double time)
{
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	bool first = true;
	double minimum_headway;
	// find the minimum waiting time at this stop among all relevant lines
	for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
	{
		bool relevant_path = false;
		// check only for lines going from this stop
		vector<vector<Busstop*>> stops_set = (*iter_paths)->get_alt_transfer_stops();
		vector<vector<Busstop*>>::iterator stops_set_iter = stops_set.begin();
		stops_set_iter++;
		for (vector<Busstop*>::iterator stops_iter = (*stops_set_iter).begin(); stops_iter < (*stops_set_iter).end(); stops_iter++)
		{
			if ((*stops_iter)->get_id() == stop->get_id())
			{
				relevant_path = true;
			}
		}
		if (relevant_path == true)
		{
			// for walk only paths
			if ((*iter_paths)->get_alt_lines().empty() == true)
			{
				return 0.0;
			}
			vector<Busline*> leg_lines = (*iter_paths)->get_alt_lines().front(); // first leg lines
			for (vector<Busline*>::iterator iter_lines = leg_lines.begin(); iter_lines < leg_lines.end(); iter_lines++)
			{
				if (first == true) // RTI for this line at this stop
				{
					minimum_headway = (*iter_lines)->calc_curr_line_headway_forward();
					first = false; 
				}
				else
				{
					minimum_headway = min (minimum_headway, (*iter_lines)->calc_curr_line_headway_forward());
				}
			}
		}
	}
	return minimum_headway;

}

bool Passenger:: make_boarding_decision (Bustrip* arriving_bus, double time) 
{
	Busstop* curr_stop = selected_path_stops.back();
	ODstops* od = curr_stop->get_stop_od_as_origin_per_stop(OD_stop->get_destination());
	double boarding_prob;
	switch (theParameters->demand_format)
	{
		case 3:
			// use the od based on last stop on record (in case of connections)
			boarding_prob = od->calc_boarding_probability(this, arriving_bus->get_line(), time, this->get_has_network_rti());
			boarding_decision = random ->brandom(boarding_prob);
			OD_stop->record_passenger_boarding_decision (this, arriving_bus, time, boarding_prob, boarding_decision);
			break;
		case 4:
			boarding_prob = calc_boarding_probability_zone(arriving_bus->get_line(), curr_stop, time);
			boarding_decision = random ->brandom(boarding_prob);
			o_zone->record_passenger_boarding_decision_zone(this, arriving_bus, time, boarding_prob, boarding_decision);
	}
	if (boarding_decision == true)
	{
		nr_boardings++;
	}
	return boarding_decision;
}

Busstop* Passenger::make_alighting_decision (Bustrip* boarding_bus, double time) // assuming that all passenger paths involve only direct trips
{
	// assuming that a pass. boards only paths from his path set
	map <Busstop*, double> candidate_transfer_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_transfer_stops_p; // the double value is the probability associated with the respective stop
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	int level_of_rti = OD_stop->get_origin()->get_rti();
	if (has_network_rti == 1)
	{
		level_of_rti = 3;
	}
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
					vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer_stops();
					vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin() + 2; // pointing to the third place - the first transfer stop
					if (path_set.size() == 1 && (*stops_iter).size() == 1)
					{
						candidate_transfer_stops_u[(*stops_iter).front()] = 10; // in case it is the only option
					}
					for (vector<Busstop*>::iterator first_transfer_stops = (*stops_iter).begin(); first_transfer_stops < (*stops_iter).end(); first_transfer_stops++)
					{
						ODstops* left_od_stop = (*first_transfer_stops)->get_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination());	
						if ((*first_transfer_stops)->get_id() == OD_stop->get_destination()->get_id())
						// in case it is the final destination for this passeneger
						{
							candidate_transfer_stops_u[(*first_transfer_stops)] = theParameters->in_vehicle_time_coefficient * ((boarding_bus->get_line()->calc_curr_line_ivt(OD_stop->get_origin(),OD_stop->get_destination(),level_of_rti))/60);
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
			return ((*stops_probs).first); // rerurn the chosen stop by MNL choice model
		}
	}
	return candidate_transfer_stops_p.begin()->first; // arbitary choice in case something failed
}

Busstop* Passenger::make_connection_decision (double time)
{
	map <Busstop*, double> candidate_connection_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_connection_stops_p; // the double value is the probability associated with the respective stop
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	if (path_set.empty() == true) // try - make result with funny things
	{
		map<Busstop*,double> & stops = selected_path_stops.back()->get_walking_distances();
		return (*stops.begin()).first;
	}
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer_stops();
		vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin()+1;
		for (vector<Busstop*>::iterator connected_stop = (*stops_iter).begin(); connected_stop < (*stops_iter).end(); connected_stop++)
		// going over all the stops at the second (connected) set
		{
			if (candidate_connection_stops_u[(*connected_stop)] == NULL && (*connected_stop)->check_destination_stop(this->get_OD_stop()->get_destination()) == true)
				// only if it wasn't done already and there exists an OD for the remaining part
			{
				ODstops* left_od_stop = (*connected_stop)->get_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination());
				if ((*connected_stop)->get_id() == OD_stop->get_destination()->get_id())
				// in case it is the final destination for this passeneger
				{
					candidate_connection_stops_u[(*connected_stop)] = theParameters->walking_time_coefficient * (*path_iter)->get_walking_distances().front() / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
					// the only utility component is the CT (walking time) till the destination
				}	 
				else
				// in case it is an intermediate transfer stop
				{
					candidate_connection_stops_u[(*connected_stop)] = left_od_stop->calc_combined_set_utility_for_connection (this, (*path_iter)->get_walking_distances().front(), time, has_network_rti);
					// the utility is combined for all paths from this transfer stop (incl. walking time to the connected stop)
				}
			}
		}
	}

	// record current passenger expectations regarding waiting times
	bool has_rti = false;
	if (has_network_rti > 0 && OD_stop->get_origin()->get_rti() > 0) // has RTI?
	{
		has_rti = true;
	}
	for (map <Busstop*, double>::iterator stop_iter = candidate_connection_stops_u.begin(); stop_iter!= candidate_connection_stops_u.end(); stop_iter++)
	{
		if (has_rti == true)
		{
			expected_waiting_times[(*stop_iter).first] = get_min_waiting_time_by_RTI((*stop_iter).first, time);
		}
		else
		{
			expected_waiting_times[(*stop_iter).first] = get_min_waiting_time_by_headway((*stop_iter).first, time);
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
	vector<double> connecting_probs;
	for (map <Busstop*, double>::iterator stops_probs = candidate_connection_stops_p.begin(); stops_probs != candidate_connection_stops_p.end(); stops_probs++)
	{
		connecting_probs.push_back((*stops_probs).second);
	}
	int transfer_stop_position = random->mrandom(connecting_probs);
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
			return ((*stops_probs).first); // return the chosen stop by MNL choice model
		}
	}
	return candidate_connection_stops_p.begin()->first; // arbitary choice in case something failed
}

Busstop* Passenger::make_first_stop_decision (double time)
{
	map <Busstop*, double> candidate_origin_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_origin_stops_p; // the double value is the probability associated with the respective stop	
	// going over all relevant origin and destination stops combinations
	if (origin_walking_distances.size() == 1) // in case there is only one possible origin stop
	{
		map<Busstop*,double>::iterator iter_stops = origin_walking_distances.begin();
		return (*iter_stops).first;
	}
	for (map<Busstop*,double>::iterator o_stop_iter = origin_walking_distances.begin(); o_stop_iter != origin_walking_distances.end(); o_stop_iter++)
	{
		if ((*o_stop_iter).second <= theParameters->max_walking_distance)
		{
			candidate_origin_stops_u[(*o_stop_iter).first] = 0;
			for (map<Busstop*,double>::iterator d_stop_iter = destination_walking_distances.begin(); d_stop_iter != destination_walking_distances.end(); d_stop_iter++)
			{
				ODstops* possible_od = (*o_stop_iter).first->get_stop_od_as_origin_per_stop((*d_stop_iter).first);
				vector<Pass_path*> path_set = possible_od->get_path_set();
				for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
				{
					vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer_stops();
					vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin();
					// taking into account the walking distances from the origin to the origin stop and from the last stop till the final destination
					candidate_origin_stops_u[(*o_stop_iter).first] += exp(theParameters->walking_time_coefficient * origin_walking_distances[(*o_stop_iter).first]/ random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4) + theParameters->walking_time_coefficient * destination_walking_distances[(*d_stop_iter).first]/ random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4) + possible_od->calc_combined_set_utility_for_connection (this,(*path_iter)->get_walking_distances().front(), time, has_network_rti));
				}
			}
			candidate_origin_stops_u[(*o_stop_iter).first] = log (candidate_origin_stops_u[(*o_stop_iter).first]);
		}
	}
	// calc MNL probabilities
	double MNL_denominator = 0.0;
	for (map <Busstop*, double>::iterator origin_stops = candidate_origin_stops_u.begin(); origin_stops != candidate_origin_stops_u.end(); origin_stops++)
	{
		// calc denominator value
		MNL_denominator += exp((*origin_stops).second);
	}
	for (map <Busstop*, double>::iterator origin_stops = candidate_origin_stops_u.begin(); origin_stops != candidate_origin_stops_u.end(); origin_stops++)
	{
		candidate_origin_stops_p[(*origin_stops).first] = exp(candidate_origin_stops_u[(*origin_stops).first]) / MNL_denominator;
	}
	// perform choice
	vector<double> origin_probs;
	for (map <Busstop*, double>::iterator stops_probs = candidate_origin_stops_p.begin(); stops_probs != candidate_origin_stops_p.end(); stops_probs++)
	{
		origin_probs.push_back((*stops_probs).second);
	}
	int origin_stop_position = random->mrandom(origin_probs);
	int iter = 0;
	for (map <Busstop*, double>::iterator stops_probs = candidate_origin_stops_p.begin(); stops_probs != candidate_origin_stops_p.end(); stops_probs++)
	{
		iter++;
		if (iter == origin_stop_position)
		{
			// constructing a structure for output
			map<Busstop*,pair<double,double>> origin_MNL; // utility followed by probability per stop
			for (map <Busstop*, double>::iterator iter_u = candidate_origin_stops_u.begin(); iter_u != candidate_origin_stops_u.end(); iter_u++)
			{
				origin_MNL[(*iter_u).first].first = (*iter_u).second;
			}
			for (map <Busstop*, double>::iterator iter_p = candidate_origin_stops_p.begin(); iter_p != candidate_origin_stops_p.end(); iter_p++)
			{
				origin_MNL[(*iter_p).first].second = (*iter_p).second;
			}
			return ((*stops_probs).first); // return the chosen stop by MNL choice model
		}
	}
	return candidate_origin_stops_p.begin()->first; // arbitary choice in case something failed

}

map<Busstop*,double> Passenger::sample_walking_distances (ODzone* zone)
{
	map<Busstop*,double> walking_distances;
	map <Busstop*,pair<double,double>> stop_distances = zone->get_stop_distances();
	for (map <Busstop*,pair<double,double>>::iterator stop_iter = stop_distances.begin(); stop_iter != stop_distances.end(); stop_iter++)
	{
			walking_distances[(*stop_iter).first] = random->nrandom((*stop_iter).second.first, (*stop_iter).second.second);
	}
	return walking_distances;
}

double Passenger::calc_boarding_probability_zone (Busline* arriving_bus, Busstop* o_stop, double time)
{
	// initialization
	double boarding_utility = 0.0;
	double staying_utility = 0.0;
	double path_utility = 0.0;
	vector<Busline*> first_leg_lines;
	bool in_alt = false; // indicates if the current arriving bus is included 
	// checks if the arriving bus is included as an option in a path set of at least ONE of relevant OD pair 
	map <Busstop*,pair<double,double>> d_stops = d_zone->get_stop_distances();
	for (map <Busstop*,pair<double,double>>::iterator iter_d_stops = d_stops.begin(); iter_d_stops != d_stops.end(); iter_d_stops++)
	{
		vector <Pass_path*> path_set = o_stop->get_stop_od_as_origin_per_stop((*iter_d_stops).first)->get_path_set();
		for (vector <Pass_path*>::iterator path = path_set.begin(); path < path_set.end(); path ++)
		{
			if (in_alt == true)
			{
				break;
			}
			if ((*path)->get_alt_lines().empty() == false) // in case it is not a walk-only alternative
			{
				vector <vector <Busline*>> alt_lines = (*path)->get_alt_lines();
				vector <Busline*> first_lines = alt_lines.front(); // need to check only for the first leg
				for (vector <Busline*>::iterator line = first_lines.begin(); line < first_lines.end(); line++)
				{
					if ((*line)->get_id() == arriving_bus->get_id())
					{
						in_alt = true;
						break;
					}
				}
			}
		}
	}
	// if this bus line can be rellevant
	if (in_alt == true)
	{
		vector<Pass_path*> arriving_paths;
		for (map <Busstop*,pair<double,double>>::iterator iter_d_stops = d_stops.begin(); iter_d_stops != d_stops.end(); iter_d_stops++)
		{
			vector <Pass_path*> path_set = o_stop->get_stop_od_as_origin_per_stop((*iter_d_stops).first)->get_path_set();
			for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
			{
				(*iter_paths)->set_arriving_bus_rellevant(false);
				if ((*iter_paths)->get_alt_lines().empty() == false) //  in case it is not a walking-only alternative
				{
					first_leg_lines = (*iter_paths)->get_alt_lines().front();
					for(vector<Busline*>::iterator iter_first_leg_lines = first_leg_lines.begin(); iter_first_leg_lines < first_leg_lines.end(); iter_first_leg_lines++)
					{
						if ((*iter_first_leg_lines)->get_id() == arriving_bus->get_id()) // if the arriving bus is a possible first leg for this path alternative
						{
							path_utility = (*iter_paths)->calc_arriving_utility(time, has_network_rti) + theParameters->waiting_time_coefficient * (destination_walking_distances[(*iter_d_stops).first] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
							// including the walking time from the last stop till the final destination
							boarding_utility += exp(path_utility); 
							arriving_paths.push_back((*iter_paths));
							(*iter_paths)->set_arriving_bus_rellevant(true);
							break;
						}
					}
				}
			}
		}
		boarding_utility = log (boarding_utility);
		for (map <Busstop*,pair<double,double>>::iterator iter_d_stops = d_stops.begin(); iter_d_stops != d_stops.end(); iter_d_stops++)
		{
			vector <Pass_path*> path_set = o_stop->get_stop_od_as_origin_per_stop((*iter_d_stops).first)->get_path_set();
			for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
			{
				if ((*iter_paths)->get_arriving_bus_rellevant() == false)
				{
					// logsum calculation
					path_utility = (*iter_paths)->calc_waiting_utility(OD_stop, this,(*iter_paths)->get_alt_transfer_stops().begin(), time, false, has_network_rti,false) + theParameters->walking_time_coefficient * (destination_walking_distances[(*iter_d_stops).first] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
					// including the walking time from the last stop till the final destination
					staying_utility += exp(path_utility);
				}
			}
		} // NOTE: NO dynamic dominance check yet in the zone case
		if (staying_utility == 0.0) 
		{
			boarding_utility = 2.0;
			staying_utility = -2.0;
		}
		else
		{
			staying_utility = log (staying_utility);
		}
		// calculate the probability to board
		return (exp(boarding_utility) / (exp(boarding_utility) + exp (staying_utility)));
		o_zone->set_boarding_u(boarding_utility);
		o_zone->set_staying_u(staying_utility);
	}
	// what to do if the arriving bus is not included in any of the alternatives?
	// currently - will not board it
	else 
	{	
		boarding_utility = -2.0;
		staying_utility = 2.0;
		return 0;
	}
}

Busstop* Passenger::make_alighting_decision_zone (Bustrip* boarding_bus, double time)
{
	// assuming that a pass. boards only paths from his path set
	map <Busstop*, double> candidate_transfer_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_transfer_stops_p; // the double value is the probability associated with the respective stop
	map <Busstop*,pair<double,double>> d_stops = d_zone->get_stop_distances();
	for (map <Busstop*,pair<double,double>>::iterator iter_d_stops = d_stops.begin(); iter_d_stops != d_stops.end(); iter_d_stops++)
	{
		vector<Pass_path*> path_set = OD_stop->get_origin()->get_stop_od_as_origin_per_stop((*iter_d_stops).first)->get_path_set();
		for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
		{
			vector<vector<Busline*>> alt_lines = (*path_iter)->get_alt_lines();
			if (alt_lines.empty() == false) // in case it is not a walking-only alternative
			{
				vector<vector<Busline*>>::iterator first_leg = alt_lines.begin();
				for (vector <Busline*>::iterator first_leg_lines = (*first_leg).begin(); first_leg_lines < (*first_leg).end(); first_leg_lines++)
				{
					if (boarding_bus->get_line()->get_id() == (*first_leg_lines)->get_id())
					{
						vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer_stops();
						vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin() + 2; // pointing to the third place - the first transfer stop
						for (vector<Busstop*>::iterator first_transfer_stops = (*stops_iter).begin(); first_transfer_stops < (*stops_iter).end(); first_transfer_stops++)
						{
							ODstops* left_od_stop = (*first_transfer_stops)->get_stop_od_as_origin_per_stop((*iter_d_stops).first);	
							candidate_transfer_stops_u[(*first_transfer_stops)] = left_od_stop->calc_combined_set_utility_for_alighting_zone (this, boarding_bus, time);	// the utility is combined for all paths from this transfer stop (incl. travel time till there and transfer penalty)
							// note - this may be called several times, but result with the same calculation
						}
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
			o_zone->record_passenger_alighting_decision_zone(this, boarding_bus, time, (*stops_probs).first, alighting_MNL);
			return ((*stops_probs).first); // rerurn the chosen stop by MNL choice model
		}
	}
	return candidate_transfer_stops_p.begin()->first; // arbitary choice in case something failed
}

Busstop* Passenger::make_connection_decision_zone (double time)
{
	// called with the new transfer stop as the origin stop	
	map <Busstop*, double> candidate_connection_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_connection_stops_p; // the double value is the probability associated with the respective stop
	double u_walk_directly = -10.0;
	if (destination_walking_distances.count(OD_stop->get_origin()) > 0) // in case this stop belongs to the destination zone
	{
		if (destination_walking_distances[OD_stop->get_origin()] < theParameters->max_walking_distance)
		{
			u_walk_directly = theParameters->walking_time_coefficient * destination_walking_distances[OD_stop->get_origin()] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
		}
	}
	map <Busstop*,pair<double,double>> distances = d_zone->get_stop_distances();
	for (map<Busstop*,pair<double,double>>::iterator iter_d_stops = distances.begin(); iter_d_stops != distances.end(); iter_d_stops++)
	{	
		vector<Pass_path*> path_set = OD_stop->get_origin()->get_stop_od_as_origin_per_stop((*iter_d_stops).first)->get_path_set();
		for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
		{
			vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer_stops();
			vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin()+1;
			for (vector<Busstop*>::iterator connected_stop = (*stops_iter).begin(); connected_stop < (*stops_iter).end(); connected_stop++)
			// going over all the stops at the second (connected) set
			{
				if (candidate_connection_stops_u[(*connected_stop)] == NULL && (*connected_stop)->check_destination_stop(this->get_OD_stop()->get_destination()) == true)
				// only if it wasn't done already and there exists an OD for the remaining part
				{
					ODstops* left_od_stop = (*connected_stop)->get_stop_od_as_origin_per_stop((*iter_d_stops).first);
					// at every stop you can try to walk from it till the final destination or continue by transit
					candidate_connection_stops_u[(*connected_stop)] = left_od_stop->calc_combined_set_utility_for_connection_zone(this,(*path_iter)->get_walking_distances().front(), time);
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
	double u_continue_transit_trip = log(MNL_denominator);
	for (map <Busstop*, double>::iterator transfer_stops = candidate_connection_stops_u.begin(); transfer_stops != candidate_connection_stops_u.end(); transfer_stops++)
	{
		candidate_connection_stops_p[(*transfer_stops).first] = exp(candidate_connection_stops_u[(*transfer_stops).first]) / MNL_denominator;
	}
	// perform choice

	// first - decide whether to walk directly to the destination or continue the trnasit trip
	this_is_the_last_stop = random ->brandom(OD_stop->calc_binary_logit(u_walk_directly,u_continue_transit_trip)); 
	if (this_is_the_last_stop == true)
	{
		return OD_stop->get_origin();
	}
	// second - if we continue by transit, than at which stop to transfer
	vector<double> connecting_probs;
	for (map <Busstop*, double>::iterator stops_probs = candidate_connection_stops_p.begin(); stops_probs != candidate_connection_stops_p.end(); stops_probs++)
	{
		connecting_probs.push_back((*stops_probs).second);
	}
	int transfer_stop_position = random->mrandom(connecting_probs);
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
			return ((*stops_probs).first); // return the chosen stop by MNL choice model
		}
	}
	return candidate_connection_stops_p.begin()->first; // arbitary choice in case something failed
}

bool Passenger::stop_is_in_d_zone (Busstop* stop)
{
	if (destination_walking_distances.count(stop)>0)
	{
		return true;
	}
	else
	{
		return false;
	}
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

