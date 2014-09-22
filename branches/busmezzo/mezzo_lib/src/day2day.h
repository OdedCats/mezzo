#ifndef DAY2DAY_
#define DAY2DAY_

#include <map>

using namespace std;

struct ODSL //structure for comparing ODSL combinations
{
	int orig;
	int dest;
	int stop;
	int line;

	bool operator == (const ODSL& rhs) const
	{
		return (orig == rhs.orig && dest == rhs.dest && stop == rhs.stop && line == rhs.line);
	}

	bool operator < (const ODSL& rhs) const
	{
		if (orig != rhs.orig)
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
	int orig;
	int dest;
	int stop;
	int line;
	int leg;

	bool operator == (const ODSLL& rhs) const
	{
		return (orig == rhs.orig && dest == rhs.dest && stop == rhs.stop && line == rhs.line && leg == rhs.leg);
	}

	bool operator < (const ODSLL& rhs) const
	{
		if (orig != rhs.orig)
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

class Day2day
{
private:
	map<ODSL, Travel_time> wt_rec; //the record of ODSL data
	map<ODSL, Travel_time> wt_day; //record of ODSL data for the current day

	map<ODSLL, Travel_time> ivt_rec; //the record of ODSL data
	map<ODSLL, Travel_time> ivt_day; //record of ODSL data for the current day

	float wt_alpha_base[3];
	float ivt_alpha_base[3];

	float kapa[4];

	float v;
	int nr_of_reps;
	int day;
	bool aggregate;
	bool individual;

	void calc_anticipated_wt (Travel_time& row);
	void calc_anticipated_ivt (Travel_time& row);

public:
	Day2day (int nr_of_reps_);
	void update_day (int d);
	float process_wt_output ();
	float process_ivt_output ();
	map<ODSL, Travel_time>& process_wt_replication ();
	map<ODSLL, Travel_time>& process_ivt_replication ();
};

#endif