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

class ODpair;
class Route;
class Link;
//class PVM;

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
	void set_entered() {entered=true;theParameters->veh_in_network++;}
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

class Bus : public Vehicle
{
public:
	Bus():Vehicle() {occupancy=0;}
	Bus(int id_, int type_, double length_,Route* route_, ODpair* odpair_, double time_) :
	Vehicle(id_, type_,length_,route_,odpair_,time_) {}

	const int get_occupancy() {return occupancy;}
	void set_occupancy (const int occup) {occupancy=occup;}
	
protected:
	int occupancy;

};


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

 private:
	list <Vehicle*> recycled;
	list <Bus*> recycled_busses;
};

//static VehicleRecycler recycler;
extern VehicleRecycler recycler;

		

#endif
