#ifndef _PASSENGER
#define _PASSENGER

#include "parameters.h"
#include "busline.h"
#include "od_stops.h"

class Busstop;
class ODstops;

class Passenger
{
public:
	Passenger ();
	~Passenger ();
	Passenger (double start_time);
	
	void init (double start_time_, Busstop* origin_stop, Busstop* destination_stop);
	// Gets and sets:
	ODstops* get_OD_stop () {OD_stop;}
	void set_ODstop (ODstops* ODstop_) {OD_stop = ODstop_;}

protected:
	int passenger_id;
	double start_time;
	ODstops* OD_stop;
};

class PassengerRecycler
{
 public:
 	~PassengerRecycler();
	Passenger* newPassenger() {	 	if (pass_recycled.empty())
     								return new Passenger();
     								else
     								{
     									Passenger* pass=pass_recycled.front();
     									pass_recycled.pop_front();
     									return pass;
     								}	
     							}
							
     void addPassenger(Passenger* pass){pass_recycled.push_back(pass);}
 
private:
	list <Passenger*> pass_recycled;
};
//static PassengerRecycler recycler;
extern PassengerRecycler pass_recycler;

#endif //_Passenger