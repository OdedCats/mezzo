#ifndef DAY2DAY_
#define DAY2DAY_

#include <map>
#include "od_stops.h"

using namespace std;



struct ODSL //structure for comparing ODSL combinations
{
	int pid;
	int orig;
	int dest;
	int stop;
	int line;

	bool operator == (const ODSL& rhs) const
	{
		return (pid == rhs.pid && orig == rhs.orig && dest == rhs.dest && stop == rhs.stop && line == rhs.line);
	}

	bool operator < (const ODSL& rhs) const
	{
		if (pid != rhs.pid)
			return pid < rhs.pid;
		else if (orig != rhs.orig)
			return orig < rhs.orig;
		else if (dest != rhs.dest)
			return dest < rhs.dest;
		else if (stop != rhs.stop)
			return stop < rhs.stop;
		else
			return line < rhs.line;
	}
} ;

struct ODSLL
{
	int pid;
	int orig;
	int dest;
	int stop;
	int line;
	int leg;

	bool operator == (const ODSLL& rhs) const
	{
		return (pid == rhs.pid && orig == rhs.orig && dest == rhs.dest && stop == rhs.stop && line == rhs.line && leg == rhs.leg);
	}

	bool operator < (const ODSLL& rhs) const
	{
		if (pid != rhs.pid)
			return pid < rhs.pid;
		else if (orig != rhs.orig)
			return orig < rhs.orig;
		else if (dest != rhs.dest)
			return dest < rhs.dest;
		else if (stop != rhs.stop)
			return stop < rhs.stop;
		else if (line != rhs.line)
			return line < rhs.line;
		else
			return leg < rhs.leg;
	}
} ;

struct Travel_time //structure for saving and adding data
{
	int counter;
	float tt[5];
	float alpha[3];
	float convergence;

	Travel_time &operator += (const Travel_time& rhs)
	{
		counter += rhs.counter;
		tt[0] += rhs.tt[0];
		tt[1] += rhs.tt[1];
		tt[2] += rhs.tt[2];
		tt[3] += rhs.tt[3];
		tt[4] += rhs.tt[4];
		alpha[0] += rhs.alpha[0];
		alpha[1] += rhs.alpha[1];
		alpha[2] += rhs.alpha[2];
		return *this;
	}

	Travel_time &operator /= (const int& rhs)
	{
		tt[0] /= rhs;
		tt[1] /= rhs;
		tt[2] /= rhs;
		tt[3] /= rhs;
		tt[4] /= rhs;
		alpha[0] /= rhs;
		alpha[1] /= rhs;
		alpha[2] /= rhs;
		return *this;
	}
} ;

template <typename id_type> float insert (map<id_type, Travel_time>& ODSL_reg, map<id_type, Travel_time>& ODSL_data); //Method for inserting data for one day into record
template float insert (map<ODSL, Travel_time>& ODSL_reg, map<ODSL, Travel_time>& ODSL_data);
template float insert (map<ODSLL, Travel_time>& ODSL_reg, map<ODSLL, Travel_time>& ODSL_data);

class Day2day
{
private:
	map<ODSL, Travel_time> wt_day; //record of ODSL data for the current day
	map<ODSLL, Travel_time> ivt_day; //record of ODSL data for the current day

	float wt_alpha_base[3];
	float ivt_alpha_base[3];

	float kapa[4];

	float v; //salience parameter
	float v_c; //crowding salience parameter
	float v1; //trust parameter
	float r; //recency parameter
	int nr_of_reps;
	int day;
	bool aggregate;
	bool individual_wt;
	bool individual_ivt;

	void calc_anticipated_wt (Travel_time& row);
	void calc_anticipated_ivt (Travel_time& row);

	int nr_of_passengers;
	int nr_of_changes;
	int nr_of_legs;
	double total_waiting_time;
	double total_wt_pk;
	double total_wt_rti;
	double total_wt_exp;
	double total_wt_anticip;
	int total_nr_missed;
	double total_in_vehicle_time;
	double total_ivt_pk;
	double total_ivt_exp;
	double total_ivt_anticip;
	double total_crowding;
	double total_acrowding;
	int nr_on_line_2;

public:
	Day2day (int nr_of_reps_);
	void set_salience(float v_) {v = v_;};
	void set_crowding_salience(float v_c_) {v_c = v_c_;};
	void set_trust(float v1_) {v1 = v1_;};
	void set_recency(float r_) {r = r_;};
	void update_day (int d);
	void write_output (string filename);
	map<ODSL, Travel_time>& process_wt_replication (vector<ODstops*>& odstops, map<ODSL, Travel_time> wt_rec);
	map<ODSLL, Travel_time>& process_ivt_replication (vector<ODstops*>& odstops, map<ODSLL, Travel_time> ivt_rec);
};

#endif