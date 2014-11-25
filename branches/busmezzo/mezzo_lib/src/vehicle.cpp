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

Vehicle::~Vehicle()
{}

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
 	/*
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
	*/

}

// ***** Special Bus Functions *****
void Bus::reset ()
{
	occupancy = 0;
	on_trip = true;
	type = 4;
	output_vehicle.clear();
}

Bus::~Bus()
{}

Busvehicle_location::~Busvehicle_location()
{}

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
			(*next_trip)->first->activate(time, line->get_busroute(), line->get_odpair(), eventlist);
		}
		// if the bus is early for the next trip, then it will be activated at the scheduled time from Busline
	}
}

void Bus::record_busvehicle_location (Bustrip* trip, Busstop* stop, double time)
{
	output_vehicle.push_back(Busvehicle_location(trip->get_line()->get_id(), trip->get_id() , stop->get_id(), bus_id , stop->get_link_id() , 1, time)); 
	output_vehicle.push_back(Busvehicle_location(trip->get_line()->get_id(), trip->get_id() , stop->get_id(), bus_id , stop->get_link_id() , 0, stop->get_exit_time())); 
}

void Bus::write_output(ostream & out)
{
	for (list <Busvehicle_location>::iterator iter = output_vehicle.begin(); iter!=output_vehicle.end();iter++)
	{
		iter->write(out);
	}
}

// ***** Bus-types functions *****
Bustype::Bustype ()
{
}

Bustype::Bustype (int type_id_, string bus_type_name_, double length_, int number_seats_, int capacity_, Dwell_time_function* dwell_time_function_):
	type_id(type_id_), bus_type_name(bus_type_name_), length(length_), number_seats(number_seats_), capacity(capacity_), dwell_time_function(dwell_time_function_)
{

}
Bustype::~Bustype ()
{
}

