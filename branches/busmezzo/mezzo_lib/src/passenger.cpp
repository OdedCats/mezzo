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
	vector<Pass_path*> path_set = OD_stop->get_path_set();
	for (vector <Pass_path*>::iterator path_iter = path_set.begin(); path_iter < path_set.end(); path_iter++)
	{
		vector<vector<Busline*>> alt_lines = (*path_iter)->get_alt_lines();
		vector<vector<Busline*>>::iterator first_leg = alt_lines.begin();
		for (vector <Busline*>::iterator first_leg_lines = (*first_leg).begin(); first_leg_lines < (*first_leg).end(); first_leg_lines++)
		{
			if (boarding_bus->get_line()->get_id() == (*first_leg_lines)->get_id())
			{
				vector<vector<Busstop*>> alt_stops = (*path_iter)->get_alt_transfer__stops();
				vector<vector<Busstop*>>::iterator stops_iter = alt_stops.begin();
				stops_iter++;
				return (*stops_iter).front();
			}
		}
	}
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

