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

/*  LINK is one of the central components of MIME. It handles the entering and
	exiting of vehicles into and from the queue and the reporting of these events to the
	grid. it calls the sdfunction to find out the speed given a density, which in turn is
	obtained from the size of the queue by density().
	
	Reporting changed: only exits are logged
	
	There are three density functions:
	- Density() is calculated using all running + alla queuing vehicles over the whole
		length of the link
	- Density_running() is calculated using ONLY the running vehicles but over the
		WHOLE length of the link
	- Density_running_only is calculated using ONLY the running vehicles over ONLY
		the non-queue part of the link


*/

/******* Uncomment for debugging VISSIMCOM
#ifndef _VISSIMCOM
	#define _VISSIMCOM
	#define _MIME
#endif
*****************/

#ifndef LINK_HH
#define LINK_HH
#include "vehicle.h"
#include "node.h"
#include "sdfunc.h"
#include "q.h"
#include "grid.h"
#include <vector>
#include <map>
#ifndef _NO_GUI
	#include "icons.h"
#endif // _NO_GUI
#include "parameters.h"
#include "linktimes.h"



struct less_ODVal
// help function for assignment matrix
{
  bool operator()(const ODVal od1, const ODVal od2) const
  {
    if (od1.first < od2.first)
		return true;
	else
	{
		if ((od1.first==od2.first) && (od1.second < od2.second))
			return true;
		else
			return false;
	}
  }
};



//#define _DEBUG_LINK   // reports debug information to the stdout on the status of the link each time a vehicle enters or exits
                        // OPTIONS FOR CALCULATION OF DENSITY:
#define _RUNNING_ONLY   // _RUNNING_ONLY: the density that is used is calculated taken the vehicles in the running part only and
                        //                the length of the running part only
                       //   _RUNNNING   : the density is calculated taking the vehicles in the running part over the whole length of the link
                       // nothing       : density is calculated over all vehicles on the whole link 

//#define _USE_EXPECTED_DELAY  // an expected delay is added to the vehicle's earliest exit time. This exp delay is calculated from the recorded
                              // outflow in the last period and the queue length.

//#define _COLLECT_TRAVELTIMES
//#define _COLLECT_ALL // comment out if Collect Traveltimes is active

using namespace std;
class Node;
class Vehicle;



class Link
{
public:
	Link (int id_, Node* in_, Node* out_, int length_, double nr_lanes_, Sdfunc* sdfunc_);
	Link();
	virtual ~Link();
	void end_of_simulation(); // consolidates all temp values in their containers
	virtual void reset();  // resets the link for restart
	// accessors, they are inline where possible, but inline keyword not necessary
	const int get_id () const {return id;}
	const int get_out_node_id () const ;
	const int get_in_node_id() const ;
	const int get_length() const  {return length;}	
	const double get_nr_lanes() const {return nr_lanes;}
	const string get_name()const  {return name;}
	void set_name(const string name_) {name=name_;}
	const int size() const; 
	const pair<double,double> set_output_moe_thickness(const unsigned int val); // sets the output MOE for the link icon returns min/max
	const pair <double,double>  set_output_moe_colour(const unsigned int val); // sets the output MOE for the link icon returns min/max
	void set_hist_time(const double time) {	hist_time=time;}
	void set_histtimes(LinkTime* ltime) {
		histtimes=ltime;
		avgtimes = new LinkTime(*histtimes);
		curr_period=0;
		tmp_avg=0.0;
		tmp_passed=0;
		}
	const bool copy_linktimes_out_in(); //!< copies the output travel times to input (historical) travel times
	const double get_hist_time() const {return hist_time;}
	const LinkTime* get_hist_times() { return histtimes;} // returns histtimes.
	const double get_cost (const double time) const {
		if (histtimes)	
			return histtimes->cost(time);
		else	
			return get_freeflow_time();
		}
	const double get_freeflow_time() const {return freeflowtime;}
	const double get_blocked() const {return (blocked_until);}  // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
	const void set_blocked(const double time) {blocked_until=time;}  // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
	virtual const bool full() const;
	virtual const bool full(const double time);
	const bool empty() const;
	const bool exit_ok() const {	return ok;}
	const double next_action (const double time) const;
	const bool veh_exiting(const double time, const Link* const nextlink, const int lookback) const; 
	void update_icon(const double time);

#ifndef _NO_GUI   
	LinkIcon* const get_icon() const {return icon;} // returns pointer to icon, allows icon to be changed, but not the pointer
	void set_icon(LinkIcon* const icon_) {icon=icon_; icon->set_pointers(&queue_percentage, &running_percentage);} // sets icon.
	void set_selected_color(const QColor selcolor) {icon->set_selected_color(selcolor);}
	const QColor get_selected_color () const {return (icon->get_selected_color());}
#endif // _NO_GUI                  
	const double get_nr_passed() const {return nr_passed;}

	LinkTime* const get_histtimes () const {if (histtimes)
		return histtimes;
	else
		return NULL;}
	LinkTime* const get_avgtimes () const {return avgtimes;}
	//  Incident stuff
	void add_alternative(const int dest, const vector<Link*> & route) ; // old way for incidents, adds stubs for alternative routes from this link to destination
	void add_alternative_route( Route* const route) ; // adds a whole route as alternative.
	void register_route (Route* const route) ;// adds route to routemap at link
	const multimap <int,Route*> & get_routes() const {return routemap;}
	const vector <Route*> get_routes_to_dest(const int dest) const ;// find all routes through this link leading to destination
	const unsigned int nr_alternative_routes(const int dest, const int incidentlink_id)  ; // returns number of alternative routes from this link to dest, avoiding incidentlink_id
	void receive_broadcast(Vehicle* const veh, const int lid, const vector <double> & parameters) const ;
	void set_incident(Sdfunc* const sdptr, const bool blocked_, const double blocked_until_);
	void unset_incident();
	void broadcast_incident_start(const int lid, const vector <double>& parameters) const;

	// general methods  for entering, exiting vehicles etc.
	virtual const bool enter_veh(Vehicle* const veh, const double time);
	virtual Vehicle* const exit_veh(const double time, Link* const  nextlink, const int lookback);
	void update_exit_times(const double time,Link*const  nextlink, const int lookback);
	virtual Vehicle* const exit_veh(const double time);
	virtual const double density() const ;
	const double density_running(const double time) const ;
	const double density_running_only(const double time) const ;
	virtual const double speed_density(const double density_) const ;
	const double speed(const double time) const ;
	// IO methods
	const bool write(ostream& out);
	void write_time(ostream& out);	
	void write_speeds(ostream & out, const int nrperiods ) {out << id << "\t" ; moe_speed->fill_missing(nrperiods,speed_density(0));
														moe_speed->write_values(out, nrperiods);}
	void write_speed(ostream & out, const int index ) {moe_speed->write_value(out,index);}
	void write_inflows(ostream & out, const int nrperiods) {out << id << "\t" ; moe_inflow->fill_missing (nrperiods, 0); 
														moe_inflow->write_values(out,nrperiods);}
	void write_inflow(ostream & out, const int index ) {moe_inflow->write_value(out,index);}
	void write_outflows(ostream & out,const int nrperiods ) {out << id << "\t" ; moe_outflow->fill_missing (nrperiods, 0);
														moe_outflow->write_values(out,nrperiods);}
	void write_outflow(ostream & out, const int index ) {moe_outflow->write_value(out,index);}
	void write_queues(ostream & out,const int nrperiods ) {out << id << "\t" ; moe_queue->fill_missing (nrperiods, 0);
														moe_queue->write_values(out,nrperiods);}
	void write_queue(ostream & out,const  int index ) {moe_queue->write_value(out,index);}
	void write_densities(ostream & out,const int nrperiods ) {out << id << "\t" ; moe_density->fill_missing (nrperiods, 0);
														moe_density->write_values(out,nrperiods);}
	void write_density(ostream & out, const int index ) {moe_density->write_value(out,index);}
	const int max_moe_size() const {return _MAX(moe_speed->get_size(), _MAX (moe_inflow->get_size(),_MAX(moe_outflow->get_size(),_MAX(moe_queue->get_size(),moe_density->get_size()))));}
	void add_blocked_exit() {nr_exits_blocked++;}
	void remove_blocked_exit() {nr_exits_blocked--;}

	void set_use_ass_matrix(const bool value)  {use_ass_matrix=value; /*set_selected(value);*/}
	void write_ass_matrix (ostream & out, const int linkflowperiod); // writes the Assignment matrix for this link and given linkflow period
	void set_selected (const bool sel) ;
	const bool get_selected () const {return selected;}

	// convergence measure
	const double calc_diff_input_output_linktimes () const;
	const double calc_sumsq_input_output_linktimes () const;

#ifdef _VISSIMCOM
	long parkinglot;// entering parkinglot
	long exitparkinglot; // exit parking lot
	long pathid;
	long lastlink; // last link in VISSIM before exiting (for adjustment of speed)
#endif //_VISSIMCOM


protected:
	int id;
	string name;
	Node* in_node;
	Node* out_node;
	Q* queue;
	Sdfunc* temp_sdfunc;
	int length; // length of the link in meters
	double nr_lanes; // nr of lanes in the link
	Sdfunc* sdfunc;
	Grid* grid;
#ifndef _NO_GUI	
	LinkIcon* icon;
#endif // _NO_GUI	
	double maxcap;
	bool ok; // to check if the exit_veh operation went allright	
	bool blocked; // set if the link is shut off during an incident
	double blocked_until; // set to -1 if not active, otherwise it has the time when the queue that blocks the link will reach the upstream node.
	int nr_exits_blocked; // set by the turning movements if they are blocked
	double avg_time; // average traversal time
	double tmp_avg; // temp var for average for period
	int tmp_passed; // temp var for nr passed for period
	int curr_period; // current period
	LinkTime* avgtimes;  // Time dependent avg travel times
	LinkTime* histtimes; // Time dependent historical travel times.
	double hist_time; // historical traversal time
	int nr_passed; // number of vehicles exited;
	double running_percentage, queue_percentage;     // percentage of vehicles in running part and queue
	double freeflowtime; // time it takes to traverse the link under freeflow
	MOE* moe_speed;
	MOE* moe_inflow;
	MOE* moe_outflow;
	MOE* moe_queue;
	MOE* moe_density;
	// Newly added for Assignment matrix
	map < int , map <ODVal, map <int,int>,less_ODVal > > ass_matrix; // assignment matrix which is indexed as follows:
	// ass_matrix [linkflow_period] [od_pair] [od_period]
	// it is accessed by the following iterator, which has to be set to the correct linkflow_period
	map <ODVal, map <int,int>, less_ODVal > ::iterator ass_iter; // used to write the assignment matrix for given linkperiod

// New 2008-01-30
	multimap <int, Route*> routemap; // map storing routes by Destination_id
	bool use_ass_matrix; // boolean set to true if this link collects assignment matrix data
	bool selected; //true if link is 'selected'
};


// InputLink should only be used as the input queue for the Origin on which all the OD-pairs
// generate their traffic.
class InputLink : public Link
{
	public:
		InputLink(const int id_, Origin* const  out_);
		~InputLink();
		virtual void reset();  // resets the link for restart

    	const bool enter_veh(Vehicle* const veh, const double time);
    	
		Vehicle* const  exit_veh(const double time, Link* const  link, const int lookback);
protected:
};


class VirtualLink : public Link
{
	public:

    VirtualLink():Link() {blocked=false; linkdensity=0.0;}
    VirtualLink(const int id_, Node*const  in_, Node* const out_, const int length_=1000, const double nr_lanes_=1.0, Sdfunc* const sdfunc_=NULL);
//	VirtualLink(int id_, Node* in_, Node* out_, int length_, int nr_lanes_, Sdfunc* sdfunc_);

    const bool enter_veh(Vehicle* const veh, const double time); // overloaded from link. Places vehicle on sendlist.
    const bool exit_veh(Vehicle* const veh, const double time);   // overloaded from link. Reports travel time for virtual link.
	void block (const int code){ blocked=(code<0 ? true:false);}
	void set_density(const double density_)  {linkdensity=density_;}
	void set_speed(const double speed_)  {linkspeed=speed_;}
	const double density() const {return linkdensity;}
	const bool full() const;
	const bool full(const double time);
	const double speed_density(const double density_) const ;
	void write_in_headways(ostream & out);
	void write_out_headways(ostream & out);
	~VirtualLink();

#ifdef _VISSIMCOM
	const vector <long> & get_v_path_ids() const {return ids;} // gets the virtual path link ids for VISSIM
	void set_v_path_ids(const vector<long> _ids) {ids=_ids;}
#endif //_VISSIMCOM

	protected:
	bool blocked;
    double linkdensity;  // updated by the 1st Microscopic segment the virtual link corresponds to
    double linkspeed; // updated by the 1st Micro seg.
  list <double> in_headways;
  list <double> out_headways;
  #ifdef _VISSIMCOM
	vector <long> ids;
#endif //_VISSIMCOM

};



#endif
