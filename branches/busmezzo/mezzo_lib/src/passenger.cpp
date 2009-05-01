///! passenger.cpp: implementation of the passenger class.

Passenger::Passenger ()
{
}

Passenger::~Passenger()
{}

Passenger::Passenger (double start_time_)
{
	start_time = start_time_;
}

void Passenger::init (double start_time, Busstop* origin_stop, Busstop* destination_stop);
{
	start_time = start_time_;
	ODstop* odstop = new ODstop (origin_stop, destination_stop);
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