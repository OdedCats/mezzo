///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
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
}

bool Passenger:: boarding_decision (Busline* arriving_bus) // assuming that passengers board the first bus arriving at the stop (regardless of its route)
{
	random ->brandom(OD_stop->calc_boarding_probability(arriving_bus));
	return true;
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

