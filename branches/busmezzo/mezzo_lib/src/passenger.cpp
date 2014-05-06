///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
	boarding_decision = false;
	already_walked = false;
	end_time = 0;
	nr_boardings = 0;
	AWT_first_leg_boarding = 0;
	random = new (Random);
	if (randseed != 0)
	{
		random->seed(randseed);
	}
	else
	{
		random->randomize();
	}
	memory_projected_RTI.clear();
	arrival_time_at_stop = 0;
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
	nr_boardings = 0;
	AWT_first_leg_boarding = 0;
	this_is_the_last_stop = false;
	memory_projected_RTI.clear();
	arrival_time_at_stop = 0;
}
void Passenger::init (int pass_id, double start_time_, ODstops* OD_stop_)
{
	passenger_id = pass_id;
	start_time = start_time_;
	end_time = 0;
	nr_boardings = 0;
	original_origin = OD_stop_->get_origin();
	OD_stop = OD_stop_;
	AWT_first_leg_boarding = 0;
	boarding_decision = false;
	already_walked = false;
	RTI_network_level = random->brandom(theParameters->share_RTI_network);
	if (theParameters->pass_day_to_day_indicator == true)
	{
		for (map<pair<Busstop*, Busline*>,double>::iterator stopline_iter = OD_stop_->get_anticipated_waiting_time().begin(); stopline_iter != OD_stop_->get_anticipated_waiting_time().end(); stopline_iter++)
		{
			pair<Busstop*, Busline*> stopline = (*stopline_iter).first;
			anticipated_waiting_time[stopline] = (*stopline_iter).second;
		}
		for (map<pair<Busstop*, Busline*>,double>::iterator stopline_iter = OD_stop_->get_alpha_RTI().begin(); stopline_iter != OD_stop_->get_alpha_RTI().end(); stopline_iter++)
		{
			pair<Busstop*, Busline*> stopline = (*stopline_iter).first;
			alpha_RTI[stopline] = (*stopline_iter).second;
		}
		for (map<pair<Busstop*, Busline*>,double>::iterator stopline_iter = OD_stop_->get_alpha_exp().begin(); stopline_iter != OD_stop_->get_alpha_exp().end(); stopline_iter++)
		{
			pair<Busstop*, Busline*> stopline = (*stopline_iter).first;
			alpha_exp[stopline] = (*stopline_iter).second;
		}
	}
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
	already_walked = false;
	this_is_the_last_stop = false;
}

void Passenger::set_memory_projected_RTI (Busstop* stop, Busline* line, double projected_RTI)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	if (memory_projected_RTI.count(stopline) == 0) // don't override previously projected values
	{
		memory_projected_RTI[stopline] = projected_RTI;
	}
}

void Passenger::set_AWT_first_leg_boarding(Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;

	// So should this be done for every line that potentially pass at this stop upon arrival at the stop

	// consider with and without RTI
	// need to calc projected RTI and PK
	
	//AWT_first_leg_boarding = alpha_exp[stopline] * get_anticipated_waiting_time(stop,line) + alpha_RTI[stopline] * wt_rti + (1-alpha_RTI[stopline]-alpha_exp[stopline])*wt_pk; 	
}

double Passenger::get_memory_projected_RTI (Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	if (memory_projected_RTI.count(stopline) == 0)
	{
		return 0;
	}
	return memory_projected_RTI[stopline];
}

double Passenger::get_anticipated_waiting_time (Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	return anticipated_waiting_time[stopline];
}

double Passenger::get_alpha_RTI (Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	return alpha_RTI[stopline];
}

double Passenger::get_alpha_exp (Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	return alpha_exp[stopline];
}

bool Passenger::any_previous_exp_ODSL (Busstop* stop, Busline* line)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	return alpha_exp.count(stopline);
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
		if (this_is_the_last_stop == true ||  this->get_OD_stop()->check_path_set() == false || OD_stop->get_origin()->get_id() == OD_stop->get_destination()->get_id()) 
		// this may happend if the passenger walked to his final stop or final destination (zonal)
		{
			end_time = time;
			pass_recycler.addPassenger(this); // terminate passenger
		}
		else // push passengers at the waiting list of their OD
		{
			arrival_time_at_stop = time;
			OD_stop->add_pass_waiting (this);
			if (RTI_network_level == true || OD_stop->get_origin()->get_rti() > 0)
			{
				vector<Busline*> lines_at_stop = OD_stop->get_origin()->get_lines();
				for (vector <Busline*>::iterator line_iter = lines_at_stop.begin(); line_iter < lines_at_stop.end(); line_iter++)
				{
					pair<Busstop*, Busline*> stopline;
					stopline.first = OD_stop->get_origin();
					stopline.second = (*line_iter);
					if (memory_projected_RTI.count(stopline) == 0)
					{
						//this->set_memory_projected_RTI(OD_stop->get_origin(),(*line_iter),(*line_iter)->time_till_next_arrival_at_stop_after_time(OD_stop->get_origin(),time));
						//this->set_AWT_first_leg_boarding();
					}
				}
			}
		}
		already_walked = false;
	}
return true;
}

bool Passenger:: make_boarding_decision (Bustrip* arriving_bus, double time) 
{
	/*Busstop* curr_stop = selected_path_stops.back().first;
	ODstops* od = curr_stop->get_stop_od_as_origin_per_stop(OD_stop->get_destination());*/
	Busstop* curr_stop = OD_stop->get_origin(); //2014-04-14 Jens West changed this, because otherwise the passengers would board lines and then not know what to do
	ODstops* od = OD_stop;
	double boarding_prob;
	switch (theParameters->demand_format)
	{
		case 3:
			// use the od based on last stop on record (in case of connections)
			boarding_prob = od->calc_boarding_probability(arriving_bus->get_line(), time, this);
			boarding_decision = random ->brandom(boarding_prob);
			//OD_stop->record_passenger_boarding_decision (this, arriving_bus, time, boarding_prob, boarding_decision);
			if (boarding_decision == 1)
			{
				int level_of_rti_upon_decision = curr_stop->get_rti();
				if (RTI_network_level == 1)
				{
					level_of_rti_upon_decision = 3;
				}
				OD_stop->record_waiting_experience(this, arriving_bus, time, level_of_rti_upon_decision,this->get_memory_projected_RTI(curr_stop,arriving_bus->get_line()),AWT_first_leg_boarding);
			}
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
						if ((*first_transfer_stops)->get_id() == OD_stop->get_destination()->get_id())
						// in case it is the final destination for this passeneger
						{
							candidate_transfer_stops_u[(*first_transfer_stops)] = theParameters->in_vehicle_time_coefficient * ((boarding_bus->get_line()->calc_curr_line_ivt(OD_stop->get_origin(),OD_stop->get_destination(),OD_stop->get_origin()->get_rti(), time))/60);
							// the only utility component is the IVT till the destination
						} 
						else
						// in case it is an intermediate transfer stop
						{
							ODstops* left_od_stop;
							if ((*first_transfer_stops)->check_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination()) == false)
							{
								left_od_stop = new ODstops ((*first_transfer_stops),this->get_OD_stop()->get_destination());
								(*first_transfer_stops)->add_odstops_as_origin(this->get_OD_stop()->get_destination(), left_od_stop);
								this->get_OD_stop()->get_destination()->add_odstops_as_destination((*first_transfer_stops), left_od_stop);
							}
							else
							{
								left_od_stop = (*first_transfer_stops)->get_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination());	
							}
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
			//OD_stop->record_passenger_alighting_decision(this, boarding_bus, time, (*stops_probs).first, alighting_MNL);
			return ((*stops_probs).first); // rerurn the chosen stop by MNL choice model
		}
	}
	return candidate_transfer_stops_p.begin()->first; // arbitary choice in case something failed
}

Busstop* Passenger::make_connection_decision (double time)
{
	map <Busstop*, double> candidate_connection_stops_u; // the double value is the utility associated with the respective stop
	map <Busstop*, double> candidate_connection_stops_p; // the double value is the probability associated with the respective stop
	Busstop* bs_o = OD_stop->get_origin();
	Busstop* bs_d = OD_stop->get_destination();
	vector<Pass_path*> path_set = bs_o->get_stop_od_as_origin_per_stop(bs_d)->get_path_set();
	//	OD_stop->get_path_set();
	if (path_set.empty() == true) // move to a nearby stop in case needed
	{
		map<Busstop*,double> & stops = OD_stop->get_origin()->get_walking_distances();
		if (stops.begin()->first->get_id() == OD_stop->get_origin()->get_id())
		{
			map<Busstop*,double>::iterator stops_iter = stops.begin();
			stops_iter++;
			if (stops_iter == stops.end())
			{
				return stops.begin()->first;
			}
			return stops_iter->first;
		}
		else
		{
			return stops.begin()->first;
		}
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
				ODstops* left_od_stop;
				if ((*connected_stop)->check_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination()) == false)
				{
					ODstops* left_od_stop = new ODstops ((*connected_stop),this->get_OD_stop()->get_destination());
					(*connected_stop)->add_odstops_as_origin(this->get_OD_stop()->get_destination(), left_od_stop);
					this->get_OD_stop()->get_destination()->add_odstops_as_destination((*connected_stop), left_od_stop);
				}
				else
				{
					left_od_stop = (*connected_stop)->get_stop_od_as_origin_per_stop(this->get_OD_stop()->get_destination());
				}
				if ((*connected_stop)->get_id() == OD_stop->get_destination()->get_id())
				// in case it is the final destination for this passeneger
				{
					candidate_connection_stops_u[(*connected_stop)] = theParameters->walking_time_coefficient * (*path_iter)->get_walking_distances().front() / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4);
					// the only utility component is the CT (walking time) till the destination
				}	 
				else
				// in case it is an intermediate transfer stop
				{
					candidate_connection_stops_u[(*connected_stop)] = left_od_stop->calc_combined_set_utility_for_connection ((*path_iter)->get_walking_distances().front(), time, this);
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
	already_walked = true;
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
					candidate_origin_stops_u[(*o_stop_iter).first] += exp(theParameters->walking_time_coefficient * origin_walking_distances[(*o_stop_iter).first]/ random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4) + theParameters->walking_time_coefficient * destination_walking_distances[(*d_stop_iter).first]/ random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4) + possible_od->calc_combined_set_utility_for_connection ((*path_iter)->get_walking_distances().front(), time, this));
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
						path_utility = (*iter_paths)->calc_arriving_utility(time, this) + theParameters->waiting_time_coefficient * (destination_walking_distances[(*iter_d_stops).first] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
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
				path_utility = (*iter_paths)->calc_waiting_utility((*iter_paths)->get_alt_transfer_stops().begin(), time, false, this) + theParameters->walking_time_coefficient * (destination_walking_distances[(*iter_d_stops).first] / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
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
		pair<Busstop*,double> stop_time;
		stop_time.first = OD_stop->get_origin();
		stop_time.second = time;
		selected_path_stops.push_back(stop_time);
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
			pair<Busstop*,double> stop_time;
			stop_time.first = (*stops_probs).first;
			stop_time.second = time;
			selected_path_stops.push_back(stop_time);
			return ((*stops_probs).first); // return the chosen stop by MNL choice model
		}
	}
	pair<Busstop*,double> stop_time;
	stop_time.first = candidate_connection_stops_p.begin()->first;
	stop_time.second = time;
	selected_path_stops.push_back(stop_time);
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

double Passenger::calc_total_waiting_time()
{
	double total_waiting_time = 0.0;
	vector <pair<Busstop*,double>>::iterator iter_stop=selected_path_stops.begin();
	iter_stop++;
	for (vector <pair<Bustrip*,double>>::iterator iter_trip=selected_path_trips.begin(); iter_trip<selected_path_trips.end(); iter_trip++)
	{
		total_waiting_time += (*iter_trip).second - (*iter_stop).second; 
		iter_stop++;
		iter_stop++;
	}
	return total_waiting_time;
}

double Passenger::calc_total_IVT()
{
	/*
	double IVT = 0.0;
	for (vector <pair<double,double>>::iterator iter = experienced_crowding_levels.begin(); iter < experienced_crowding_levels.end(); iter++)
	{
		IVT += (*iter).first;
	}
	return IVT;
	*/
	double total_IVT;
	total_IVT = 0.0;
	vector <pair<Busstop*,double>>::iterator iter_stop=selected_path_stops.begin();
	iter_stop++;
	iter_stop++;
	for (vector <pair<Bustrip*,double>>::iterator iter_trip=selected_path_trips.begin(); iter_trip<selected_path_trips.end(); iter_trip++)
	{
		total_IVT += (*iter_stop).second - (*iter_trip).second; 
		iter_stop++;
		iter_stop++;
	}
	return total_IVT;
}

double Passenger::calc_IVT_crowding()
{
	double VoT_crowding = 0.0;
	for (vector <pair<double,double>>::iterator iter = experienced_crowding_levels.begin(); iter < experienced_crowding_levels.end(); iter++)
	{
		VoT_crowding += (*iter).first * (*iter).second;
	}
	return VoT_crowding;
}

double Passenger::calc_total_walking_time()
{
	double total_walking_time = 0.0;
	for (vector <pair<Busstop*,double>>::iterator iter_stop=selected_path_stops.begin(); iter_stop<selected_path_stops.end(); iter_stop++)
	{
		iter_stop++;
		total_walking_time += (*iter_stop).second - (*(iter_stop-1)).second; 
	}
	return total_walking_time;
}

double Passenger::calc_total_waiting_time_due_to_denied_boarding()
{
	double total_walking_time_due_to_denied_boarding = 0.0;
	for (vector <pair<Busstop*,double>>::iterator iter_denied=waiting_time_due_denied_boarding.begin(); iter_denied < waiting_time_due_denied_boarding.end(); iter_denied++)
	{
		vector <pair<Busstop*,double>>::iterator iter_stop=selected_path_stops.begin();
		iter_stop++;
		for (vector <pair<Bustrip*,double>>::iterator iter_trip=selected_path_trips.begin(); iter_trip<selected_path_trips.end(); iter_trip++)
		{
			if ((*iter_stop).first->get_id() == (*iter_denied).first->get_id())
			{
				total_walking_time_due_to_denied_boarding += (*iter_trip).second - (*iter_denied).second; 
				break;
			}
			iter_stop++;
			iter_stop++;
		}
		
	}
	return total_walking_time_due_to_denied_boarding;
}

void Passenger::write_selected_path(ostream& out)
{
	// claculate passenger travel time components
	if (end_time > 0)
	{
	
	double total_waiting_time = calc_total_waiting_time();
	double total_IVT = calc_total_IVT();
	double total_IVT_crowding = calc_IVT_crowding();
	double total_walking_time = calc_total_walking_time();
	double total_waiting_time_due_to_denied_boarding = calc_total_waiting_time_due_to_denied_boarding();
	
	out << passenger_id << '\t'<< original_origin->get_id() << '\t' << OD_stop->get_destination()->get_id() << '\t' << start_time << '\t' << total_walking_time << '\t' << total_waiting_time << '\t' << total_waiting_time_due_to_denied_boarding << '\t' << total_IVT << '\t' << total_IVT_crowding << '\t' << end_time << '\t' << '{';
	for (vector <pair<Busstop*,double>>::iterator stop_iter = selected_path_stops.begin(); stop_iter < selected_path_stops.end(); stop_iter++)
	{
		out << (*stop_iter).first->get_id() << '\t';
	}
	out << '}' << '\t' << '{' << '\t';
	for (vector <pair<Bustrip*,double>>::iterator trip_iter = selected_path_trips.begin(); trip_iter < selected_path_trips.end(); trip_iter++)
	{
		out << (*trip_iter).first->get_id() << '\t';
	}	
	out << '}' << endl;

	}
}

int Passenger::get_selected_path_last_line_id ()
{
	return selected_path_trips.back().first->get_line()->get_id();
}

int Passenger::get_last_denied_boarding_stop_id ()
{
	Busstop* stop = waiting_time_due_denied_boarding.back().first;
	return stop->get_id();
}

bool Passenger::empty_denied_boarding ()
{
	return waiting_time_due_denied_boarding.empty();
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

