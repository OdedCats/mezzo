
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

class Vehicle
{
  public:
   Vehicle();
   Vehicle(int id_, int type_, double length_,Route* route_, ODpair* odpair_, double time_);
   void init (int id_, int type_, double length_, Route* route_, ODpair* odpair_, double time_);
   const double get_length(){return length;}
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
	Bustype (int type_id_, double length_, int number_seats_, int capacity_);
	~Bustype ();
	double get_length () {return length;}
	int get_number_seats () {return number_seats;}
	int get_capacity () {return capacity;}
	int get_id () {return type_id;}
protected:
	int type_id;	// bus type id
	double length;
	int number_seats;
	int capacity;
};

class Bus : public Vehicle
{
public:
	Bus():Vehicle(), trip(NULL) 
	{
		occupancy=0;
		active = false;
		number_seats = 50;
		capacity = 80;
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
	Vehicle(id_, type_,length_,route_,odpair_,time_), trip(NULL)
	{	occupancy = 0;
		active = false;
		number_seats = 50;
		capacity = 80;
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
	{	id = bv_id_;
		occupancy = 0;
		active = false;
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
// GETS and SETS
	const int get_occupancy() {return occupancy;}
	void set_occupancy (const int occup) {occupancy=occup;}
	Bustrip* get_bustrip () {return trip;}
	void set_bustrip ( Bustrip* trip_) {trip=trip_;}
	int get_number_seats () {return number_seats;}
	int get_capacity () {return capacity;}
	int get_active () {return active;}
	void set_active (bool active_) {active=active_;}
	int get_at_stop() { return at_stop; }
	void set_at_stop ( bool val) { at_stop = val;}

	double calc_departure_time (double time); // calculates departure time from origin according to arrival time and schedule (including layover effect)
	void advance_next_trip (); // Returns true if progressed trip-pointer and false if the roster is done.
//	void write_buses_generation (string name); // Not needed anymore
	void add_trips (vector <Start_trip*>  st) {driving_roster = st; next_trip = driving_roster.begin();}
	vector <Start_trip*> driving_roster; // trips assignment for each bus vehicle.
	vector <Start_trip*> :: iterator& get_next_trip() {return next_trip;} // returns pointer to next trip
protected:
	Random* random;
	int number_seats; // Two added variables for LOS satistics and for dwell time calculations
	int capacity; // In the future will be determined according to the bus type
	int occupancy;
	Bustrip* trip;
	bool active; // istrue when the bus started to serve trips (called set_curr_trip());	
	vector <Start_trip*>::iterator next_trip; // pointer to the current trip served by the bus vehicle
	bool at_stop;
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

