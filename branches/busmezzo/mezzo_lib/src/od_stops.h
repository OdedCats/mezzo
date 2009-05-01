#ifndef _PASSENGER
#define _PASSENGER

#include "parameters.h"
#include "od.h
#include "busline.h"

class ODstops
{
public:
	ODstops ();
	ODstops (Busstop* origin_stop_, Busstop* destination_stop_);
	~ODstops ();
	
	//Gets and Sets:
	Busstop* get_origin() {return origin_stop;}
	Busstop* get_destination {return destination_stop;}
	void set_origin (Busstop* origin_stop_) {origin_stop=origin_stop_;}
	void set_destination (Busstop* destination_stop_) {destination_stop=destination_stop_;}
	vector <Pass_route*> paths;
	void add_paths (Pass_route* pass_route_) {paths.push_back(pass_route_);}

protected:
	Busstop* origin_stop;
	Busstop* destination_stop;
};
#endif //_OD_stops