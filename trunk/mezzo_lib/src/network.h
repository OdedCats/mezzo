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

#ifndef NETWORK_HH
#define NETWORK_HH

//#define _DEBUG_NETWORK
//#define _DEBUG_SP // shortest path routines
#define _USE_VAR_TIMES   //!< variable link travel times



// standard template library inclusions

#include <vector>
#include <fstream>
#include <string>


//inclusion of parameter constants
#include "parameters.h"


//inclusion of label-correcting shortest path algorithm
#include "Graph.h"


// Mezzo inclusions
#include "server.h"
#include "link.h"
#include "node.h"
#include "sdfunc.h"
#include "turning.h"
#include "route.h"
#include "od.h"
#include "icons.h"
#include "vtypes.h"
#include "linktimes.h"
#include "eventlist.h"
#include "trafficsignal.h"
#include "busline.h"

// inclusions for the GUI
#ifndef _NO_GUI
	#include <QPixmap>
	#include <qpixmap.h>
	#include <QMatrix>
#endif // _NO_GUI

//thread support
#include <QThread.h>

//include the PVM communicator (for hybrid with MITSIMLAB)
#ifdef _PVM
#include "pvm.h"
#endif // _PVM

// Or include the VISSIMCOM communicator
#ifdef _VISSIMCOM
#include "vissimcom.h"
#endif //_VISSIMCOM

using namespace std;
class TurnPenalty;
class Incident;

struct ODRate
{
public:
	ODVal odid;
	double rate;
};


struct ODSlice
{
public:
	const bool remove_rate(const ODVal& odid); // removes od_rate for given od_id.
	vector <ODRate> rates;	
};


class  ODMatrix
{
public:
	ODMatrix ();
	void add_slice(const double time, ODSlice* slice);
	void reset(Eventlist* eventlist,vector <ODpair*> * odpairs); // rebooks all the MatrixActions
	const vector <double> get_loadtimes(); // returns the loadtimes for each OD slice, slice 0 at t=0 has index [0], etc.
	const bool remove_rate(const ODVal& odid); // removes od_rates for given od_id for all slices
private:
	vector < pair <double,ODSlice*> > slices;	
};

class MatrixAction: public Action
{
public:
	MatrixAction(Eventlist* eventlist, double time, ODSlice* slice_, vector<ODpair*> *ods_);
	const bool execute(Eventlist* eventlist, const double time);
private:
	ODSlice* slice;
	vector <ODpair*> * ods;
};

/*! Network is the main simulation class, containing all the network objects 
 * such as Links, Nodes, ODpairs, etc. as well as all the main simulation, reading and writing functions
 * It is included in the GUI part (MainForm in canvas_qt4.h) that calls it's public functions to read in the input files, simulate
 * and write results.
 *
 * last modifications:
 * add map <int,Object*> containers for Nodes, Links, Routes, Origins, Destinations, Junctions for faster look-up.
 * modified find_alternatives_all (int lid, double penalty, Incident* incident) to use the routemap structure at links for finding alternative routes.
 * Wilco Burghout
 * last change: 2008-01-11
  
 */

class Network
{
public:
	Network();
	//Network(const Network& net) {}
	~Network();
	bool readmaster(string name); //!< reads the master file.
#ifndef _NO_GUI
	double executemaster(QPixmap * pm_, QMatrix * wm_); //!< starts the scenario, returns total running time
	const double get_scale() {return scale;} //!< returns the scale of the drawing
#endif //_NO_GUI
	double executemaster(); //!< without GUI
	int reset(); //!< resets the simulation to 0, clears all the state variables. returns runtime
	void end_of_simulation(double time); //!< finalise all the temp values into containers (linktimes)
	double step(double timestep); //!< executes one step of simulation, called by the gui, returns current value of time
	const double run_iterations (); // runs iterations until max_iter or until mal_rel_gap_threshold  is reached (see Parameters). Returns rel_gap
	const void run_route_iterations(); // outer loop around run_iterations to find iteratively the route choice set.
	const bool check_convergence(const int iter_, const double rel_gap_ltt_ ,const double rel_gap_rf_); // returns true if convergence criterium is reached or if max_iter is reached.
	bool writeall(unsigned int repl=0); //writes the output, appends replication number to output files
	bool readnetwork(string name); //!< reads the network and creates the appropriate structures
	bool init(); //!< creates eventlist and initializes first actions for all turnings at time 0 and starts the Communicator

	void complete_turnpenalties(); //!< checks the turnings and completes the list of turning penalties for the shortest path
	void remove_orphan_nodes (); //!< removes all nodes that have no links connected from the network.
	
	bool init_shortest_path(); //!< builds the shortest path graph
	vector<Link*> get_path(int destid); //!<gives the links on the shortest path to destid (from current rootlink)
	bool shortest_paths_all(); //!< calculates shortest paths and generates the routes
	bool find_alternatives_all (int lid, double penalty, Incident* incident); //!< finds the alternative paths 'without' link lid.
	//void delete_spurious_routes(); //!< deletes all routes that have no OD pair.
	void renum_routes (); //!< renumerates the routes, to keep a consecutive series after deletions & additions
	// bool run(int period); //!< RUNS the network for 'period' seconds OBSOLETE
	bool addroutes (int oid, int did, ODpair* odpair); //!< adds routes to an ODpair
	bool add_od_routes()	; //!< adds routes to all ODpairs
	bool readdemandfile(string name);  //!< reads the OD matrix and creates the ODpairs
	bool readlinktimes(string name); //!< reads historical link travel times
	bool set_freeflow_linktimes();
	bool copy_linktimes_out_in(); //!< copies output link travel times to input
	bool readpathfile(string name); //!< reads the routes
	bool readincidentfile(string name); //!< reads the file with the incident (for now only 1)
	bool writepathfile (string name); //!< appends the routes found to the route file
	bool writeoutput(string name); //!< writes detailed output, at this time theOD output!
	bool writesummary(string name); //!< writes the summary of the OD output
	bool writelinktimes(string name); //!<writes average link traversal times.
	bool writeheadways(string name); //!< writes the timestamps of vehicles entering a Virtual Link (i e Mitsim).
	bool writerouteflows (string name); //!< writes the route flows to a standard file
	//!<same format as historical times read by readlinktimes(string name)
	bool register_links();//!<registers the links at the origins and destinations
	void set_incident(int lid, int sid, bool blocked, double blocked_until); //!< sets the incident on link lid (less capacity, lower max speed)
	void unset_incident(int lid); //!< restores the incident link to its normal behaviour
	void broadcast_incident_start(int lid); //!< informs the vehicles on the links of the incident on link lid
	void broadcast_incident_stop(int lid); //!< informs the vehicles that the incident on link lid has been cleared
	bool readturnings(string name); //!< reads the turning movements
	void create_turnings();          //!< creates the turning movements
	bool writeturnings(string name);  //!< writes the turing movements
	bool writemoes(string ending=""); //!< writes all the moes: speeds, inflows, outflows, queuelengths and densities per link
	bool open_convergence_file(string name); //!< opents  the convergence  file.
	void write_line_convergence(const int iteration, const double rgap_ltt, const double rgap_rf) { convergence_out << iteration << '\t' << rgap_ltt << '\t' << rgap_rf << endl;}
	void close_convergence_file(); //!< closes the convergence file
	ofstream & get_convergence_stream() { return convergence_out;}
	bool writeassmatrices(string name); //!< writes the assignment matrices
	bool write_v_queues(string name); //!< writes the virtual queue lengths

	bool readassignmentlinksfile(string name); //!< reads the file with the links for which the assignment matrix is collected

	bool readvtypes (string name); //!< reads the vehicles types with their lentghs and percentages.
	bool readvirtuallinks(string name); //!< reads the virtual links that connect boundary out nodes to boundary in nodes.
	bool readserverrates(string name); //!< reads in new rates for specified servers. This way server capacity can be variable over time for instance for exits.
	bool readsignalcontrols(string name); //!< reads the signal control settings
	void seed (long int seed_) {randseed=seed_; vehtypes.set_seed(seed_);}          //!< sets the random seed
	void removeRoute(Route* theroute);
	void reset_link_icons(); //!< makes sure all the link-icons are shown normally when the run button is pressed. This corrects the colours in case of an incident file (it colours show affected links)
		
	void set_output_moe_thickness ( unsigned int val ) ; // sets the output moe for the link thickness  for analysis
	void set_output_moe_colour ( unsigned int val ) ; // sets the output moe for the link colours for analysis

#ifndef _NO_GUI
	void recenter_image();   //!< sets the image in the center and adapts zoom to fit window
	QMatrix netgraphview_init(); //!< scale the network graph to the view initialized by pixmaps
	
	void redraw(); //!< redraws the image

#endif //_NO_GUI

	// GET's
	const double get_currenttime(){return time;}
	const double get_runtime(){return runtime;}
	const bool get_calc_paths() const {return calc_paths;}

	//const double get_time_alpha(){return time_alpha;}
	Parameters* get_parameters () {return theParameters;} 
	const vector <ODpair*>& get_odpairs () {return odpairs;} // keep as vector

	map <int, Origin*>& get_origins() {return originmap;}
	map <int, Destination*>& get_destinations() {return destinationmap;}
	map <int, Node*>& get_nodes() {return nodemap;}
	map <int,Link*>& get_links() {return linkmap;}

	const unsigned int get_nr_routes() { return routemap.size(); }
	
	multimap<ODVal, Route*>::iterator find_route (int id, ODVal val);
	bool exists_route (int id, ODVal val); // checks if route with this ID exists for OD pair val
	bool exists_same_route (Route* route); // checks if a similar route with the same links already exists
// STATISTICS
	//Linktimes
	const double calc_rel_gap_linktimes(); //!< calculates the relative gap for the output-input link travel times
	const double calc_abs_diff_input_output_linktimes(); //!< calculates the sum of the absolute differences in output-input link travel times
	const double calc_diff_input_output_linktimes (); //!< calculates the sum of the differences in output-input link travel times
	const double calc_sumsq_input_output_linktimes (); //!< calculates the sum square of the differences in output-input link travel times
	const double calc_rms_input_output_linktimes();//!< calculates the root mean square of the differences in output-input link travel times
	const double calc_rmsn_input_output_linktimes();//!< calculates the Normalized (by mean) root mean square differences in output-input link travel times
	const double calc_mean_input_linktimes(); //!< calculates the mean of the input link travel times;
	//Routeflows
	const double calc_rel_gap_routeflows(); //!< calculates the relative gap for the output-input route flows
	const double calc_abs_diff_input_output_routeflows(); //!< calculates the sum of the absolute differences in output-input routeflows
	const double calc_sum_routeflows(); //!< calculates the sum of the routeflows, i.e. total vehicle departures in network
	const double calc_sum_prev_routeflows(); //!< calculates the sum of the previous iteration's routeflows, i.e. total vehicle departures in network for previous iteration

	// OD times
	const double calc_rms_input_output_odtimes();
	const double calc_mean_input_odtimes();
	const double calc_rmsn_input_output_odtimes();
	// SET's
	void set_workingdir (const string dir) {workingdir = dir;}
	const string get_workingdir () {return workingdir;}
	//void set_time_alpha(double val) {time_alpha=val;}

	// Public transport
	bool readbusroutes(string name); //!< reads the busroutes, similar to readroutes
	bool readbusroute(istream& in); //!< reads a busroute
	bool readbuslines(string name); //!< reads the busstops, buslines, and trips
	bool readbusstop (istream& in); //!< reads a busstop
	bool readbusline(istream& in); //!< reads a busline
	bool readbustrip(istream& in); //!< reads a trip
#ifndef _NO_GUI
	double get_width_x() {return width_x;} //!< returns image width in original coordinate system
	double get_height_y() {return height_y;} //!< ... height ...
	void set_background (string name) {if (drawing) drawing->set_background(name.c_str());}
#endif // _NO_GUI 

protected:
	//vector <Node*> nodes;
	map <int, Node*> nodemap; //!< 
	map <int,int> node_to_graphnode;
	map <int,int> graphnode_to_node;
	//vector <Origin*> origins;
	map <int, Origin*> originmap; //!< 
	//vector <Destination*> destinations;
	map <int, Destination*> destinationmap; //!< 
	//  vector <Junction*> junctions;
	map <int, Junction*> junctionmap; //!< 
	vector <BoundaryOut*> boundaryouts; // Remove Later...
	map <int, BoundaryOut*> boundaryoutmap; //!< 
	vector <BoundaryIn*> boundaryins; // Remove Later...
	map <int, BoundaryIn*> boundaryinmap; //!< 
	//  vector <Link*> links;
	map <int, Link*> linkmap; //!< 
	map <int,int> link_to_graphlink;
	map <int,int> graphlink_to_link;
	// vector <Sdfunc*> sdfuncs;
	map <int, Sdfunc*> sdfuncmap; //!< 
	//  vector <Turning*> turnings;
	map <int, Turning*> turningmap; //!< 
	// vector <Server*> servers;
	map <int, Server*> servermap; //!< 
	vector <ChangeRateAction*> changerateactions; //!<
//	vector <Route*> routes;	
	multimap <ODVal, Route*> routemap; //!< 
	vector <ODpair*> odpairs; //!< keep using OD pair vector for now, as map is too much of a hassle with two indices.
	// map <int, ODpair*> odpairmap; 
	vector <Incident*> incidents;
	vector <VirtualLink*> virtuallinks;
	map <int, VirtualLink*> virtuallinkmap; //!< 
	// Turning penalties
	vector <TurnPenalty*> turnpenalties;
	// Vehicle types
	Vtypes vehtypes;

	vector <double> incident_parameters; // yes this is very ugly, as is the web of functions added, but I'll take them out asap
	vector <Stage*> stages;
	vector <SignalPlan*> signalplans;
	vector <SignalControl*> signalcontrols;
	// Public Transport
	vector <Busline*> buslines; //!< the buslines that generate bus trips on busroutes, serving busstops
	vector <Bustrip*> bustrips;  //!< the trips list of all buses
	vector <Busstop*> busstops; //!< stops on the buslines
	vector <Busroute*> busroutes; //!< the routes that buses follow
	//Shortest path graph
#ifndef _USE_VAR_TIMES
	Graph<double, GraphNoInfo<double> > * graph;
#else
	Graph<double, LinkTimeInfo > * graph;
#endif
	// Random stream
	Random* random;

	//GUI
#ifndef _NO_GUI
	Drawing* drawing; //!< the place where all the Icons live
	QPixmap* pm; //!< the place where the drawing is drawn
	QMatrix* wm; //!< worldmatrix that contains all the transformations of the drawing (scaling, translation, rotation, &c)
	QMatrix initview_wm; //!< world matrix that transform the drawing to the inital view
	double scale; //!< contains the scale of the drawing
	double width_x; //!< width of boundaries of drawing in original coordinate system
	double height_y; //!< height of boundaries of drawing in org. coord. sys.
#endif // _NO_GUI
	// Eventlist
	Eventlist* eventlist;
	// start of read functions
	bool readincidents(istream& in);
	bool readincident(istream & in);
	bool readincidentparams (istream &in);
	bool readincidentparam (istream &in);
	bool readx1 (istream &in);
	bool readtimes(istream& in);
	bool readtime(istream& in);
	bool readnodes(istream& in);
	bool readnode(istream& in);
	bool readsdfuncs(istream& in);
	bool readsdfunc(istream& in);
	bool readlinks(istream& in);
	bool readlink(istream& in);
	bool readservers(istream& in);
	bool readserver(istream& in);
	bool readturning(istream& in);
	bool readgiveway(istream& in);
	bool readgiveways(istream& in);
	bool readroutes(istream& in);
	bool readroute(istream& in);
	bool readods(istream& in);
	bool readod(istream& in, double scale=1.0);
	bool readvtype (istream & in);
	bool readvirtuallink(istream & in);
	bool readrates(istream & in);
	ODRate readrate(istream& in, double scale);
	bool readserverrate(istream& in);
	bool readsignalcontrol(istream & in);
	bool readsignalplan(istream& in, SignalControl* sc);
	bool readstage(istream& in, SignalPlan* sp);
	bool readparameters(string name);
	// end of read functions
	vector <string> filenames; //!< filenames for input/output as read in the master file
	string workingdir;
	unsigned int replication;
	int runtime; //!< == stoptime
	int starttime;
	int routenr; // id for next route to be created (> max route id)
	bool calc_paths; //!< if true new shortest paths are calculated and new paths added to the route file
	double time;
	int no_ass_links; //!< number of links observed in assignment matrix
	// Linktimes
	int nrperiods; //!< number of linktime periods
	double periodlength; //!< length of each period in seconds.
	LinkTimeInfo* linkinfo;
	ofstream convergence_out;
	// PVM communicator
#ifdef _PVM   
	PVM * communicator;
	int friend_id;
#endif // _NO_PVM 
#ifdef _VISSIMCOM
	VISSIMCOM * communicator;
	string vissimfile;
#endif // _VISSIMCOM
	// ODMATRIX
	ODMatrix odmatrix;
}; 
//end of network definition

//! Incident Class contains the methods needed for the simulation of incidents on a link.
//! It derives from Action, and the execute() creates four events:
//! 1. Set the incident on the affected link at the pre-specified time. This changes the speed-density function
//! 2. Start the broadcast of information to the affected links and origins
//! 3. End the incident on the affected link
//! 4. End the broadcast of the information.

class Incident: public Action
{
public:
	Incident (int lid_, int sid_, double start_, double stop_,double info_start_,double info_stop_, Eventlist* eventlist, Network* network_, bool blocked_);
	const bool execute(Eventlist* eventlist, const double time); //!< Creates the events needed for setting and ending the incident and information broadcast
	void broadcast_incident_start(int lid); //!< Broadcasts the incident to all the affected links and origins. At origins a flag will be set so all created vehicles will automatically switch, until notification that incident is over
	void broadcast_incident_stop(int lid); //!< Broadcasts the end of an incident to all Origins (Not needed for Links? Check...)

	void set_affected_links(map <int, Link*> & affected_links_) {affected_links=affected_links_;} //!< sets the links that are affected by the incident
	void set_affected_origins(map<int, Origin*> & affected_origins_) {affected_origins=affected_origins_;} //!< sets the origins that are affected by the incident
	void set_incident_parameters(vector <double> & incident_parameters_) {incident_parameters = incident_parameters_;} //!< sets the incident parameters that are used for the choices
#ifndef _NO_GUI
	void set_icon(IncidentIcon* icon_) {icon=icon_;}
	void set_visible(bool val) {icon->set_visible(val);}
#endif

private:
	map <int, Link*> affected_links; 
	map <int, Origin*> affected_origins;
	double start;
	double stop;
	double info_start;
	double info_stop;
	int lid; //!< Incident link
	int sid;
	Network* network;
	bool blocked;
	vector <double> incident_parameters; 
#ifndef _NO_GUI
	IncidentIcon* icon;
#endif
};

class TurnPenalty
{
public:
	int from_link;
	int to_link;
	double cost;
};

class NetworkThread: public QThread
{
public:

	NetworkThread (string masterfile,int threadnr = 1,long int seed=0):masterfile_(masterfile),threadnr_(threadnr),seed_(seed) 
		{
			theNetwork= new Network();
			if (seed_)
					theNetwork->seed(seed_);
		}
	void init () 
		{
			theNetwork->readmaster(masterfile_);
			runtime_=theNetwork->executemaster();
		}
	void run ()
	  {	
		  if (theNetwork->get_parameters()->max_route_iter > 1)
			  theNetwork->run_route_iterations();
		  else
		  {
			  if (theNetwork->get_parameters()->max_iter > 1)
				  theNetwork->run_iterations();
			  else
					theNetwork->step(runtime_);
		  }
	  }
	void iterate()
	{
		double result= theNetwork->run_iterations();
	}

	void saveresults (unsigned int replication = 0)
	  {
			theNetwork->writeall(replication);
	  }
	void reset ()
	{
		theNetwork->reset();
	}
	 ~NetworkThread () 
	  {
			delete theNetwork;
	  }
private:
	Network* theNetwork;
	long int seed_;
    string masterfile_;
	double runtime_;
	int threadnr_;

};


#endif
