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

// Special Bus Functions

double Bus::calc_departure_time (double time) // calculates departure time from origin according to arrival time and schedule (including layover effect)
{
	double min_layover = 2.00; 
	double mean_error_layover = 2.00;
	double std_error_layover = 1.00;
	// These three parameters should be used from the parameters input file

	double error_layover = random->lnrandom (mean_error_layover, std_error_layover); // error factor following log normal distribution
	double curr_departure = curr_trip->second;

	if (curr_trip == driving_roster.begin()) // if it is the first trip for this bus
	{
		return (curr_departure + random->nrandom_trunc (mean_error_layover, std_error_layover, 1.0));
			// first dispatching is subject to a normal truncated deviation (Vandebona & Richardson, 1986)
	}

	double departure_time = Max(curr_departure, time + min_layover);
	// If the scheduled time is after arrival+layover, it determines departure time. 
	// Otherwise (bus arrived behind schedule) - delay at origin.

	departure_time += error_layover; // Allways add the error factor
	return departure_time;// output note: departure time
}

void Bus::advance_curr_trip () 
{
	if (get_active() == false) // first time this function is called
	{
		curr_trip = driving_roster.begin();
		set_active (true);
	}
	else
	{
		if (curr_trip != driving_roster.end())
		{
			curr_trip++; // a trip was completed - points to the next trip on the schedule
		}
		else
		{
			set_active (false); // no more trips on the schedule for this bus
		}
	}
}

// not needed anymore
/*void Bus::write_buses_generation (string name)
{
	ofstream out(name.c_str());
	assert(out);
	out << "Bus ID: " << get_id() << " was created at: " << start_time << endl;  
}*/

// Bus-types functions
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