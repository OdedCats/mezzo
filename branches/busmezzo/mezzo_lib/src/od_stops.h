#ifndef _ODSTOPS
#define _ODSTOPS

#include "parameters.h"
#include "od.h"
#include "busline.h"
#include "pass_route.h"
#include "passenger.h"
#include "Random.h"
#include "MMath.h" 

class Busline;
class Busstop;
class Bustrip;
class Passenger;
class Pass_path;
class ODzone;

typedef vector <Passenger*> passengers;

class Pass_boarding_decision // container object holding output data for passenger boarding decision
{
public:
	Pass_boarding_decision (int pass_id_, int original_origin_, int destination_stop_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_,
							double boarding_probability_, bool boarding_,double u_boarding_, double u_staying_):
							pass_id(pass_id_),original_origin(original_origin_),destination_stop(destination_stop_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),boarding_probability(boarding_probability_),boarding(boarding_),
							u_boarding(u_boarding_),u_staying(u_staying_) {}
	virtual ~Pass_boarding_decision(); //!< destructor
	void write (ostream& out) { out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t' << boarding_probability << '\t'
		<< boarding << '\t'<< u_boarding << '\t'<< u_staying <<'\t'<< endl; }
	void reset () { pass_id = 0; original_origin = 0; destination_stop = 0; line_id = 0; trip_id = 0; stop_id = 0; time = 0;
	generation_time = 0; boarding = 0; u_boarding = 0; u_staying = 0; }
	int pass_id;
	int original_origin;
	int destination_stop;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	double boarding_probability;
	bool boarding;
	double u_boarding;
	double u_staying;
};

class Pass_alighting_decision // container object holding output data for passenger alighting decision
{
public:
	Pass_alighting_decision (int pass_id_, int original_origin_, int destination_stop_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_, int chosen_alighting_stop_,
							map<Busstop*,pair<double,double>> alighting_MNL_):
							pass_id(pass_id_),original_origin(original_origin_),destination_stop(destination_stop_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),chosen_alighting_stop(chosen_alighting_stop_),alighting_MNL(alighting_MNL_) {}
	virtual ~Pass_alighting_decision(); //!< destructor
	void write (ostream& out);
	void reset () { pass_id = 0; original_origin = 0; destination_stop = 0; line_id = 0; trip_id = 0; stop_id = 0; time = 0;
	generation_time = 0; }
	int pass_id;
	int original_origin;
	int destination_stop;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	int chosen_alighting_stop;
	map<Busstop*,pair<double,double>> alighting_MNL;
};

class Pass_connection_decision // container object holding output data for passenger connection decision
{
public:
	Pass_connection_decision (int pass_id_, int original_origin_, int destination_stop_, int stop_id_, double time_, double generation_time_, int chosen_connection_stop_, map<Busstop*,pair<double,double>> connecting_MNL_):
							pass_id(pass_id_), original_origin(original_origin_), destination_stop(destination_stop_), stop_id(stop_id_),time(time_), generation_time(generation_time_), chosen_connection_stop(chosen_connection_stop_), connecting_MNL(connecting_MNL_) {}
	virtual ~Pass_connection_decision(); //!< destructor
	void write (ostream& out);
	void reset () { pass_id = 0; original_origin = 0; destination_stop = 0; stop_id = 0; time = 0;
	generation_time = 0; }
	int pass_id;
	int original_origin;
	int destination_stop;
	int stop_id;
	double time;
	double generation_time;
	int chosen_connection_stop;
	map<Busstop*,pair<double,double>> connecting_MNL;
};

class Pass_waiting_experience // container object holding output data for passenger waiting experience
{
public:
	Pass_waiting_experience (int pass_id_, int original_origin_, int destination_stop_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_,
							double expected_WT_PK_, bool RTI_level_available_ ,double projected_WT_RTI_, double experienced_WT_, double AWT_):
							pass_id(pass_id_),original_origin(original_origin_),destination_stop(destination_stop_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),expected_WT_PK(expected_WT_PK_),RTI_level_available(RTI_level_available_),projected_WT_RTI(projected_WT_RTI_),
							experienced_WT(experienced_WT_),AWT(AWT_) {}
	virtual ~Pass_waiting_experience(); //!< destructor
	void write (ostream& out) { out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t' << expected_WT_PK << '\t'
		<< RTI_level_available << '\t'<< projected_WT_RTI << '\t'<< experienced_WT <<'\t'<< AWT <<'\t'<< endl; }
	void reset () { pass_id = 0; original_origin = 0; destination_stop = 0; line_id = 0; trip_id = 0; stop_id = 0; time = 0;
	generation_time = 0; expected_WT_PK = 0; RTI_level_available = 0; projected_WT_RTI = 0; experienced_WT = 0; AWT = 0;}
	int pass_id;
	int original_origin;
	int destination_stop;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	double expected_WT_PK;
	bool RTI_level_available;
	double projected_WT_RTI;
	double experienced_WT;
	double AWT;
};

class Pass_onboard_experience
{
public:
	Pass_onboard_experience (int pass_id_, int original_origin_, int destination_stop_, int line_id_, int trip_id_, int stop_id_, int leg_id_, double expected_ivt_, pair<double, double> experienced_ivt_):
							pass_id(pass_id_),original_origin(original_origin_),destination_stop(destination_stop_),line_id(line_id_), trip_id(trip_id_), stop_id(stop_id_), 
							leg_id(leg_id_), expected_ivt(expected_ivt_), experienced_ivt(experienced_ivt_) {}
							void write (ostream& out) { out << pass_id << '\t' << original_origin << '\t' << destination_stop << '\t' << line_id << '\t'<< trip_id << '\t' << stop_id << '\t' << leg_id << '\t' << expected_ivt << '\t' << experienced_ivt.first << '\t' << experienced_ivt.second << endl;}
	int pass_id;
	int original_origin;
	int destination_stop;
	int line_id;
	int trip_id;
	int stop_id;
	int leg_id;
	double expected_ivt;
	pair<double, double> experienced_ivt;
	double crowding;
};

struct SLL
{
	Busstop* stop;
	Busline* line;
	Busstop* leg;

	bool operator < (const SLL& rhs) const
	{
		if (stop != rhs.stop)
			return stop < rhs.stop;
		else if (line != rhs.line)
			return line < rhs.line;
		else
			return leg < rhs.leg;
	}
};

class ODstops : public Action
{
public:
	ODstops ();
	ODstops (Busstop* origin_stop_, Busstop* destination_stop_);
	ODstops (Busstop* origin_stop_, Busstop* destination_stop_, double arrival_rate_);
	~ODstops ();
	void reset ();
	
	//Gets and Sets:
	Busstop* get_origin() {return origin_stop;}
	Busstop* get_destination () {return destination_stop;}
	void set_arrival_rate (double rate_) {arrival_rate= rate_;}
	vector <Pass_path*> get_path_set () {return path_set;}
	void set_path_set (vector <Pass_path*> path_set_) {path_set = path_set_;}
	bool check_path_set ();
	double get_arrivalrate () {return arrival_rate;}
	vector<Passenger*> get_waiting_passengers() {return waiting_passengers;}
	int get_min_transfers () {return min_transfers;}
	void set_waiting_passengers(passengers passengers_) {waiting_passengers = passengers_;}
	void set_origin (Busstop* origin_stop_) {origin_stop=origin_stop_;}
	void set_destination (Busstop* destination_stop_) {destination_stop=destination_stop_;}
	void set_min_transfers (int min_transfers_) {min_transfers = min_transfers_;}
	map <Passenger*,list<Pass_boarding_decision>> get_boarding_output () {return output_pass_boarding_decision;}
	map <Passenger*,list<Pass_alighting_decision>> get_alighting_output () {return output_pass_alighting_decision;}
	map <Passenger*,list<Pass_connection_decision>> get_connection_output () {return output_pass_connection_decision;}
	map <Passenger*,list<Pass_waiting_experience>> get_waiting_output () {return output_pass_waiting_experience;}
	map <Passenger*,list<Pass_onboard_experience>> get_onboard_output () {return output_pass_onboard_experience;}
	vector <Passenger*> get_passengers_during_simulation () {return passengers_during_simulation;}
	void add_pass_waiting (Passenger* add_pass) {waiting_passengers.push_back(add_pass);}
	
	// Passengers processes
	void book_next_passenger (double curr_time);
	bool execute(Eventlist* eventlist, double time);

	// Path set
	void add_paths (Pass_path* pass_path_) {path_set.push_back(pass_path_);}
	double calc_boarding_probability (Busline* arriving_bus, double time, Passenger* pass);
	double calc_binary_logit (double utility_i, double utility_j);
	double calc_multinomial_logit (double utility_i, double utility_sum);
	double calc_path_size_logit (map<Pass_path*,pair<bool,double>> set_utilities, double utliity_i, double utliity_j);
	map<Pass_path*,double> calc_path_size_factor_nr_stops (map<Pass_path*,double> cluster_set_utilities);
	double calc_path_size_factor_between_clusters (Pass_path* path, map<Pass_path*,double> cluster_probs);
	double calc_combined_set_utility_for_alighting (Passenger* pass, Bustrip* bus_on_board, double time); // the trip that the pass. is currently on-board when calc. utility from downstream stop
	double calc_combined_set_utility_for_alighting_zone (Passenger* pass, Bustrip* bus_on_board, double time); 
	double calc_combined_set_utility_for_connection (double walking_distance, double time, Passenger* pass);
	double calc_combined_set_utility_for_connection_zone (Passenger* pass, double walking_distance, double time);
	bool check_if_path_is_dominated (Pass_path* considered_path, vector<Pass_path*> arriving_paths);

	// output-related functions
	void record_passenger_boarding_decision (Passenger* pass, Bustrip* trip, double time, double boarding_probability, bool boarding_decision); //!< creates a log-file for boarding decision related info
	void record_passenger_alighting_decision (Passenger* pass, Bustrip* trip, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> alighting_MNL); // !< creates a log-file for alighting decision related info
	void record_passenger_connection_decision (Passenger* pass, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> connecting_MNL_);
	void record_waiting_experience (Passenger* pass, Bustrip* trip, double time, int level_of_rti_upon_decision, double projected_RTI, double AWT); // !< creates a log-file for a decision and related waiting time components
	void record_onboard_experience(Passenger* pass, Bustrip* trip, double time, Busstop* stop, pair<double,double> riding_coeff);
	void write_boarding_output(ostream & out, Passenger* pass);
	void write_alighting_output(ostream & out, Passenger* pass);
	void write_connection_output(ostream & out, Passenger* pass);
	void write_waiting_exp_output(ostream & out, Passenger* pass);
	void write_onboard_exp_output(ostream & out, Passenger* pass);
	void write_od_summary(ostream & out);
	void write_od_summary_without_paths (ostream & out);
	void calc_pass_measures ();

	//Day2Day
	void set_anticipated_waiting_time (Busstop* stop, Busline* line, double anticipated_WT);
	void set_anticipated_ivtt (Busstop* stop, Busline* line, Busstop* leg, double anticipated_IVTT);
	void set_alpha_RTI (Busstop* stop, Busline* line, double alpha); 
	void set_alpha_exp (Busstop* stop, Busline* line, double alpha); 
	void set_ivtt_alpha_exp (Busstop* stop, Busline* line, Busstop* leg, double alpha); 
	map<pair<Busstop*, Busline*>,double> get_anticipated_waiting_time () {return anticipated_waiting_time;}
	map<pair<Busstop*, Busline*>,double> get_alpha_RTI () {return alpha_RTI;}
	map<pair<Busstop*, Busline*>,double> get_alpha_exp () {return alpha_exp;}
	map<SLL, double> get_anticipated_ivtt () {return anticipated_ivtt;}
	map<SLL, double> get_ivtt_alpha_exp () {return ivtt_alpha_exp;}

protected:
	Busstop* origin_stop;
	Busstop* destination_stop;
	double arrival_rate; 
	passengers waiting_passengers; // a list of passengers with this OD that wait at the origin
	int min_transfers; // the minimum number of trnasfers possible for getting from O to D
	
	vector <Pass_path*> path_set;
	double boarding_utility;
	double staying_utility;

	// output structures and measures (all output stored by origin zone)
	map <Passenger*,list<Pass_boarding_decision>> output_pass_boarding_decision;
	map <Passenger*,list<Pass_alighting_decision>> output_pass_alighting_decision;
	map <Passenger*,list<Pass_connection_decision>> output_pass_connection_decision;
	map <Passenger*,list<Pass_waiting_experience>> output_pass_waiting_experience;
	map <Passenger*,list<Pass_onboard_experience>> output_pass_onboard_experience;
	vector <Passenger*> passengers_during_simulation;
	int nr_pass_completed;
	double avg_tt;
	double avg_nr_boardings;
	vector <pair<vector<Busstop*>, pair <int,double>>> paths_tt;

	Random* random;
	Eventlist* eventlist; //!< to book passenger generation events
	bool active; // indicator for non-initialization call

	// relevant only in case of day2day procedures
	map<pair<Busstop*, Busline*>,double> anticipated_waiting_time;
	map<pair<Busstop*, Busline*>,double> alpha_RTI;
	map<pair<Busstop*, Busline*>,double> alpha_exp;
	map<SLL,double> anticipated_ivtt;
	map<SLL,double> ivtt_alpha_exp;

};

class Pass_boarding_decision_zone // container object holding output data for passenger boarding decision
{
public:
	Pass_boarding_decision_zone (int pass_id_, int origin_zone_, int destination_zone_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_,
							double boarding_probability_, bool boarding_,double u_boarding_, double u_staying_):
							pass_id(pass_id_),origin_zone(origin_zone_),destination_zone(destination_zone_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),boarding_probability(boarding_probability_),boarding(boarding_),
							u_boarding(u_boarding_),u_staying(u_staying_) {}
	virtual ~Pass_boarding_decision_zone(); //!< destructor
	void write (ostream& out) { out << pass_id << '\t' << origin_zone << '\t' << destination_zone << '\t' << line_id << '\t'<< trip_id << '\t'<< stop_id<< '\t'<< time << '\t'<< generation_time << '\t' << boarding_probability << '\t'
		<< boarding << '\t'<< u_boarding << '\t'<< u_staying <<'\t'<< endl; }
	void reset () { pass_id = 0; origin_zone = 0; destination_zone = 0; line_id = 0; trip_id = 0; stop_id = 0; time = 0;
	generation_time = 0; boarding = 0; u_boarding = 0; u_staying = 0; }
	int pass_id;
	int origin_zone;
	int destination_zone;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	double boarding_probability;
	bool boarding;
	double u_boarding;
	double u_staying;
};

class Pass_alighting_decision_zone // container object holding output data for passenger alighting decision
{
public:
	Pass_alighting_decision_zone (int pass_id_, int origin_zone_, int destination_zone_, int line_id_, int trip_id_, int stop_id_, double time_,	double generation_time_, int chosen_alighting_stop_,
							map<Busstop*,pair<double,double>> alighting_MNL_):
							pass_id(pass_id_),origin_zone(origin_zone_),destination_zone(destination_zone_),line_id(line_id_),trip_id(trip_id_), stop_id(stop_id_),time(time_),generation_time(generation_time_),chosen_alighting_stop(chosen_alighting_stop_),alighting_MNL(alighting_MNL_) {}
	virtual ~Pass_alighting_decision_zone(); //!< destructor
	void write (ostream& out);
	void reset () { pass_id = 0; origin_zone = 0; destination_zone = 0; line_id = 0; trip_id = 0; stop_id = 0; time = 0;
	generation_time = 0; }
	int pass_id;
	int origin_zone;
	int destination_zone;
	int line_id;
	int trip_id;
	int stop_id;
	double time;
	double generation_time;
	int chosen_alighting_stop;
	map<Busstop*,pair<double,double>> alighting_MNL;
};

class ODzone : public Action
{
public:
	ODzone (int zone_id);
	~ODzone ();
	int get_id () {return id;}
	void reset (); 
	map <Busstop*,pair<double,double>> get_stop_distances() {return stops_distances;}
	void set_boarding_u (double boarding_utility_) {boarding_utility = boarding_utility_;}
	void set_staying_u (double staying_utility_) {staying_utility = staying_utility_;}
	
	// initialize
	void add_stop_distance (Busstop* stop, double mean_distance, double sd_distance);
	void add_arrival_rates (ODzone* d_zone, double arrival_rate); 

	// demand generation
	bool execute(Eventlist* eventlist, double curr_time);

	// output-related functions
	void record_passenger_boarding_decision_zone (Passenger* pass, Bustrip* trip, double time, double boarding_probability, bool boarding_decision); //!< creates a log-file for boarding decision related info
	void record_passenger_alighting_decision_zone (Passenger* pass, Bustrip* trip, double time, Busstop* chosen_alighting_stop, map<Busstop*,pair<double,double>> alighting_MNL); // !< creates a log-file for alighting decision related info
	void write_boarding_output_zone(ostream & out, Passenger* pass);
	void write_alighting_output_zone(ostream & out, Passenger* pass);
	map <Passenger*,list<Pass_boarding_decision_zone>> get_boarding_output_zone () {return output_pass_boarding_decision_zone;}
	map <Passenger*,list<Pass_alighting_decision_zone>> get_alighting_output_zone () {return output_pass_alighting_decision_zone;}

protected:
	int id;
	double boarding_utility;
	double staying_utility;
	map <Busstop*,pair<double,double>> stops_distances; // the mean and SD of the distances to busstops included in the zone
	map <ODzone*,double> arrival_rates; // hourly arrival rate from this origin zone to the specified destination zone 
	Random* random;
	Eventlist* eventlist; //!< to book passenger generation events
	bool active; // indicator for non-initialization call

	
	// output structures and measures
	map <Passenger*,list<Pass_boarding_decision_zone>> output_pass_boarding_decision_zone;
	map <Passenger*,list<Pass_alighting_decision_zone>> output_pass_alighting_decision_zone;
	vector <Passenger*> passengers_during_simulation;
	int nr_pass_completed;
	double avg_tt;
	double avg_nr_boardings;
	vector <pair<vector<Busstop*>, pair <int,double>>> paths_tt;
};

#endif // OD