///! odstops.cpp: implementation of the odstops class.
#include "od_stops.h"
#include <math.h>
#include "MMath.h"

ODstops::ODstops ()
{
}

ODstops::ODstops (Busstop* origin_stop_, Busstop* destination_stop_):
origin_stop(origin_stop_), destination_stop(destination_stop_)
{
	min_transfers = 100;
	arrival_rate = 0.0;
	active = false;
	random = new (Random);
	path_set.clear();
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
	path_set.clear();
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

Pass_boarding_decision::~Pass_boarding_decision()
{}

Pass_alighting_decision::~Pass_alighting_decision()
{}

Pass_connection_decision::~Pass_connection_decision()
{}

Pass_waiting_experience::~Pass_waiting_experience()
{}

void ODstops::set_anticipated_waiting_time (Busstop* stop, Busline* line, double anticipated_WT)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	anticipated_waiting_time[stopline] = anticipated_WT;
}

void ODstops::set_anticipated_ivtt (Busstop* stop, Busline* line, Busstop* leg, double anticipated_IVTT)
{
	SLL stoplineleg;
	stoplineleg.stop = stop;
	stoplineleg.line = line;
	stoplineleg.leg = leg;
	anticipated_ivtt[stoplineleg] = anticipated_IVTT;
}

void ODstops::set_alpha_RTI (Busstop* stop, Busline* line, double alpha)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	alpha_RTI[stopline] = alpha;
}

void ODstops::set_alpha_exp (Busstop* stop, Busline* line, double alpha)
{
	pair<Busstop*, Busline*> stopline;
	stopline.first = stop;
	stopline.second = line;
	alpha_exp[stopline] = alpha;
}

void ODstops::set_ivtt_alpha_exp (Busstop* stop, Busline* line, Busstop* leg, double alpha)
{
	SLL stoplineleg;
	stoplineleg.stop = stop;
	stoplineleg.line = line;
	stoplineleg.leg = leg;
	ivtt_alpha_exp[stoplineleg] = alpha;
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
	output_pass_waiting_experience.clear();
	passengers_during_simulation.clear();
}

bool ODstops::execute (Eventlist* eventlist, double curr_time) // generate passengers with this OD and books an event for next passenger generation
{
	double headway_to_next_pass;
	if (curr_time < theParameters->start_pass_generation)
	{
		if (check_path_set() == true)
		{
			headway_to_next_pass = random -> erandom (arrival_rate / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
			if (theParameters->start_pass_generation + headway_to_next_pass < theParameters->stop_pass_generation)
			{
				eventlist->add_event(theParameters->start_pass_generation + headway_to_next_pass, this);
				active = true;
			}
		}
		return true;
	}
	if (curr_time >= theParameters->stop_pass_generation)
	{
		return true;
	}
// called only for generting pass.
	if (active == true) // generate passenger from the second call, as first initialization call just set time to first passenger
	{	
		//Passenger* pass = pass_recycler.newPassenger();
		Passenger* pass = new Passenger;
		passengers_during_simulation.push_back(pass);
		pid++; 
		pass->init (pid, curr_time, this);
		pair<Busstop*,double> stop_time;
		stop_time.first = origin_stop;
		stop_time.second = curr_time;
		pass->add_to_selected_path_stop(stop_time);
		Busstop* connection_stop = pass->make_connection_decision(curr_time);
		stop_time.first = connection_stop;
		if (connection_stop->get_id() != origin_stop->get_id()) // if the pass. walks to another stop
		{
			// set connected_stop as the new origin
			if (connection_stop->check_stop_od_as_origin_per_stop(destination_stop) == false)
			{
				ODstops* od_stop = new ODstops (connection_stop,destination_stop);
				connection_stop->add_odstops_as_origin(destination_stop, od_stop);
				destination_stop->add_odstops_as_destination(connection_stop, od_stop);
			}
			pass->set_ODstop(connection_stop->get_stop_od_as_origin_per_stop(destination_stop)); // set this stop as his new origin (new OD)
			map<Busstop*,double> walk_dis = origin_stop->get_walking_distances();
			double arrival_time_to_connected_stop = curr_time + walk_dis[connection_stop] / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4);
			pass->execute(eventlist, arrival_time_to_connected_stop);
			pair<Busstop*,double> stop_time;
			stop_time.first = connection_stop;
			stop_time.second = arrival_time_to_connected_stop;
			pass->add_to_selected_path_stop(stop_time);
		}
		else // if the pass. stays at the same stop
		{
			waiting_passengers.push_back (pass); // storage the new passenger at the list of waiting passengers with this OD
			pass->set_arrival_time_at_stop(curr_time);
			pass->add_to_selected_path_stop(stop_time);
			if (pass->get_pass_RTI_network_level() == true || this->get_origin()->get_rti() > 0)
			{
				vector<Busline*> lines_at_stop = this->get_origin()->get_lines();
				for (vector <Busline*>::iterator line_iter = lines_at_stop.begin(); line_iter < lines_at_stop.end(); line_iter++)
				{
					pair<Busstop*, Busline*> stopline;
					stopline.first = this->get_origin();
					stopline.second = (*line_iter);
					pass->set_memory_projected_RTI(this->get_origin(),(*line_iter),(*line_iter)->time_till_next_arrival_at_stop_after_time(this->get_origin(),curr_time));
					//pass->set_AWT_first_leg_boarding();
				}
			}
		}
	}
	/*
	for (vector <Passenger*>::iterator wait_pass = waiting_passengers.begin(); wait_pass < waiting_passengers.end(); wait_pass++)
	{
		if ((*wait_pass)->get_OD_stop()->get_origin() != this->get_origin() || (*wait_pass)->get_OD_stop()->get_destination() != this->get_destination())
		{
			break;
		}
	}
	*/
	if (check_path_set() == true)
	{
		headway_to_next_pass = random -> erandom (arrival_rate / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
		eventlist->add_event (curr_time + headway_to_next_pass, this);
		active = true;
	}
	return true;
}

double ODstops::calc_boarding_probability (Busline* arriving_bus, double time, Passenger* pass)
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
		if (path_set.size() < 2) // if the choice-set includes only a single alternative of the arriving bus - then there is no choice left
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
						path_utility = (*iter_paths)->calc_arriving_utility(time, pass);
						set_utilities[(*iter_paths)].first = true;
						set_utilities[(*iter_paths)].second = path_utility;
						boarding_utility += exp(path_utility); 
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
			Busstop* first_boarding_stop = (*iter_paths)->get_alt_transfer_stops()[1].front(); //Added by Jens 2014-06-12 to increase the chance of boarding. Now only paths starting from this stop are evaluated.

			if ((*iter_paths)->get_arriving_bus_rellevant() == false && first_boarding_stop == origin_stop)
			{
				// logsum calculation
				if (check_if_path_is_dominated((*iter_paths), arriving_paths) == false)
				{
					path_utility = (*iter_paths)->calc_waiting_utility((*iter_paths)->get_alt_transfer_stops().begin(), time, false, pass);
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

bool ODstops::check_if_path_is_dominated (Pass_path* considered_path, vector<Pass_path*> arriving_paths)
{
	if (path_set.size() < 2)
	{	
		return false;
	}
	for (vector <Pass_path*>::iterator path2 = arriving_paths.begin(); path2 < arriving_paths.end(); path2++)
	{
		// check if one of the arriving paths dominates the considered path
		if (considered_path->find_number_of_transfers() > (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(0.0) >= (*path2)->calc_total_scheduled_in_vehicle_time(0.0) && considered_path->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance()) 
		{
			return true;
		}
		if (considered_path->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(0.0) > (*path2)->calc_total_scheduled_in_vehicle_time(0.0) * (1+ theParameters->dominancy_perception_threshold)&& considered_path->calc_total_walking_distance() >= (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
		{ 		
			return true;
		}
		if (considered_path->find_number_of_transfers() >= (*path2)->find_number_of_transfers() && considered_path->calc_total_scheduled_in_vehicle_time(0.0) >= (*path2)->calc_total_scheduled_in_vehicle_time(0.0) * (1+ theParameters->dominancy_perception_threshold)&& considered_path->calc_total_walking_distance() > (*path2)->calc_total_walking_distance() * (1 + theParameters->dominancy_perception_threshold)) 
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

bool ODstops::check_path_set ()
{
	if (path_set.empty()==true) 
	{
		return false;
	}
	else 
	{
		return true;
	}
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
	if (check_path_set() == false)
	{
		return -10000;
	}
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		double time_till_transfer = bus_on_board->get_line()->calc_curr_line_ivt(pass->get_OD_stop()->get_origin(),origin_stop,pass->get_OD_stop()->get_origin()->get_rti(), time); // in seconds
		staying_utility += exp(random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * (time_till_transfer/60) + random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4 )  +  (*paths)->calc_waiting_utility((*paths)->get_alt_transfer_stops().begin(), time + time_till_transfer, true, pass));
		// taking into account IVT till this intermediate stop, transfer penalty and the utility of the path from this transfer stop till the final destination
	}
	return log(staying_utility);
}

double ODstops::calc_combined_set_utility_for_alighting_zone (Passenger* pass, Bustrip* bus_on_board, double time)
{
	// calc logsum over all the paths from this origin stop
	staying_utility = 0.0;
	for (vector <Pass_path*>::iterator paths = path_set.begin(); paths < path_set.end(); paths++)
	{
		double time_till_transfer = bus_on_board->get_line()->calc_curr_line_ivt(pass->get_OD_stop()->get_origin(),origin_stop,pass->get_OD_stop()->get_origin()->get_rti(), time); // in seconds
		staying_utility += exp(random->nrandom(theParameters->in_vehicle_time_coefficient, theParameters->in_vehicle_time_coefficient / 4 ) * (time_till_transfer/60) + random->nrandom(theParameters->transfer_coefficient, theParameters->transfer_coefficient / 4 )  +  (*paths)->calc_waiting_utility((*paths)->get_alt_transfer_stops().begin(), time + time_till_transfer, true, pass)) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
		// taking into account IVT till this intermediate stop, transfer penalty and the utility of the path from this transfer stop till the final destination (incl. final walking link)
	}
	// in case it is walkable from the alighting stop till the final destination
	if (pass->stop_is_in_d_zone(origin_stop) && pass->get_destination_walking_distance(origin_stop) < theParameters->max_walking_distance)
	{
		staying_utility += exp(theParameters->in_vehicle_time_coefficient * ((bus_on_board->get_line()->calc_curr_line_ivt(origin_stop,destination_stop,origin_stop->get_rti(), time))/60) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4)));
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

double ODstops::calc_combined_set_utility_for_connection (double walking_distance, double time,Passenger* pass)
{
	// calc logsum over all the paths from this origin stop
	double connection_utility = 0.0;
	if (check_path_set() == false)
	{
		return -10000;
	}
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
			connection_utility += exp(random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * time_till_connected_stop + (*paths)->calc_waiting_utility(alt_stops_iter, time + (time_till_connected_stop * 60), false, pass));
			// taking into account CT (walking time) till this connected stop and the utility of the path from this connected stop till the final destination
		}
	}
	return log(connection_utility);
}

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
			connection_utility += exp(random->nrandom(theParameters->walking_time_coefficient, theParameters->walking_time_coefficient/4) * time_till_connected_stop + (*paths)->calc_waiting_utility(alt_stops_iter, time + (time_till_connected_stop * 60), false, pass)) + theParameters->walking_time_coefficient * (pass->get_destination_walking_distance(destination_stop) / random->nrandom(theParameters->average_walking_speed, theParameters->average_walking_speed/4));
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

void ODstops::record_passenger_boarding_decision (Passenger* pass, Bustrip* trip, double time, double boarding_probability, bool boarding_decision)  // add to output structure boarding decision info
{
	int original_origin_id = pass->get_original_origin()->get_id();
	int destination_id = pass->get_OD_stop()->get_destination()->get_id();
	int origin_id = pass->get_OD_stop()->get_origin()->get_id();
	output_pass_boarding_decision[pass].push_back(Pass_boarding_decision(pass->get_id(), original_origin_id, destination_id, trip->get_line()->get_id(), trip->get_id() , origin_id , time, pass->get_start_time(), boarding_probability , boarding_decision, boarding_utility, staying_utility)); 
}

void ODstops::record_passenger_alighting_decision (Passenger* pass, Bustrip* trip, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> alighting_MNL)  //  add to output structure alighting decision info
{
	output_pass_alighting_decision[pass].push_back(Pass_alighting_decision(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), chosen_alighting_stop->get_id(), alighting_MNL)); 
}

void ODstops::record_passenger_connection_decision (Passenger* pass, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> connecting_MNL_)  //  add to output structure connection decision info
{
	output_pass_connection_decision[pass].push_back(Pass_connection_decision(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), chosen_alighting_stop->get_id(), connecting_MNL_)); 
}

void ODstops::record_waiting_experience(Passenger* pass, Bustrip* trip, double time, int level_of_rti_upon_decision, double projected_RTI, double AWT)  //  add to output structure action info
{
	double expected_WT_PK = (trip->get_line()->calc_curr_line_headway())/2; // in seconds
	double experienced_WT = time - pass->get_arrival_time_at_stop();
	output_pass_waiting_experience[pass].push_back(Pass_waiting_experience(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id() , time, pass->get_start_time(), expected_WT_PK, level_of_rti_upon_decision, projected_RTI ,experienced_WT, AWT)); 
}

void ODstops::record_onboard_experience(Passenger* pass, Bustrip* trip, double time, Busstop* stop, pair<double,double> riding_coeff)
{
	double expected_ivt;
	double first_stop_time;
	double second_stop_time;
	for (vector<Visit_stop*>::iterator stop_v = trip->stops.begin(); stop_v < trip->stops.end(); stop_v++)
	{
			if ((*stop_v)->first->get_id() == stop->get_id())
			{
				second_stop_time = (*stop_v)->second;
				expected_ivt = second_stop_time - first_stop_time; //when the right stop is found, the visit time is substracted by the visit time of the previous stop
				break;
			}
			first_stop_time = (*stop_v)->second;
	}
	double experienced_ivt = riding_coeff.first*riding_coeff.second;
	output_pass_onboard_experience[pass].push_back(Pass_onboard_experience(pass->get_id(), pass->get_original_origin()->get_id(), pass->get_OD_stop()->get_destination()->get_id(), trip->get_line()->get_id(), trip->get_id() , pass->get_OD_stop()->get_origin()->get_id(), stop->get_id(), expected_ivt, experienced_ivt));
}

void ODstops::write_boarding_output(ostream & out, Passenger* pass)
{
	for (list <Pass_boarding_decision>::iterator iter = output_pass_boarding_decision[pass].begin(); iter!=output_pass_boarding_decision[pass].end();iter++)
	{
		iter->write(out);
	}
}

void ODstops::write_waiting_exp_output(ostream & out, Passenger* pass)
{
	for (list <Pass_waiting_experience>::iterator iter = output_pass_waiting_experience[pass].begin(); iter!=output_pass_waiting_experience[pass].end();iter++)
	{
		iter->write(out);
	}
}

void ODstops::write_onboard_exp_output(ostream & out, Passenger* pass)
{
	for (list <Pass_onboard_experience>::iterator iter = output_pass_onboard_experience[pass].begin(); iter!=output_pass_onboard_experience[pass].end();iter++)
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

void ODstops::write_connection_output(ostream & out, Passenger* pass)
{
	for (list <Pass_connection_decision>::iterator iter = output_pass_connection_decision[pass].begin(); iter!=output_pass_connection_decision[pass].end();iter++)
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

void ODstops::write_od_summary_without_paths(ostream & out)
{
	out << origin_stop->get_id() << '\t' << destination_stop->get_id() << '\t' << nr_pass_completed << '\t' << avg_tt << '\t' << avg_nr_boardings << '\t' << endl; 
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
			vector<Busstop*> chosen_stops;
			vector <pair<Busstop*,double>> stops_time = (*pass_iter)->get_selected_path_stops();
			for (vector <pair<Busstop*,double>>::iterator stops = stops_time.begin(); stops < stops_time.end(); stops++)
			{
				chosen_stops.push_back((*stops).first);
			}
			bool existing_path = false;
			for (vector<pair<vector<Busstop*>, pair <int,double>>>::iterator iter_path = paths_tt.begin(); iter_path < paths_tt.end(); iter_path++)
			{
				bool same_path = true;
				vector <pair<Busstop*,double>>::iterator checked_path_stops_iter = stops_time.begin();
				for (vector<Busstop*>::iterator stops_iter = (*iter_path).first.begin(); stops_iter < (*iter_path).first.end(); stops_iter++)
				{
					if ((*stops_iter)->get_id() != (*checked_path_stops_iter).first->get_id() || checked_path_stops_iter == stops_time.end())
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

void Pass_connection_decision::write (ostream& out)
{ 
	out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << stop_id<< '\t' << time << '\t'<< generation_time << '\t' << chosen_connection_stop << '\t';
	for (map<Busstop*,pair<double,double>>::iterator iter = connecting_MNL.begin(); iter != connecting_MNL.end(); iter++)
	{
		out<< (*iter).first->get_id() << '\t';
		out<< (*iter).second.first << '\t';
		out<< (*iter).second.second << '\t';
	}
	out << endl;
}

ODzone::ODzone (int zone_id)
{
	id = zone_id;
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

void ODzone::add_stop_distance (Busstop* stop, double mean_distance, double sd_distance)
{
	stops_distances[stop].first = mean_distance; 
	stops_distances[stop].second = sd_distance;
}

void ODzone::add_arrival_rates (ODzone* d_zone, double arrival_rate)
{
	arrival_rates[d_zone] = arrival_rate;
}

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
			pair<Busstop*,double> stop_time;
			stop_time.first = origin_stop;
			stop_time.second = curr_time;
			pass->add_to_selected_path_stop(stop_time);
			pass->add_to_selected_path_stop(stop_time); // twice - to maintain the same path structure
			passengers_during_simulation.push_back(pass);
			map <Busstop*,pair<double,double>> d_stops = (*dzones_iter).first->get_stop_distances();
			pass->set_ODstop(origin_stop->get_stop_od_as_origin_per_stop(d_stops.begin()->first)); // set the origin stop as pass's origin and an arbitary destination stop
			pass->execute(eventlist, curr_time + pass->get_origin_walking_distance(origin_stop) / random->nrandom (theParameters->average_walking_speed, theParameters->average_walking_speed/4));
			pass->get_OD_stop()->get_waiting_passengers().push_back(pass);
			// book next pass. generation
			double headway_to_next_pass = random -> erandom ((*dzones_iter).second / 3600.0); // passenger arrival is assumed to be a poission process (exp headways)
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

Pass_boarding_decision_zone::~Pass_boarding_decision_zone()
{}

Pass_alighting_decision_zone::~Pass_alighting_decision_zone()
{}

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
