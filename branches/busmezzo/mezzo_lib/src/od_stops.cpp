///! odstops.cpp: implementation of the odstops class.
#include "od_stops.h"
#include <math.h>
#include "MMath.h"

ODstops::ODstops ()
{
}

ODstops::ODstops (Busstop* origin_stop_, Busstop* destination_stop_)
{
	origin_stop = origin_stop_;
	destination_stop = destination_stop_;
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
	min_IVT = 100000;
	active = false;
	boarding_utility = 0;
	staying_utility = 0;
	waiting_passengers.clear();
	output_pass_boarding_decision.clear();
	output_pass_alighting_decision.clear();
	passengers_during_simulation.clear(); //we completely erase the passengers from the memory
}

void ODstops::end_of_day()
{
	min_transfers = 100;
	min_IVT = 100000;
	active = false;
	boarding_utility = 0;
	staying_utility = 0;
	waiting_passengers.clear();
	output_pass_boarding_decision.clear();
	output_pass_alighting_decision.clear();
	for (vector <Passenger*>::iterator iter_pass = passengers_during_simulation.begin(); iter_pass < passengers_during_simulation.end(); iter_pass++)
	{
		(*iter_pass)->evening_cleaning();
	}
		
}

bool ODstops::execute (Eventlist* eventlist, double curr_time) // generate passengers with this OD and books an event for next passenger generation
{
	if (curr_time < theParameters->start_pass_generation)
	{
		eventlist->add_event(theParameters->start_pass_generation, this);
		return true;
	}
	if (curr_time >= theParameters->stop_pass_generation)
	{
		return true;
	}
// called only for generting pass.
	if(theParameters->calendar.size()==1)
	{
		if (active == true) // generate passenger from the second call, as first initialization call just set time to first passenger
		{	
			Passenger* pass = new Passenger;
			passengers_during_simulation.push_back(pass);
			pid++; 
			pass->init (pid, curr_time, this);
			pass->add_to_selected_path_stop(origin_stop);
			Busstop* connection_stop = pass->make_connection_decision(curr_time);
			pass->add_to_selected_path_stop(connection_stop);
			if (connection_stop->get_id() != origin_stop->get_id()) // if the pass. walks to another stop
			{
				// set connected_stop as the new origin
				pass->set_ODstop(connection_stop->get_stop_od_as_origin_per_stop(destination_stop)); // set this stop as his new origin (new OD)
				map<Busstop*,double> walk_dis = origin_stop->get_walking_distances();
				double walking_time = 60 * (walk_dis[connection_stop] / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4));
				pass->add_to_walking_time (walking_time);
				pass->set_last_arrival_time_at_stop(curr_time + walking_time);
				pass->add_event_to_pass (eventlist, curr_time + walking_time);
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
		random = new (Random);
		if (randseed != 0)
		{
			random->seed(randseed);
		}
		else
		{
			random->randomize();
		}
		double headway_to_next_pass = random->erandom(arrival_rate/3600.0); // passenger arrival is assumed to be a poission process (exp headways)
		eventlist->add_event (curr_time + headway_to_next_pass, this);
		tracer_pass = passengers_during_simulation.begin(); //we initialize here the iterator as pointer to the first value
		active = true;
		return true;
	}
	else
	{ 
		if (active == true)
	    {	
		  (*tracer_pass)->add_to_selected_path_stop(origin_stop);
		  Busstop* connection_stop = (*tracer_pass)->make_connection_decision(curr_time);
	      (*tracer_pass)->add_to_selected_path_stop(connection_stop);
		  if (connection_stop->get_id() != origin_stop->get_id()) // if the pass. walks to another stop
	      {
			// set connected_stop as the new origin
			(*tracer_pass)->set_ODstop(connection_stop->get_stop_od_as_origin_per_stop(destination_stop)); // set this stop as his new origin (new OD)
			map<Busstop*,double> walk_dis = origin_stop->get_walking_distances();
			double walking_time = 60 * (walk_dis[connection_stop] / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4));
			(*tracer_pass)->add_to_walking_time (walking_time);
			(*tracer_pass)->set_last_arrival_time_at_stop(curr_time + walking_time);
			(*tracer_pass)->add_event_to_pass (eventlist, curr_time + walking_time);
		  }
		  else // if the pass. stays at the same stop
		  {
			waiting_passengers.push_back ((*tracer_pass)); // storage the new passenger at the list of waiting passengers with this OD
		  }
		  
		  if(tracer_pass<passengers_during_simulation.end())
		  { //this instruction is to progress to the next passenger in the vector of passengers at this stop
			(*tracer_pass)++;  
			eventlist->add_event ((*tracer_pass)->get_start_time(), this);
		  }
		  else
		  {
			  tracer_pass=passengers_during_simulation.begin();
		  }
		}
	}
	active = true;
	return true;
}

double ODstops::calc_boarding_probability (Passenger* pass, Busline* arriving_bus, double time, bool has_network_rti)
{
	// initialization
	boarding_utility = 0.0;
	staying_utility = 0.0;
	double path_utility = 0.0;
	map<Pass_path*,pair<bool,double>> set_utilities; // true - boarding, false - staying
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
			boarding_utility = 2.0;
			staying_utility = -2.0;
			return 1;
		}
		vector<Pass_path*> arriving_paths;
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
						path_utility = (*iter_paths)->calc_arriving_utility(pass, time, has_network_rti);
						set_utilities[(*iter_paths)].first = true;
						set_utilities[(*iter_paths)].second = path_utility;
						boarding_utility += exp(path_utility); 
						/*FAB*/ //boarding_utility = 0.5*exp(path_utility)+0.5*exp((*iter_paths)->calc_arriving_utility(pass, exp_waiting_time, false)); 
						arriving_paths.push_back((*iter_paths));
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
				if (check_if_path_is_dominated(pass,(*iter_paths), arriving_paths) == false)
				{
					path_utility = (*iter_paths)->calc_waiting_utility(this, pass,(*iter_paths)->get_alt_transfer_stops().begin(), time, false, has_network_rti, false);
					set_utilities[(*iter_paths)].first = false;
					set_utilities[(*iter_paths)].second = path_utility;
					staying_utility += exp(path_utility);
				}
			}
		}
		if (staying_utility == 0.0) // in case all the staying alternatives are dominated by arriving alternatives
		{
			boarding_utility = 2.0;
			staying_utility = -2.0;
			return 1;
		}
		else
		{
			staying_utility = log (staying_utility);
		}
		// calculate the probability to board
		switch (theParameters->choice_model)
		{
			case 1:
				return calc_binary_logit(boarding_utility, staying_utility); 
			case 2:
				return calc_path_size_logit(set_utilities, boarding_utility, staying_utility);
		}
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

bool ODstops::check_if_path_is_dominated (Passenger* pass, Pass_path* considered_path, vector<Pass_path*> arriving_paths)
{
	if (path_set.size() < 2)
	{	
		return false;
	}
	for (vector <Pass_path*>::iterator path2 = arriving_paths.begin(); path2 < arriving_paths.end(); path2++)
	{
		// check if one of the arriving paths dominates the considered path
		if (considered_path->find_number_of_transfers() > (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(pass) >= (*path2)->calc_total_scheduled_in_vehicle_time(pass) && considered_path->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance()) 
		{
			return true;
		}
		if (considered_path->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(pass) > (*path2)->calc_total_scheduled_in_vehicle_time(pass) * (1+ theParameters->dominancy_perception_threshold)&& considered_path->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
		{ 		
			return true;
		}
		if (considered_path->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(pass) >= (*path2)->calc_total_scheduled_in_vehicle_time(pass) * (1+ theParameters->dominancy_perception_threshold)&& considered_path->calc_total_walking_distance() > (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
		{
			return true;
		}
	}
	return false;
}

double ODstops::calc_binary_logit (double utility_i, double utility_j)
{
	return ((exp(utility_i)) / (exp(utility_i) + exp (utility_j)));
}

double ODstops::calc_multinomial_logit (double utility_i, double utility_sum)
{
	return ((exp(utility_i)) / utility_sum);
}

double ODstops::calc_path_size_logit (map<Pass_path*,pair<bool,double>> set_utilities, double utliity_i, double utliity_j)
{
	map<Pass_path*,double> u_1,u_2,f_1,f_2,e_1,e_2,p_1,p_2;
	double sum_1 = 0.0;
	double sum_2 = 0.0;
	double w_ff1 = 0.0;
	double w_ff2 = 0.0;
	// distinguish between two clusters
	for (map<Pass_path*,pair<bool,double>>::iterator u_iter = set_utilities.begin(); u_iter!= set_utilities.end(); u_iter++)
	{
		if ((*u_iter).second.first == true)
		{
			u_1[(*u_iter).first] = (*u_iter).second.second;
		}
		else
		{
			u_2[(*u_iter).first] = (*u_iter).second.second;
		}
	}
	// PSL at the cluster level
	// calc factors
	if (f_1.size() < 2)
	{
		f_1[(*(u_1.begin())).first] = 1.0;
	}
	else
	{
		f_1 = calc_path_size_factor_nr_stops(u_1);
	}
	if (f_2.size() < 2)
	{
		f_2[(*(u_2.begin())).first] = 1.0;
	}
	else
	{
		f_2 = calc_path_size_factor_nr_stops(u_2);
	}
	// calc sums for prob fun
	for (map<Pass_path*,double>::iterator set_iter = f_1.begin(); set_iter!= f_1.end(); set_iter++)
	{
		e_1[(*set_iter).first] = exp(u_1[(*set_iter).first]+log((*set_iter).second));
		sum_1 += e_1[(*set_iter).first];
	}
	for (map<Pass_path*,double>::iterator set_iter = f_2.begin(); set_iter!= f_2.end(); set_iter++)
	{
		e_2[(*set_iter).first] = exp(u_2[(*set_iter).first]+log((*set_iter).second));
		sum_2 += e_2[(*set_iter).first];
	}
	for (map<Pass_path*,double>::iterator set_iter = f_1.begin(); set_iter!= f_1.end(); set_iter++)
	{
		p_1[(*set_iter).first] = e_1[(*set_iter).first]/sum_1;
	}
	for (map<Pass_path*,double>::iterator set_iter = f_2.begin(); set_iter!= f_2.end(); set_iter++)
	{
		p_2[(*set_iter).first] = e_2[(*set_iter).first]/sum_2;
	}
	for (map<Pass_path*,double>::iterator set_iter = p_1.begin(); set_iter!= p_1.end(); set_iter++)
	{
		w_ff1 = calc_path_size_factor_between_clusters((*set_iter).first,p_2) * (*set_iter).second;
	}
	for (map<Pass_path*,double>::iterator set_iter = p_2.begin(); set_iter!= p_2.end(); set_iter++)
	{
		w_ff2 = calc_path_size_factor_between_clusters((*set_iter).first,p_1) * (*set_iter).second;
	}
	return (exp(utliity_i+log(w_ff1)) / (exp(utliity_i+log(w_ff1)) + exp (utliity_j+log(w_ff2))));
}

double ODstops::calc_path_size_factor_between_clusters (Pass_path* path, map<Pass_path*,double> cluster_probs)
{
	vector<vector<Busstop*>> alt_transfer_stops = path->get_alt_transfer_stops();
	double factor = 0.0;
	double nr_counted_stops = 2 * (path->get_number_of_transfers()+1); // number of stops minus origin and destination
	double nr_stops_set_factor;
	map<Busstop*,int> delta_stops;
	map<Busstop*,double> stop_factor;
	for (vector<vector<Busstop*>>::iterator alt_transfer_stops_iter = alt_transfer_stops.begin(); alt_transfer_stops_iter < alt_transfer_stops.end(); alt_transfer_stops_iter++)
	{
		nr_stops_set_factor = 1 / (*alt_transfer_stops_iter).size();
		for (vector<Busstop*>::iterator stops_iter = (*alt_transfer_stops_iter).begin(); stops_iter < (*alt_transfer_stops_iter).end(); stops_iter++)
		{
			delta_stops[(*stops_iter)] = 0;	
			for (map<Pass_path*,double>::iterator set_iter = cluster_probs.begin(); set_iter != cluster_probs.end(); set_iter++)
			{
				vector<vector<Busstop*>> alt_transfer_stops1 = (*set_iter).first->get_alt_transfer_stops();
				for (vector<vector<Busstop*>>::iterator alt_transfer_stops_iter1 = alt_transfer_stops1.begin(); alt_transfer_stops_iter1 < alt_transfer_stops1.end(); alt_transfer_stops_iter1++)
				{
					for (vector<Busstop*>::iterator stops_iter1 = (*alt_transfer_stops_iter1).begin(); stops_iter1 < (*alt_transfer_stops_iter1).end(); stops_iter1++)
					{
						if ((*stops_iter)->get_id() == (*stops_iter1)->get_id())
						{
							delta_stops[(*stops_iter)]++;
							
						}
					}
				}
				stop_factor[(*stops_iter)] = 1 / (nr_stops_set_factor * delta_stops[(*stops_iter)]);
				factor += 1/(stop_factor[(*stops_iter)] * (*set_iter).second * nr_counted_stops); // weight the factor by the within-cluster prob. of the compared path
			}
		}		
	}
	return factor;
}

map<Pass_path*,double> ODstops::calc_path_size_factor_nr_stops (map<Pass_path*,double> cluster_set_utilities)
{
	map <Pass_path*,double> set_factors;
	double nr_counted_stops;
	double nr_stops_set_factor;
	for (map<Pass_path*,double>::iterator set_iter = cluster_set_utilities.begin(); set_iter != cluster_set_utilities.end(); set_iter++)
	{
		set_factors[(*set_iter).first] = 0.0;
		nr_counted_stops = 2 * ((*set_iter).first->get_number_of_transfers()+1); // number of stops minus origin and destination	
		map<Busstop*,int> delta_stops;
		map<Busstop*,double> stop_factor;
		vector<vector<Busstop*>> alt_transfer_stops = (*set_iter).first->get_alt_transfer_stops();
		for (vector<vector<Busstop*>>::iterator alt_transfer_stops_iter = alt_transfer_stops.begin(); alt_transfer_stops_iter < alt_transfer_stops.end(); alt_transfer_stops_iter++)
		{
			nr_stops_set_factor = 1 / (*alt_transfer_stops_iter).size();
			for (vector<Busstop*>::iterator stops_iter = (*alt_transfer_stops_iter).begin(); stops_iter < (*alt_transfer_stops_iter).end(); stops_iter++)
			{
				delta_stops[(*stops_iter)] = 0;
				for (map<Pass_path*,double>::iterator set_iter1 = cluster_set_utilities.begin(); set_iter1 != cluster_set_utilities.end(); set_iter1++)
				{
					vector<vector<Busstop*>> alt_transfer_stops1 = (*set_iter1).first->get_alt_transfer_stops();
					for (vector<vector<Busstop*>>::iterator alt_transfer_stops_iter1 = alt_transfer_stops1.begin(); alt_transfer_stops_iter1 < alt_transfer_stops1.end(); alt_transfer_stops_iter1++)
					{
						for (vector<Busstop*>::iterator stops_iter1 = (*alt_transfer_stops_iter1).begin(); stops_iter1 < (*alt_transfer_stops_iter1).end(); stops_iter1++)
						{
							if ((*stops_iter)->get_id() == (*stops_iter1)->get_id())
							{
								delta_stops[(*stops_iter)]++;
							}
						}
					}
				}
				stop_factor[(*stops_iter)] = 1 / (nr_stops_set_factor * delta_stops[(*stops_iter)]);
				set_factors[(*set_iter).first] += 1/ (stop_factor[(*stops_iter)] * 	nr_counted_stops);
	;
			}
		}
	}
	return set_factors;
}	

double ODstops::calc_combined_set_utility_for_alighting (Passenger* pass, Bustrip* bus_on_board, double time)
{
	// calc logsum over all the paths from this origin stop
	staying_utility = 0.0;
	int level_of_rti = pass->get_OD_stop()->get_origin()->get_rti();
	if (pass->get_has_network_rti() == 1)
	{
		level_of_rti = 3;
	}
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		double time_till_transfer = bus_on_board->get_line()->calc_curr_line_ivt(pass, pass->get_OD_stop()->get_origin(),origin_stop,level_of_rti); // in seconds
		staying_utility += exp(random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * (time_till_transfer/60) + random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4 )  +  (*paths)->calc_waiting_utility(this, pass, (*paths)->get_alt_transfer_stops().begin(), time + time_till_transfer, true, level_of_rti, false));
		// taking into account IVT till this intermediate stop, transfer penalty and the utility of the path from this transfer stop till the final destination
	}
	return log(staying_utility);
}

/*
double ODstops::calc_combined_set_utility_for_alighting_zone (Passenger* pass, Bustrip* bus_on_board, double time)
{
	// calc logsum over all the paths from this origin stop
	staying_utility = 0.0;
	int level_of_rti = pass->get_OD_stop()->get_origin()->get_rti();
	if (pass->get_has_network_rti() == 1)
	{
		level_of_rti = 3;
	}
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		double time_till_transfer = bus_on_board->get_line()->calc_curr_line_ivt(pass, pass->get_OD_stop()->get_origin(),origin_stop,level_of_rti); // in seconds
		staying_utility += exp(random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * (time_till_transfer/60) + random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4 )  +  (*paths)->calc_waiting_utility(this, pass, (*paths)->get_alt_transfer_stops().begin(), time + time_till_transfer, true, level_of_rti, false)) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
		// taking into account IVT till this intermediate stop, transfer penalty and the utility of the path from this transfer stop till the final destination (incl. final walking link)
	}
	// in case it is walkable from the alighting stop till the final destination
	if (pass->stop_is_in_d_zone(origin_stop) && pass->get_destination_walking_distance(origin_stop) < theParameters->max_walking_distance)
	{
		staying_utility += exp(theParameters->in_vehicle_time_coefficient * ((bus_on_board->get_line()->calc_curr_line_ivt(pass, origin_stop,destination_stop,level_of_rti))/60) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4)));
	}
	else
	{
		if (path_set.size() == 0)
		{
			staying_utility = -10.0;
			return staying_utility;
		}
	}
	return log(staying_utility);
}
*/

double ODstops::calc_combined_set_utility_for_connection (Passenger* pass, double walking_distance, double time, bool has_network_rti)
{
	// calc logsum over all the paths from this origin stop
	double connection_utility = 0.0;
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		bool without_walking_first = false;
		// go only through paths that does not include walking to another stop from this connection stop
		vector<vector<Busstop*>> alt_stops = (*paths)->get_alt_transfer_stops();
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
			connection_utility += exp(random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * time_till_connected_stop + (*paths)->calc_waiting_utility(this, pass, alt_stops_iter, time + (time_till_connected_stop * 60), false, has_network_rti, false));
			// taking into account CT (walking time) till this connected stop and the utility of the path from this connected stop till the final destination
		}
	}
	return log(connection_utility);
}

/*
double ODstops::calc_combined_set_utility_for_connection_zone (Passenger* pass, double walking_distance, double time)
{
	// calc logsum over all the paths from this origin stop
	double connection_utility = 0.0;
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		bool without_walking_first = false;
		// go only through paths that does not include walking to another stop from this connection stop
		vector<vector<Busstop*>> alt_stops = (*paths)->get_alt_transfer_stops();
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
			connection_utility += exp(random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * time_till_connected_stop + (*paths)->calc_waiting_utility(this, pass, alt_stops_iter, time + (time_till_connected_stop * 60), false,pass->get_has_network_rti(),false )) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
			// taking into account CT (walking time) till this connected stop and the final destination and the utility of the path from this connected stop till the final destination (incl. final walking link)
		}
	}
	// if walking distance till the destination is below threshold, then the alternative of walking only is also included
	if (pass->get_destination_walking_distance(destination_stop) < theParameters->max_walking_distance)
	{
		connection_utility += exp(theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4)));
	}
	return log(connection_utility);
}
*/

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
	
	//we save here the performance measures of the current day
	theParameters->calendar.back()->day_nr_pass_completed+=nr_pass_completed;
	theParameters->calendar.back()->day_avg_total_travel_time+=avg_tinvehicle + avg_twait + avg_twalk;
	theParameters->calendar.back()->day_avg_tinvehicle+=avg_tinvehicle;
	theParameters->calendar.back()->day_avg_twait+=avg_twait;
	theParameters->calendar.back()->day_avg_twalk+=avg_twalk;

	out << origin_stop->get_id() << '\t' << destination_stop->get_id() << '\t' << nr_pass_completed << '\t' << avg_tinvehicle + avg_twait + avg_twalk << '\t' << avg_tinvehicle << '\t' << avg_twait << '\t'<< avg_twalk << '\t' << avg_nr_boardings << '\t' << paths_tt.size()<< '\t'<< endl; 
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
	avg_tinvehicle = 0.0;
	avg_twalk = 0.0;
	avg_twait = 0.0;
	avg_nr_boardings = 0.0; 
	for (vector <Passenger*>::iterator pass_iter = passengers_during_simulation.begin(); pass_iter < passengers_during_simulation.end(); pass_iter++)
	{
		if ((*pass_iter)->get_end_time() > 0)
		{
			nr_pass_completed++;
			avg_tt += (*pass_iter)->get_end_time() - (*pass_iter)->get_start_time();
			avg_twalk += (*pass_iter)->get_walking_time();
			avg_twait += (*pass_iter)->get_waiting_time();
			avg_tinvehicle += (*pass_iter)->get_in_vehicle_time();
			avg_nr_boardings += (*pass_iter)->get_nr_boardings();
			vector<Busstop*> chosen_stops = (*pass_iter)->get_chosen_path_stops();
			bool existing_path = false;
			for (vector<pair<vector<Busstop*>, pair <int,double>>>::iterator iter_path = paths_tt.begin(); iter_path < paths_tt.end(); iter_path++)
			{
				bool same_path = true;
				vector<Busstop*>::iterator checked_path_stops_iter = chosen_stops.begin();
				for (vector<Busstop*>::iterator stops_iter = (*iter_path).first.begin(); stops_iter < (*iter_path).first.end(); stops_iter++)
				{
					if (checked_path_stops_iter == chosen_stops.end())
					{
						same_path = false;
						break;
					}
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
	avg_tinvehicle = avg_tinvehicle / nr_pass_completed;
	avg_twalk = avg_twalk / nr_pass_completed;
	avg_twait = avg_twait / nr_pass_completed;
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

ODzone::ODzone (Zone* origin_zone_, Zone* destination_zone_, double arrival_rate_)
{
	origin_zone = origin_zone_;
	destination_zone = destination_zone_;
	arrival_rate = arrival_rate_;
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

ODzone::~ODzone()
{
	delete random;
}

/*
bool ODzone::execute (Eventlist* eventlist, double curr_time)
{
	if (curr_time < theParameters->start_pass_generation)
	{
		eventlist->add_event(theParameters->start_pass_generation, this);
		return true;
	}
	if (curr_time >= theParameters->stop_pass_generation)
	{
		return true;
	}
// called only for generting pass.
	if (active = true) // generate passenger from the second call, as first initialization call just set time to first passenger
	{	
		// for each of the destination zones from this origin zone
		for (map<ODzone*,double>::iterator dzones_iter = arrival_rates.begin(); dzones_iter != arrival_rates.end(); dzones_iter++)
		{
			//Passenger* pass = pass_recycler.newPassenger();
			Passenger* pass = new Passenger;
			pid++; 
			pass->init_zone(pid, curr_time, this, (*dzones_iter).first);
			pass->set_origin_walking_distances(pass->sample_walking_distances(this));
			pass->set_destination_walking_distances(pass->sample_walking_distances((*dzones_iter).first));
			Busstop* origin_stop = pass->make_first_stop_decision(curr_time);
			pass->add_to_selected_path_stop(origin_stop);
			pass->add_to_selected_path_stop(origin_stop); // twice - to maintain the same path structure
			passengers_during_simulation.push_back(pass);
			map <Busstop*,pair<double,double>> d_stops = (*dzones_iter).first->get_stop_distances();
			pass->set_ODstop(origin_stop->get_stop_od_as_origin_per_stop(d_stops.begin()->first)); // set the origin stop as pass's origin and an arbitary destination stop
			pass->execute(eventlist, curr_time + pass->get_origin_walking_distance(origin_stop) / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4));
			pass->get_OD_stop()->get_waiting_passengers().push_back(pass);
			// book next pass. generation
			double headway_to_next_pass = random->erandom((*dzones_iter).second / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
			eventlist->add_event (curr_time + headway_to_next_pass, this);
		}
	}
	active = true;
	return true;
}

void ODzone::record_passenger_boarding_decision_zone (Passenger* pass, Bustrip* trip, double time, double boarding_probability, bool boarding_decision)  // add to output structure boarding decision info
{
	output_pass_boarding_decision_zone[pass].push_back(Pass_boarding_decision_zone(pass->get_id(), id, pass->get_d_zone()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), boarding_probability , boarding_decision, boarding_utility, staying_utility)); 
}

void ODzone::record_passenger_alighting_decision_zone (Passenger* pass, Bustrip* trip, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> alighting_MNL)  //  add to output structure alighting decision info
{
	output_pass_alighting_decision_zone[pass].push_back(Pass_alighting_decision_zone(pass->get_id(), id, pass->get_d_zone()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), chosen_alighting_stop->get_id(), alighting_MNL)); 
}

void ODzone::write_boarding_output_zone(ostream & out, Passenger* pass)
{
	for (list <Pass_boarding_decision_zone>::iterator iter = output_pass_boarding_decision_zone[pass].begin(); iter!=output_pass_boarding_decision_zone[pass].end();iter++)
	{
		iter->write(out);
	}
}

void ODzone::write_alighting_output_zone(ostream & out, Passenger* pass)
{
	for (list <Pass_alighting_decision_zone>::iterator iter = output_pass_alighting_decision_zone[pass].begin(); iter!=output_pass_alighting_decision_zone[pass].end();iter++)
	{
		iter->write(out);
	}
}


void ODzone::reset()
{
	boarding_utility = 0; 
	staying_utility = 0;
	active = false;
	output_pass_boarding_decision_zone.clear();
	output_pass_alighting_decision_zone.clear();
	passengers_during_simulation.clear();
	nr_pass_completed = 0; 
	avg_tt = 0;
	avg_nr_boardings = 0;
	paths_tt.clear(); 
}

*/
void Pass_alighting_decision_zone::write (ostream& out) 
{ 
	out << pass_id << '\t' << origin_zone << '\t' << destination_zone << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t' << chosen_alighting_stop << '\t' ;
	for (map<Busstop*,pair<double,double>>::iterator iter = alighting_MNL.begin(); iter != alighting_MNL.end(); iter++)
	{
		out<< (*iter).first->get_id() << '\t';
		out<< (*iter).second.first << '\t';
		out<< (*iter).second.second << '\t';
	}
	out << endl; 
}
