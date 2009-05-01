///! passenger.cpp: implementation of the passenger class.
#include "passenger.h"
Passenger::Passenger ()
{
}

Passenger::~Passenger()
{}

Passenger::Passenger (double start_time_)
{
	start_time = start_time_;
}

void Passenger::init (double start_time_, Busstop* origin_stop, Busstop* destination_stop)
{
	start_time = start_time_;
	ODstops* odstop = new ODstops (origin_stop, destination_stop);
	set_ODstop (odstop);
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