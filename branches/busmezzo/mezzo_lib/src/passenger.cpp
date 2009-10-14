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
	for (vector <ODstops*>::iterator od_stop = origin_stop->origins.begin(); od_stop < origin_stop->origins.end(); od_stop++)
	{
		if ((*od_stop)->get_destination() == destination_stop)
		{
			set_ODstop (*(od_stop));
			break;
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