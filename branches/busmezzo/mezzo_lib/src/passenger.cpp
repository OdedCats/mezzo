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
	OD_stop = OD_stop_;
	boarding_decision = false;
}

bool Passenger:: make_boarding_decision (Bustrip* arriving_bus, double time) 
{
	boarding_decision = random ->brandom(OD_stop->calc_boarding_probability(arriving_bus->get_line()));
	OD_stop->record_passenger_boarding_decision (this, arriving_bus, time, boarding_decision);
	return boarding_decision;
}

Busstop* Passenger::alighting_decision () // assuming that all passenger paths involve only direct trips
{
	return get_OD_stop()->get_destination();
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

