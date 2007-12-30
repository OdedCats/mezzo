#include "vehicle.h"
#include "MMath.h"

Vehicle::Vehicle()
{
 id=-1;
 type=-1;
 route=NULL;
 odpair=NULL;
 start_time=0.0;
 length=0;
 exit_time=0.0;
 entered=false;
 switched=0;
 meters=0;
}


Vehicle::Vehicle(int id_, int type_, double length_, Route* route_, ODpair* odpair_, double time_): id(id_), route(route_), odpair(odpair_), start_time(time_) , type(type_), length(length_), exit_time(0.0)
{
	entered=false;	
	switched=0;
	meters=0;
}

void Vehicle::init (int id_, int type_, double length_, Route* route_, ODpair* odpair_, double time_)
{
 id=id_;
 type=type_;
 route=route_;
 odpair=odpair_;
 start_time=time_;
 length=length_;
 exit_time=0.0;
 entered=false;
 switched=0;
 meters=0;
}

void Vehicle::set_curr_link(Link* curr_link_)
{
	curr_link=curr_link_;
}

Link* Vehicle::get_curr_link()
{
	return curr_link;
}

Link* Vehicle::nextlink()
{
	if (entered)
		return route->nextlink(curr_link);
	else
		return route->firstlink();
}

const odval Vehicle::get_odids ()
{return odpair->odids();}

int Vehicle::get_oid()
{



	odval oid=odpair->odids() ;
	return oid.first;	
}

int Vehicle::get_did()
{
	odval did=odpair->odids() ;
	return did.second;	
}
 	

void Vehicle::report(double time)
{
  arrival_time=time;
  list <double> collector;
  collector.push_back(id);
  collector.push_back(start_time);
  collector.push_back(arrival_time);
  collector.push_back(arrival_time - start_time);
  collector.push_back(meters);
  collector.push_back(route->get_id());
  collector.push_back(switched);
  odpair->report(collector);
}

 // VehicleRecycler procedures

VehicleRecycler::	~VehicleRecycler()
{
 	for (list <Vehicle*>::iterator iter=recycled.begin();iter!=recycled.end();)
	{			
		delete (*iter); // calls automatically destructor
		iter=recycled.erase(iter);	
	}
	for (list <Bus*>::iterator iter1=recycled_busses.begin();iter1!=recycled_busses.end();)
	{			
		delete (*iter1); // calls automatically destructor
		iter1=recycled_busses.erase(iter1);	
	}

}

// ***** Special Bus Functions *****

void Bus::set_bustype_attributes (Bustype* bty) 
// change the fields that are determined by the bustype
{
	type = 4;
	length = bty->get_length();
	number_seats = bty->get_number_seats();
	capacity = bty->get_capacity();
	bus_type = bty;
}

void Bus::advance_curr_trip (double time, Eventlist* eventlist) // progresses trip-pointer 
{
	vector <Start_trip*>::iterator trip1, next_trip; // find the pointer to the current and next trip
	for (vector <Start_trip*>::iterator trip = curr_trip->driving_roster.begin(); trip < curr_trip->driving_roster.end(); trip++)
	{
		if ((*trip)->first == curr_trip)
		{
			trip1 = trip;
			break;
		}
	}
	next_trip = trip1+1;
	on_trip = false;  // the bus is avaliable for its next trip
	if (next_trip != curr_trip->driving_roster.end()) // there are more trips for this bus
	{
		if ((*next_trip)->first->get_starttime() <= time) // if the bus is already late for the next trip
		{
			Busline* line = (*next_trip)->first->get_line();
			// then the trip is activated
			(*next_trip)->first->activate((*next_trip)->first->calc_departure_time(time), line->get_busroute(), line->get_vtype(), line->get_odpair(), eventlist);
		}
		// if the bus is early for the next trip, then it will be activated at the scheduled time from Busline
	}
}


// ***** Bus-types functions *****
Bustype::Bustype ()
{
}

Bustype::Bustype (int type_id_, double length_, int number_seats_, int capacity_):
	type_id(type_id_), length(length_), number_seats(number_seats_), capacity(capacity_)
{

}
Bustype::~Bustype ()
{
}