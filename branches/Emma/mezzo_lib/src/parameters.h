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

#ifndef PARAMETERS_
#define PARAMETERS_
/*! Here go all the constants that are used as parameters, later they will be read from a file.
	There should be a global Parameters class, and a global var Parameters theParameters; and all use of parameters should then be 
	theParameters.randseed for example
	Also, this file should specify which modules are used and which ones not, instead of the current DEFINES
*/
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include "Random.h"

#ifndef _NO_GUI
#include <qcolor>
#endif


// DEFINES


#define _DETERMINISTIC_ROUTE_CHOICE
#define _DETERMINISTIC_VTYPES
//#define _MULTINOMIAL_LOGIT
#define _UNSAFE // faster reading, but less checking of input data


// GLOBAL VARIABLES

extern long int randseed; // random seed
extern int vid;     // global vehicle id nr.
//extern double time_alpha;
extern std::ofstream eout; // for all debugging output

const string version = "Mezzo 0.59.0";

// OLD Network.hh parameters

const double an_step=0.1;  // 10 times per second
const double speedup=50.0;
const double disturbance=0.10; // the disturbance in the shortest path link travel times.


// OLD Server.hh parameters
const double min_hdway=0.1; // minimum headway
const double cf_hdway=1.5;   // average carfolling headway
const double sd_cf_hdway=0.5; // sd of carfollowing headways (N(mu,sd))
const double max_cf_hdway=2.0; // minimum hdway for vehicles not in platoons
// Parameters for OD traffic generation
// const double odsig=0.5; // the standard sigma for bound traffic (N(1.5,0.5))


// sdfunc.hh parameters
const int min_speed=-1; //OLD PARAM to override the SD functions' min speeds, if Positive

// OLD q.hh parameters Determine Route-Changing behaviour. Needs to be changed with the route-switching module.
const double perc_receive_broadcast=0.7; // percentage of drivers that receive the INCIDENT broadcast.
const double dont_know_delay=0.114; // percentage of drivers that don't know (cannot estimate) delay on current route.


//OD parameters
//const double small_od_rate = 3.0; // determines when a OD can have more than 1 route

// PARAMETER CLASS
/*!  Here go all the global parameters used in the simulation and the GUI
*/
class Parameters
{	
public:
	Parameters () ;
	bool read_parameters (istream & in);
	void write_parameters(ostream & out);

// THE (PUBLIC) PARAMETERS
	
	// Drawing parameters
		

	
	// parameters for Output View
	int thickness_width; //!< multiplicator for thickness MOE value in output view.
	int cutoff; //!< cutoff percentage (0-100) in int format for which data labels are shown
	bool show_link_names;//!< if true show link names in output
	bool show_link_ids; //!< if true link ids are shown in output
	bool show_data_values; //!< if true data values are shown in output.
	// Simulation view
	bool draw_link_ids; //!< If true link ID's are shown in simulation view
	int link_thickness; //!< Thickness with which links are drawn
	int node_thickness; //!< Thickness with which nodes are drawn	
	int node_radius; //!< Radius with which nodes are drawn
	int queue_thickness; //!< Thickness with which the Queue part of a Link is drawn
	int selected_thickness; //!< Thickness with which selected objects are drawn
	int text_size; //!< Size of text in the network image
	bool show_background; //!< If true background image is displayed behind network (if any image was loaded)
	int background_x; //!< start_x for background image
	int background_y; //!< start_y 
	double background_scale; //!< scale for background image

#ifndef _NO_GUI
	QColor linkcolor; //!< Colour of links
	QColor nodecolor; //!< Colour of nodes
	QColor queuecolor; //!< Colour of queue part of links
	QColor backgroundcolor; //!< Background colour
	QColor selectedcolor; //!< Colour for selected objects
#endif
	double gui_update_step; //!< TODO: implement the gui update step!
	double zerotime; //!< Start time of simulation
	
// #moe_parameters
   double moe_speed_update; //!< update interval for link speed data
   double moe_inflow_update; //!<  update interval for link inflow data
   double moe_outflow_update; //!<  update interval for link outflow data
   double moe_queue_update; //!<  update interval for link queuelength data
   double moe_density_update; //!<  update interval for link density data
   double linktime_alpha; //!< smoothing factor for link travel times

// #assignment_matrix_parameters
   bool use_ass_matrix; //!< if true an assignment matrix is generated for all links in ass_links.dat
   double ass_link_period; //!< interval length for link periods in Ass. matrix
   double ass_od_period; //!< interval length for OD periods in ass. matrix

//#turning_parameters
   int default_lookback_size; //!< default queue look-back
   double turn_penalty_cost;  //!< added penalty in shortest path alg. if a turn is forbidden
   bool use_giveway; //!< if true, giveway logic is used
   double max_wait; //!< default max waiting time for give_way
// #server_parameters
   bool od_servers_deterministic; //!< if true the time headways in OD servers are determiunistic, otherwise neg_exp
   double odserver_sigma; //!< Obsolete since OD servers now have Mu=Sigma  (neg exp). Was for use with previous combined normal-neg_exp servers
 //TODO implement in file format  
   bool implicit_nr_servers; //!<if true the number of servers through a turning is calculated by MIN(nr incoming lanes , nr outgoing lanes), otherwise it is one per defined turning, but parallel turnings may be defined.

// #vehicle_parameters
   int standard_veh_length; //!< used to calculate a-priori capacity of links

// #route_parameters
   double update_interval_routes; //!< interval for re-calculating route costs based on time-dependent travel times.
   double mnl_theta; //!< parameter for Multinomial LOGIT route choice
   double kirchoff_alpha; //!< parameter for Kirchoff route choice
   bool delete_bad_routes; //!< deletes bad (long) routes as well as cleans up nr of routes for small od pairs.
   double max_rel_route_cost; //!< max relative cost of a route, compared to the minimum cost route, to be allowed in the choice set.
   double small_od_rate; //!< minimum od_rate for an od-pair to have more than 1 route
   // NEW, to be added to the parameters file format
   bool use_linktime_disturbances; //!< if true use linktime disturbances to generate multiple route variations for each search
   double linktime_disturbance; //!< random disturbance factor for generating multiple routes for single search.
   int routesearch_random_draws; //!< number of re-draws for the route searches
   bool scale_demand;//!< if true demand is scaled down for route searches with the factor below
   double scale_demand_factor; //!< scale factor for demand during route searches
   bool renum_routes;
   bool overwrite_histtimes; //!< if true the input (historical) link travel times are overwritten when output files are saved.


//#iteration_control
   int max_iter; //!< max number of iterations
   double rel_gap_threshold; //!< relative gap threshold. For the moment linktime rel_gap, will be route_flow_relgap
// TODO: to be implemented:
   int max_route_iter; //!< max number of route_search_iterations, runs as outer loop around the 'equilibrium' iterations !! NOT yet in file.
   

// #mime_parameters: ONLY for use in HYBRID situations
   double mime_comm_step; //!< Communication step for MICRO-MESO information exchange
   int mime_min_queue_length; //!< min queue length for stopping / starting shockwaves
   double mime_queue_dis_speed; //!< queue dissipation speed if not calculated from flow & density data
// TODO: Implement the use of the following parameters
   double vissim_step; //!< time step for the VISSIM micro model
   double sim_speed_factor; //!< REALTIME factor to keep the hybrid model running at a fixed speed

	
   // State variables
   bool shortest_paths_initialised; //!< true if shortest paths have been initialised
   int veh_in_network; //!< nr of vehicles in network at any moment
   vector <double> od_loadtimes; //!< contains the load times for all slices in the OD matrix

   	// variables for Output View
	unsigned int viewmode; //! Sets the view mode for the graphics: 0 = simulation run (standard), 1= show output data (such as flows etc)
	double max_thickness_value; //!< max MOE value for showing link thickness in output view. Regulates scaling of values to thickness
	double max_colour_value; //!< link colour indicating max MOE value in output view.
	double min_thickness_value; //!< min MOE value for showing link thickness in output view.
	double min_colour_value; //!< link colour indicating min MOE value in output view.
	bool inverse_colour_scale; //!< inverse colours for MIN and MAX MOE value in output view.
	double running_time; //!< total running time
	int show_period; //!< selects which time period to show in output view

	// Flags for input files. If true, these files should be read, otherwise they are created.
	bool read_turnings; 
	bool read_signals;
	bool read_histtimes; 
	bool read_routes;
	bool read_incidents;
	bool read_virtuallinks;
	bool read_serverrates;
	bool read_background;

	//Flags for output files. if true these values should be collected and written. TODO: implement selective collection of data.
	bool write_linktimes;
	bool write_output;
	bool write_summary;
	bool write_speeds;
	bool write_inflows;
	bool write_outflows;
	bool write_queuelengths;
	bool write_densities;
		






};


extern Parameters* theParameters ;


// Some fixes to deal with the way Microsoft VS deals with min and max. all occurrences of min and max should
// be replaced by the macros _MIN and _MAX, which will be converted to the appropriate functions
// given the compiler.
#ifndef _MIN
#define _MIN min
#endif // _MIN

#ifndef _MAX
#define _MAX max
#endif // _MAX

#endif

