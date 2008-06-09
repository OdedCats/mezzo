#ifndef PARAMETERS_
#define PARAMETERS_
/*! Here go all the constants that are used as parameters, later they will be read from a file.
	There should be a global Parameters class, and a global var Parameters theParameters; and all use of parameters should then be 
	theParameters.randseed for example
	Also, this file should specify which modules are used and which ones not, instead of the current DEFINES
*/
#include <string>
#include <vector>
#include "Random.h"

#ifndef _NO_GUI
#include <qcolor.h>
#endif


// DEFINES

#define _DETERMINISTIC_OD_SERVERS
#define _DETERMINISTIC_ROUTE_CHOICE
#define _DETERMINISTIC_VTYPES
//#define _MULTINOMIAL_LOGIT
#define _UNSAFE // faster reading, but less checking of input data
//#define _BUSES

// GLOBAL VARIABLES
extern long int randseed; // random seed
extern int vid;     // global vehicle id nr.
extern double time_alpha;

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
const double dont_know_delay=0.114; // percentage of drivers that don't know delay


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
	bool draw_link_ids; //!< If true link ID's are shown
	int link_thickness; //!< Thickness with which links are drawn
	int node_thickness; //!< Thickness with which nodes are drawn	
	int node_radius; //!< Radius with which nodes are drawn
	int queue_thickness; //!< Thickness with which the Queue part of a Link is drawn
	int selected_thickness; //!< Thickness with which selected objects are drawn
	int text_size; //!< Size of text in the network image
	bool show_background; //!< If true background image is displayed behind network (if any image was loaded)
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

// #server_parameters
   bool od_servers_deterministic; //!< if true the time headways in OD servers are determiunistic, otherwise neg_exp
   double odserver_sigma; //!< Obsolete since OD servers now have Mu=Sigma  (neg exp). Was for use with previous combined normal-neg_exp servers

// #vehicle_parameters
   int standard_veh_length; //!< used to calculate a-priori capacity of links

// #route_parameters
   double update_interval_routes; //!< interval for re-calculating route costs based on time-dependent travel times.
   double mnl_theta; //!< parameter for Multinomial LOGIT route choice
   double kirchoff_alpha; //!< parameter for Kirchoff route choice
   bool delete_bad_routes; //!< deletes bad (long) routes as well as cleans up nr of routes for small od pairs.
   double max_rel_route_cost; //!< max relative cost of a route, compared to the minimum cost route, to be allowed in the choice set.
   double small_od_rate; //!< minimum od_rate for an od-pair to have more than 1 route

   // statevar:
   bool shortest_paths_initialised; //!< true if shortest paths have been initialised
// #mime_parameters: ONLY for use in HYBRID situations
   double mime_comm_step; //!< Communication step for MICRO-MESO information exchange
   int mime_min_queue_length; //!< min queue length for stopping / starting shockwaves
   double mime_queue_dis_speed; //!< queue dissipation speed if not calculated from flow & density data
// TODO: Implement the use of the following paramaters
   double vissim_step; //!< time step for the VISSIM micro model
   double sim_speed_factor; //!< REALTIME factor to keep the hybrid model running at a fixed speed
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

