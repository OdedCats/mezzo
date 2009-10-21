#ifndef _ODSTOPS
#define _ODSTOPS

#include "parameters.h"
#include "od.h"
#include "busline.h"
#include "pass_route.h"
#include "passenger.h"
#include "Random.h"
#include "MMath.h" 

class Busstop;
class Passenger;
class Pass_route;

typedef vector <Passenger*> passengers;

class ODstops : public Action
{
public:
	ODstops ();
	ODstops (Busstop* origin_stop_, Busstop* destination_stop_, double arrival_rate_);
	~ODstops ();
	
	//Gets and Sets:
	Busstop* get_origin() {return origin_stop;}
	Busstop* get_destination () {return destination_stop;}
	double get_arrivalrate () {return arrival_rate;}
	passengers get_waiting_passengers() {return waiting_passengers;}
	void set_waiting_passengers(passengers w_passengers) {waiting_passengers = w_passengers;}
	void set_origin (Busstop* origin_stop_) {origin_stop=origin_stop_;}
	void set_destination (Busstop* destination_stop_) {destination_stop=destination_stop_;}
	
	// Passengers processes
	void book_next_passenger (double curr_time);
	bool execute(Eventlist* eventlist, double time);

	// Path set - not operative yet
	vector <Pass_route*> paths;
	void add_paths (Pass_route* pass_route_) {paths.push_back(pass_route_);}

protected:
	Busstop* origin_stop;
	Busstop* destination_stop;
	double arrival_rate; 
	passengers waiting_passengers; // a list of passengers with this OD that wait at the origin
	Random* random;
	Eventlist* eventlist; //!< to book passenger generation events
	bool active; // indicator for non-initialization call
};
#endif //_OD_stops