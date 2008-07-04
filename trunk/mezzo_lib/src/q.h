
#ifndef Q_HH
#define Q_HH
#include "vehicle.h"
#include "route.h"
#include <list>
#include "Random.h"
#include <algorithm>
#include "parameters.h"


 /*

   -NEW implementation sorts vehicles by their exit times when inserted
   - used list instead of vector due to linear find, constant insertion and
      retrieval times


 */

//#define _DEBUG_Q

class Route;
class Busroute;
class Vehicle;
class Link;

typedef pair <int , vector<Link*> > alternativetype   ;

typedef pair <double , Vehicle*> Veh_in_Q;


struct compare_time
{
 compare_time(double time_):time(time_) {}
 bool operator () (Veh_in_Q value)
 	{
 	 return (value.first>time);
 	}
 double time;
};


class Q
{
  public:

  Q(double maxcap_, double freeflowtime_);
  ~Q();
 // Accessors -> made inline so they are faster....
//  inline const bool full() {return (calc_total_space() >= maxcap);}           // for some reason this doesnt work. check later...
  inline const bool full() {return (vehicles.size() >= maxcap);}
  inline const bool empty(){return vehicles.empty();}
  inline const bool exit_ok() {return ok;}
  inline const int size() {return vehicles.size();}
  inline const int queue(double time) {return (size()-nr_running(time));}
  inline const int nr_running(double time) 	{list <Veh_in_Q> :: iterator iter=(find_if (vehicles.begin(),vehicles.end(), compare_time (time) ) ); 	
  																	return distance(iter,vehicles.end());}
  inline const double next () {return next_action;}
  inline void add_alternative(int dest, vector<Link*> route) {alternatives.insert(alternatives.begin(),alternativetype(dest,route));}
  inline void add_alternative_route (Route* route_) {routes.insert(routes.begin(),route_);}
  // entering and exiting vehicles, maybe they should be inline as well...
  bool enter_veh(Vehicle* veh);
  Vehicle* exit_veh(double time, Link* nextlink, int lookback);
  Vehicle* exit_veh(double time);
  void update_exit_times(double t0, Link* nextlink, int lookback, double v_exit, double v_shockwave);
  void broadcast_incident_start(int lid, vector <double> parameters);
  void receive_broadcast(Vehicle* veh, int lid, vector <double> parameters);
  void switchroute(Vehicle* veh, Route* curr_route, Route* alt_route, vector <double> parameters);
  double calc_space(double time);
  double calc_total_space();
  private:
   double maxcap;
   double freeflowtime;
   // help vars
  bool ok;   // used to indicate whether an exit_veh has been successful
  double next_action; // when the next turn action should be scheduled.
  							 // used when a vehicle hasn't arrived at the stopline yet.
  list <Veh_in_Q>::iterator viter; // iterator for the vehicles
   double ttime;
  int n, nextid,vnextid;
  Vehicle* vehicle;
  Veh_in_Q value;
  // end help vars
  list  <Veh_in_Q> vehicles;
  Random* random; // to produce random numbers for broadcast, route switch &c
  vector <alternativetype> alternatives; // alternative routes from this link to each destination.
  vector <Route*> routes; // alternative routes created from 'alternatives' when needed, used by the diverted vehicles.
};


#endif
