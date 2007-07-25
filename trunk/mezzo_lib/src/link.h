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



struct less_odval
// help function for assignment matrix
{
  bool operator()(const odval od1, const odval od2) const
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
	Link (int id_, Node* in_, Node* out_, int length_, int nr_lanes_, Sdfunc* sdfunc_);
	Link();
   virtual ~Link();
   // accessors, they are inline where possible, but inline keyword not necessary
   const int get_id () {return id;}
   const int get_out_node_id () ;
	const int get_in_node_id() ;
   const int get_length() {return length;}	
	const int get_nr_lanes() {return nr_lanes;}
	//const int Link::size();
	const int size();
	void set_hist_time(double time) {	hist_time=time;}
	void set_histtimes(LinkTime* ltime) {histtimes=ltime;
																			avgtimes->nrperiods=histtimes->nrperiods;
																			avgtimes->periodlength=histtimes->periodlength;
																			curr_period=0;
																			tmp_avg=0.0;
																			tmp_passed=0;}
	const double get_hist_time() {return hist_time;}
	double get_cost (double time) {
																if (histtimes)	
																	return histtimes->cost(time);
																else	
																	 return get_freeflow_time();
																	}
	const double get_freeflow_time() {return freeflowtime;}
  const double get_blocked() {return (blocked_until);}  // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
  const void set_blocked(double time) {blocked_until=time;}  // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
    virtual const bool full();
      virtual const bool full(double time);
  	const bool empty();
  	const bool exit_ok() {	return ok;}
  	const double next_action (double time);
#ifndef _NO_GUI   
  	void set_icon(LinkIcon* icon_) {icon=icon_; icon->set_pointers(&queue_percentage, &running_percentage);}
#endif // _NO_GUI                  
	double get_nr_passed() {return nr_passed;}
	
	LinkTime* get_histtimes () {if (histtimes)
															return histtimes;
														else
															return NULL;}
	LinkTime* get_avgtimes () {return avgtimes;}
  	void add_alternative(int dest, vector<Link*> route) ;
  	void receive_broadcast(Vehicle* veh, int lid, vector <double> parameters) ;
  	// methods  	
  	virtual bool enter_veh(Vehicle* veh, double time);
  	virtual Vehicle* exit_veh(double time, Link* nextlink, int lookback);
    void update_exit_times(double time,Link* nextlink, int lookback);
  	virtual Vehicle* exit_veh(double time);
   	virtual const double density();
   	const double density_running(double time);
   	const double density_running_only(double time);
    virtual double speed_density(double density_);

    double speed(double time);
   bool write(ostream& out);
	void write_time(ostream& out);	
   void update_icon(double time);
   void set_incident(Sdfunc* sdptr, bool blocked_);
   void unset_incident();
   void broadcast_incident_start(int lid, vector <double> parameters);
   void write_speeds(ostream & out ) {out << id << "\t" ; moe_speed->write_values(out);}
   void write_speed(ostream & out, int index ) {moe_speed->write_value(out,index);}
   void write_inflows(ostream & out ) {out << id << "\t" ; moe_inflow->write_values(out);}
   void write_inflow(ostream & out, int index ) {moe_inflow->write_value(out,index);}
   void write_outflows(ostream & out ) {out << id << "\t" ; moe_outflow->write_values(out);}
   void write_outflow(ostream & out, int index ) {moe_outflow->write_value(out,index);}
   void write_queues(ostream & out ) {out << id << "\t" ; moe_queue->write_values(out);}
   void write_queue(ostream & out, int index ) {moe_queue->write_value(out,index);}
   void write_densities(ostream & out ) {out << id << "\t" ; moe_density->write_values(out);}
   void write_density(ostream & out, int index ) {moe_density->write_value(out,index);}
   int max_moe_size() {return _MAX(moe_speed->get_size(), _MAX (moe_inflow->get_size(),_MAX(moe_outflow->get_size(),_MAX(moe_queue->get_size(),moe_density->get_size()))));}
   void add_blocked_exit() {nr_exits_blocked++;}
   void remove_blocked_exit() {nr_exits_blocked--;}
   
   void set_use_ass_matrix(const bool value)  {use_ass_matrix=value; /*set_selected(value);*/}
   void write_ass_matrix (ostream & out, int linkflowperiod); // writes the Assignment matrix for this link and given linkflow period
   void set_selected (const bool sel) ;
   bool get_selected () {return selected;}

   // convergence measure
   double calc_diff_input_output_linktimes ();
   double calc_sumsq_input_output_linktimes ();
  
#ifdef _VISSIMCOM
   long parkinglot;
	long pathid;
	long lastlink;
#endif //_VISSIMCOM


  protected:
   int id;
  	Node* in_node;
  	Node* out_node;
  	Q* queue;
  	Sdfunc* temp_sdfunc;
  	int length; // length of the link in meters
  	int nr_lanes; // nr of lanes in the link
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
	map < int , map <odval, map <int,int>,less_odval > > ass_matrix; // assignment matrix which is indexed as follows:
			// ass_matrix [linkflow_period] [od_pair] [od_period]
			// it is accessed by the following iterator, which has to be set to the correct linkflow_period
	map <odval, map <int,int>, less_odval > ::iterator ass_iter; // used to write the assignment matrix for given linkperiod
	bool use_ass_matrix; // boolean set to true if this link collects assignment matrix data
	bool selected; //true if link is 'selected'
};


// InputLink should only be used as the input queue for the Origin on which all the OD-pairs
// generate their traffic.
class InputLink : public Link
{
	public:
		InputLink(int id_, Origin* out_);
		~InputLink();
    	bool enter_veh(Vehicle* veh, double time);
    	
		Vehicle* exit_veh(double time, Link* link, int lookback);
	private:
};


class VirtualLink : public Link
{
	public:

    VirtualLink():Link() {blocked=false; linkdensity=0.0;}
    VirtualLink(int id_, Node* in_, Node* out_, int length_=1000, int nr_lanes_=1, Sdfunc* sdfunc_=NULL);
    bool enter_veh(Vehicle* veh, double time); // overloaded from link. Places vehicle on sendlist.
    bool exit_veh(Vehicle* veh, double time);   // overloaded from link. Reports travel time for virtual link.
	void block (int code) {	if (code<0)
													blocked=true;
											else
													blocked=false;}
  void set_density(const double density_) {linkdensity=density_;}
  void set_speed(const double speed_) {linkspeed=speed_;}
  const double density() {return linkdensity;}
  const bool full();
  const bool full(double time);
  double speed_density(double density_);
  void write_in_headways(ostream & out);
  void write_out_headways(ostream & out);
	~VirtualLink();

#ifdef _VISSIMCOM
	vector <long> get_v_path_ids() {return ids;} // gets the virtual path link ids for VISSIM
	void set_v_path_ids(vector<long> _ids) {ids=_ids;}
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
