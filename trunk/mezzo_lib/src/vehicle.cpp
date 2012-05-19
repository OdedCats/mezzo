#include "vehicle.h"

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
 //next_link_id = -1;
 }


Vehicle::Vehicle(const int id_, const int type_, const double length_, Route*const  route_, ODpair* const odpair_, const double time_): id(id_), route(route_), odpair(odpair_), start_time(time_) , type(type_), length(length_), exit_time(0.0)
{
	entered=false;	
	switched=0;
	meters=0;
	//next_link_id = -1;
	next_link_iter=route->firstlink_iter();
}

void Vehicle::init (const int id_, const int type_, const double length_, Route* const route_, ODpair* const odpair_, const double time_)
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
	 route->register_veh_departure(start_time);
	 //next_link_id = -1;
	 next_link_iter=route->firstlink_iter();
}

void Vehicle::set_entered()
{
	entered=true;
	theParameters->veh_in_network++;
	next_link_iter++;
	//next_link_id=(*next_link_iter)->get_id();
}

void Vehicle::advance_to_next_link(Link* const next)
{
	


}


void Vehicle::set_curr_link(Link* const  curr_link_)
{
	curr_link=curr_link_;
	if (entered)
	{
		next_link_iter++;
		//next_link_id=(*next_link_iter)->get_id();
	}
}

Link* const  Vehicle::get_curr_link() const 
{
	return curr_link;
}

Link* const  Vehicle::nextlink() const 
{
	if (entered)
		return (*next_link_iter);
		//return route->nextlink(curr_link);
	else
		return route->firstlink();
}

const ODVal Vehicle::get_odids () const 
{return odpair->odids();}

const int Vehicle::get_oid() const 
{
	ODVal oid=odpair->odids() ;
	return oid.first;	
}

const int Vehicle::get_did() const 
{
	ODVal did=odpair->odids() ;
	return did.second;	
}
 	

void Vehicle::report(const double time)
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
 	for (list <Vehicle*>::iterator iter=recycled.begin();iter!=recycled.end();++iter)
	{			
		delete (*iter); // calls automatically destructor
	}
	recycled.clear();
	for (list <Bus*>::iterator iter1=recycled_busses.begin();iter1!=recycled_busses.end();++iter1)
	{			
		delete (*iter1); // calls automatically destructor
	}
	recycled_busses.clear();
}