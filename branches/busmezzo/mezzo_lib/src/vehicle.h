/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef VEHICLE_HH
#define VEHICLE_HH
#include "route.h"
#include "od.h"
//#include "PVM.h"
#include "signature.h"
#include <list>
#include "vtypes.h"
#include "busline.h"
#include "Random.h"

class ODpair;
class Route;
class Link;
//class PVM;
class Bustype;
class Bustrip;
class Busstop;
class Busvehicle_location;
class Dwell_time_function;

class Vehicle
{
  public:
   Vehicle();
   virtual ~Vehicle(); //!< destructor
   Vehicle(int id_, int type_, double length_,Route* route_, ODpair* odpair_, double time_);
   void init (int id_, int type_, double length_, Route* route_, ODpair* odpair_, double time_);
   const double get_length(){return length;}
   void set_length (double length_) {length = length_;}
   const double get_exit_time(){return exit_time;}
   const double get_start_time(){return start_time;}
   const odval get_odids () ;
   void set_exit_time(double time){exit_time=time;}
   void set_entry_time(double time){entry_time=time;}
   void set_route (Route* route_) {route=route_; switched=1;}
   void set_switched(int i) {switched=i;}
   const double get_entry_time() {return entry_time;}
   void set_curr_link(Link* curr_link_);
   Link* get_curr_link();
   Route* get_route() {return route;}
   Link* nextlink();
   const int get_id() {return id;}
   const int get_type() {return type;}
   int get_oid();
   int get_did();
	void set_entered() {entered=true;}
	void add_meters(int meters_) {meters+=meters_;}
	void set_meters(int meters_) {meters=meters_;}
	int get_meters () {return meters;}
	void report(double time);
  protected:
	int id;
	Route* route;
	ODpair * odpair;
	double start_time;
	int type;
	double length;
 	double entry_time;
	double exit_time; 	
	double arrival_time;
	Link* curr_link;
	bool entered;
	int switched;
	int meters;
};
#ifdef _BUSES
class Bustrip;
typedef pair<Bustrip*,double> Start_trip;

class Bustype 
{
public:
	Bustype ();
	Bustype (int type_id_, string bus_type_name, double length_, int number_seats_, int capacity_, Dwell_time_function* dwell_time_function_);
	~Bustype ();
	double get_length () {return length;}
	int get_number_seats () {return number_seats;}
	int get_capacity () {return capacity;}
	Dwell_time_function* get_dt_function() {return dwell_time_function;}
	int get_id () {return type_id;}
protected:
	int type_id;	// bus type id
	string bus_type_name;
	double length;
	int number_seats;
	int capacity;
	Dwell_time_function* dwell_time_function;
};

class Bus : public Vehicle
{
public:
	Bus():Vehicle() 
	{
		occupancy = 0;
		on_trip = false;
		number_seats = 45;
		capacity = 70;
		type = 4;
		random = new (Random);
		if (randseed != 0)
		{
				random->seed(randseed);
		}
		else
		{
				random->randomize();
		}		
	}
	Bus(int id_, int type_, double length_,Route* route_, ODpair* odpair_, double time_) :
	Vehicle(id_, type_,length_,route_,odpair_,time_)
	{	occupancy = 0;
		on_trip = false;
		number_seats = 50;
		capacity = 70;
		type = 4;
		random = new (Random);
		if (randseed != 0)
		{
				random->seed(randseed);
		}
		else
		{
				random->randomize();
		}		
	};	
	Bus (int bv_id_, Bustype* bty) 
	{	bus_id = bv_id_;
		type = 4;
		occupancy = 0;
		on_trip = false;
		length = bty->get_length();
		number_seats = bty->get_number_seats();
		capacity = bty->get_capacity();
		random = new (Random);
		if (randseed != 0)
		{
				random->seed(randseed);
		}
		else
		{
				random->randomize();
		}		
	};
	virtual ~Bus(); //!< destructor
	void reset ();
// GETS and SETS
	int get_bus_id () {return bus_id;}
	void set_bus_id (int bus_id_) {bus_id = bus_id_;}
	const int get_occupancy() {return occupancy;}
	void set_occupancy (const int occup) {occupancy=occup;}
	int get_number_seats () {return number_seats;}
	int get_capacity () {return capacity;}
	bool get_on_trip () {return on_trip;}
	void set_on_trip (bool on_trip_) {on_trip=on_trip_;}
	Bustype* get_bus_type () {return bus_type;}
	void set_curr_trip (Bustrip* curr_trip_) {curr_trip = curr_trip_;}
	Bustrip* get_curr_trip () {return curr_trip;}
	
// other functions:	
	void set_bustype_attributes (Bustype* bty); // change the fields that are determined by the bustype
	void advance_curr_trip (double time, Eventlist* eventlist); // progresses trip-pointer 

// output-related functions
	void record_busvehicle_location (Bustrip* trip,  Busstop* stop, double time);
	void write_output(ostream & out);

protected:
	int	bus_id;
	Bustype* bus_type;
	Random* random;
	int number_seats; // Two added variables for LOS satistics and for dwell time calculations
	int capacity; // In the future will be determined according to the bus type
	int occupancy;
	Bustrip* curr_trip;
	bool on_trip; // is true when bus is on a trip and false when waiting for the next trip
	list <Busvehicle_location> output_vehicle; //!< list of output data for buses visiting stops
};

class Busvehicle_location // container object holding output data for stop visits
{
public:
	Busvehicle_location (int line_id_,	int trip_id_, int stop_id_, int vehicle_id_, int link_id_, bool entering_stop_, double time_):
							line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_), vehicle_id(vehicle_id_), link_id(link_id_), entering_stop(entering_stop_), time (time_){}
	virtual ~Busvehicle_location(); //!< destructor
	void write (ostream& out) { out << line_id << '\t'<< trip_id << '\t'<< stop_id << '\t'<< vehicle_id << '\t'<<link_id << '\t'<< entering_stop << '\t'
		<< time  << '\t'	<< endl; }
	void reset () {line_id = 0 ; trip_id = 0; vehicle_id = 0; stop_id = 0; link_id = 0; entering_stop = 0; time = 0; }
	int line_id;
	int trip_id;
	int vehicle_id;
	int stop_id;
	int link_id;
	bool entering_stop;
	double time;
};

#endif// BUSES

class VehicleRecycler
{
 public:
 	~VehicleRecycler();
	Vehicle* newVehicle() {	 	if (recycled.empty())
     								return new Vehicle();
     							else
     							{
     								Vehicle* veh=recycled.front();
     								recycled.pop_front();
     								return veh;
     							}	
     						}
							
     void addVehicle(Vehicle* veh){recycled.push_back( veh);}
#ifdef _BUSES	 
	 Bus* newBus() {	 	if (recycled_busses.empty())
     								return new Bus();
     							else
     							{
     								Bus* bus=recycled_busses.front();
     								recycled_busses.pop_front();
     								return bus;
     							}	
     						}
							
     void addBus(Bus* bus){recycled_busses.push_back( bus);}
#endif
 private:
	list <Vehicle*> recycled;
#ifdef _BUSES
	list <Bus*> recycled_busses;
#endif
};

//static VehicleRecycler recycler;
extern VehicleRecycler recycler;

#endif

