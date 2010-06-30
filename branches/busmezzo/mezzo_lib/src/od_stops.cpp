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
	output_pass_boarding_decision.clear();
	output_pass_alighting_decision.clear();
	passengers_during_simulation.clear();
}

bool ODstops::execute (Eventlist* eventlist, double curr_time) // generate passengers with this OD and books an event for next passenger generation
{
	if (curr_time < theParameters->start_pass_generation)
	{
		eventlist->add_event (theParameters->start_pass_generation, this);
		return true;
	}
	if (curr_time >= theParameters->stop_pass_generation)
	{
		return true;
	}
// called only for generting pass.
	if (active = true) // generate passenger from the second call, as first initialization call just set time to first passenger
	{	
		//Passenger* pass = pass_recycler.newPassenger();
		Passenger* pass = new Passenger;
		passengers_during_simulation.push_back(pass);
		pid++; 
		pass->init (pid, curr_time, this);
		pass->add_to_selected_path_stop(origin_stop);
		Busstop* connection_stop = pass->make_connection_decision(curr_time);
		if (connection_stop->get_id() != origin_stop->get_id()) // if the pass. walks to another stop
		{
			// set connected_stop as the new origin
			pass->set_ODstop(connection_stop->get_stop_od_as_origin_per_stop(destination_stop)); // set this stop as his new origin (new OD)
			map<Busstop*,double> walk_dis = origin_stop->get_walking_distances();
			pass->execute(eventlist, curr_time + walk_dis[connection_stop] / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4));
		}
		else // if the pass. stays at the same stop
		{
			waiting_passengers.push_back (pass); // storage the new passenger at the list of waiting passengers with this OD
		}
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

double ODstops::calc_boarding_probability (Busline* arriving_bus, double time)
{
	// initialization
	boarding_utility = 0.0;
	staying_utility = 0.0;
	vector<Busline*> first_leg_lines;
	bool in_alt = false; // indicates if the current arriving bus is included 
	
	// checks if the arriving bus is included as an option in the path set of this OD pair 
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
			if ((*iter_paths)->get_alt_lines().empty() == false) //  in case it is not a walking-only alternative
			{
				first_leg_lines = (*iter_paths)->get_alt_lines().front();
				for(vector<Busline*>::iterator iter_first_leg_lines = first_leg_lines.begin(); iter_first_leg_lines < first_leg_lines.end(); iter_first_leg_lines++)
				{
					if ((*iter_first_leg_lines)->get_id() == arriving_bus->get_id()) // if the arriving bus is a possible first leg for this path alternative
					{
						boarding_utility += exp((*iter_paths)->calc_arriving_utility(time)); 
						(*iter_paths)->set_arriving_bus_rellevant(true);
						break;
					}
				}
			}
		}
		boarding_utility = log (boarding_utility);
		for (vector<Pass_path*>::iterator iter_paths = path_set.begin(); iter_paths < path_set.end(); iter_paths++)
		{
			if ((*iter_paths)->get_arriving_bus_rellevant() == false)
			{
				// logsum calculation
				staying_utility += exp((*iter_paths)->calc_waiting_utility((*iter_paths)->get_alt_transfer__stops().begin(), time, false));
			}
		}
		staying_utility = log (staying_utility);
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

double ODstops::calc_combined_set_utility_for_alighting (Passenger* pass, Bustrip* bus_on_board, double time)
{
	// calc logsum over all the paths from this origin stop
	staying_utility = 0.0;
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		double time_till_transfer = bus_on_board->get_line()->calc_curr_line_ivt(pass->get_OD_stop()->get_origin(),origin_stop,pass->get_OD_stop()->get_origin()->get_rti()); // in seconds
		staying_utility += exp(random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * (time_till_transfer/60) + random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4 )  +  (*paths)->calc_waiting_utility((*paths)->get_alt_transfer__stops().begin(), time + time_till_transfer, true));
		// taking into account IVT till this intermediate stop, transfer penalty and the utility of the path from this transfer stop till the final destination
	}
	return log(staying_utility);
}

double ODstops::calc_combined_set_utility_for_connection (double walking_distance, double time)
{
	// calc logsum over all the paths from this origin stop
	double connection_utility = 0.0;
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		bool without_walking_first = false;
		// go only through paths that does not include walking to another stop from this connection stop
		vector<vector<Busstop*>> alt_stops = (*paths)->get_alt_transfer__stops();
		vector<vector<Busstop*>>::iterator alt_stops_iter = alt_stops.begin();
		alt_stops_iter++;
		// check if the first (connected) stop is also included in the second element (no further walking)
		for (vector<Busstop*>::iterator stop_iter = (*alt_stops_iter).begin(); stop_iter < (*alt_stops_iter).end(); stop_iter++)
		{
			if ((*stop_iter)->get_id() == (origin_stop->get_id()))
			{
				without_walking_first = true;
			}
		}
		if (without_walking_first == true) // considering only no multi-walking alternatives
		{
			double time_till_connected_stop = walking_distance / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed / 4); // in minutes
			connection_utility += exp(random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * time_till_connected_stop + (*paths)->calc_waiting_utility(alt_stops_iter, time + (time_till_connected_stop * 60), false));
			// taking into account CT (walking time) till this connected stop and the utility of the path from this connected stop till the final destination
		}
	}
	return log(connection_utility);
}

void ODstops::record_passenger_boarding_decision (Passenger* pass, Bustrip* trip, double time, double boarding_probability, bool boarding_decision)  // add to output structure boarding decision info
{
	output_pass_boarding_decision[pass].push_back(Pass_boarding_decision(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), boarding_probability , boarding_decision, boarding_utility, staying_utility)); 
}

void ODstops::record_passenger_alighting_decision (Passenger* pass, Bustrip* trip, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> alighting_MNL)  //  add to output structure alighting decision info
{
	output_pass_alighting_decision[pass].push_back(Pass_alighting_decision(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), chosen_alighting_stop->get_id(), alighting_MNL)); 
}

void ODstops::write_boarding_output(ostream & out, Passenger* pass)
{
	for (list <Pass_boarding_decision>::iterator iter = output_pass_boarding_decision[pass].begin(); iter!=output_pass_boarding_decision[pass].end();iter++)
	{
		iter->write(out);
	}
}

void ODstops::write_alighting_output(ostream & out, Passenger* pass)
{
	for (list <Pass_alighting_decision>::iterator iter = output_pass_alighting_decision[pass].begin(); iter!=output_pass_alighting_decision[pass].end();iter++)
	{
		iter->write(out);
	}
}

void ODstops::write_od_summary(ostream & out)
{
	calc_pass_measures();
	out << origin_stop->get_id() << '\t' << destination_stop->get_id() << '\t' << nr_pass_completed << '\t' << avg_tt << '\t' << avg_nr_boardings << '\t' << endl; 
	for (vector <pair<vector<Busstop*>, pair <int,double>>>::iterator path_iter = paths_tt.begin(); path_iter < paths_tt.end(); path_iter++)
	{
		for (vector<Busstop*>::iterator stop_iter = (*path_iter).first.begin(); stop_iter < (*path_iter).first.end(); stop_iter++)
		{
			out << (*stop_iter)->get_id() << '\t';
		}
		out << (*path_iter).second.first << '\t' << (*path_iter).second.second << endl;
	}
}

void ODstops::calc_pass_measures ()
{
	nr_pass_completed = 0;
	avg_tt = 0.0;
	avg_nr_boardings = 0.0; 
	for (vector <Passenger*>::iterator pass_iter = passengers_during_simulation.begin(); pass_iter < passengers_during_simulation.end(); pass_iter++)
	{
		if ((*pass_iter)->get_end_time() > 0)
		{
			nr_pass_completed++;
			avg_tt += (*pass_iter)->get_end_time() - (*pass_iter)->get_start_time();
			avg_nr_boardings += (*pass_iter)->get_nr_boardings();
			vector<Busstop*> chosen_stops = (*pass_iter)->get_chosen_path_stops();
			bool existing_path = false;
			for (vector<pair<vector<Busstop*>, pair <int,double>>>::iterator iter_path = paths_tt.begin(); iter_path < paths_tt.end(); iter_path++)
			{
				bool same_path = true;
				vector<Busstop*>::iterator checked_path_stops_iter = chosen_stops.begin();
				for (vector<Busstop*>::iterator stops_iter = (*iter_path).first.begin(); stops_iter < (*iter_path).first.end(); stops_iter++)
				{
					if ((*stops_iter)->get_id() != (*checked_path_stops_iter)->get_id() || checked_path_stops_iter == chosen_stops.end())
					{
						same_path = false;
						break;
					}
					checked_path_stops_iter++;
				}
				if (same_path == true)
				{
					(*iter_path).second.first++;
					(*iter_path).second.second += (*pass_iter)->get_end_time() - (*pass_iter)->get_start_time();
					existing_path = true;
					break;
				}
			}
			if (existing_path == false)
			{
				pair<vector<Busstop*>, pair<int,double>> path;
				path.first = chosen_stops;
				path.second.first = 1;
				path.second.second = (*pass_iter)->get_end_time() - (*pass_iter)->get_start_time();
				paths_tt.push_back(path);
			}
		}
	}
	for (vector <pair<vector<Busstop*>, pair <int,double>>>::iterator paths_tt_iter = paths_tt.begin(); paths_tt_iter < paths_tt.end(); paths_tt_iter++)
	{
		(*paths_tt_iter).second.second = (*paths_tt_iter).second.second / (*paths_tt_iter).second.first ;
	}
	avg_tt = avg_tt / nr_pass_completed;
	avg_nr_boardings = avg_nr_boardings / nr_pass_completed;
}

void Pass_alighting_decision::write (ostream& out) 
{ 
	out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t' << chosen_alighting_stop << '\t' ;
	for (map<Busstop*,pair<double,double>>::iterator iter = alighting_MNL.begin(); iter != alighting_MNL.end(); iter++)
	{
		out<< (*iter).first->get_id() << '\t';
		out<< (*iter).second.first << '\t';
		out<< (*iter).second.second << '\t';
	}
	out << endl; 
}