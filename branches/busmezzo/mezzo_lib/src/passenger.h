#ifndef _PASSENGER
#define _PASSENGER

#include "parameters.h"
#include "busline.h"
#include "od_stops.h"
#include "Random.h"

class Bustrip;
class Busline;
class Busstop;
class ODstops;

class Passenger
{
public:
	Passenger ();
	~Passenger ();
	void init (int pass_id, double start_time_, ODstops* OD_stop_);
	void reset();
	
	// Gets and sets:
	int get_id () {return passenger_id;}
	double get_start_time () {return start_time;}
	ODstops* get_OD_stop () {return OD_stop;}
	Busstop* get_original_origin () {return original_origin;}
	void set_ODstop (ODstops* ODstop_) {OD_stop = ODstop_;}

	// Passenger decision processes - currently the simplest case possible is assumed
	bool make_boarding_decision (Bustrip* arriving_bus, double time); // boarding decision making 
	Busstop* make_alighting_decision (Bustrip* boarding_bus, double time); // alighting decision making - currently: alight at your destination stop (assuming no transfers)

protected:
	int passenger_id;
	double start_time;
	Busstop* original_origin;
	ODstops* OD_stop;
	bool boarding_decision;
	Random* random;
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