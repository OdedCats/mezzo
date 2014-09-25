#include "day2day.h"
#include <map>
#include <iostream>
#include <fstream>
#include <string>

enum k {EXP, PK, RTI, anticip, anticip_EXP};
enum l {e0, e1, crowding, e3, e4};
enum m {wt, ivt};

float &operator / (const Travel_time& lhs, const Travel_time& rhs)
{
	float quotient;
	if (rhs.tt[anticip] > 0)
		quotient = lhs.tt[anticip] / rhs.tt[anticip];
	else
		quotient = 1.0f;
	return quotient;
};

template <typename id_type>
map<id_type, Travel_time>& operator << (map<id_type, Travel_time>& ODSLreg, pair<const id_type, Travel_time>& row) //if existing ODSL is found, data is replaced else a new row is inserted
{
	map<id_type, Travel_time>::iterator odsl_sum = ODSLreg.find(row.first);
	if (odsl_sum != ODSLreg.end())
	{
		row.second.convergence = row.second / odsl_sum->second;
		odsl_sum->second = row.second;
	}
	else
		ODSLreg.insert(row);
	return ODSLreg;
};

template <typename id_type>
float insert (map<id_type, Travel_time>& ODSL_reg, map<id_type, Travel_time>& ODSL_data) //Method for inserting data for one day into record
{
	float crit = 0;
	for (map<id_type, Travel_time>::iterator row = ODSL_data.begin(); row != ODSL_data.end(); row++) //aggregate over days
	{
		row->second /= row->second.counter; //finish the averaging by dividing by the number of occurences which is counted when adding
				
		ODSL_reg << *row; //if existing ODSL is found, data is replaced else a new row is inserted

		crit += abs(row->second.convergence - 1); //for the break criterium
	}

	crit /= ODSL_data.size(); //to get the average

	return crit;
};

template <typename id_type>
map<id_type, Travel_time>& operator += (map<id_type, Travel_time>& ODSLreg, const pair<const id_type, Travel_time>& row) //if existing ODSL is found, data is added, else a new row is inserted
{
	map<id_type, Travel_time>::iterator odsl_sum = ODSLreg.find(row.first);
	if (odsl_sum != ODSLreg.end())
		odsl_sum->second += row.second;
	else
		ODSLreg.insert(row);
	return ODSLreg;
};

template <typename id_type>
map<id_type, Travel_time>& operator += (map<id_type, Travel_time>& ODSL_reg, map<id_type, Travel_time>& ODSL_data)
{
	for (map<id_type, Travel_time>::iterator row = ODSL_data.begin(); row != ODSL_data.end(); row++) //aggregate over replications
	{
		row->second /= row->second.counter; //finish the averaging by dividing by the number of occurences which is counted when adding
		row->second.counter = 1;

		ODSL_reg += *row; //if existing ODSL is found, data is added, else a new row is inserted
	}

	return ODSL_reg;
}

template <typename id_type>
void insert_alphas (const id_type& tt_odsl, Travel_time& tt, map<id_type, Travel_time>& tt_rec, const float alpha_base[], const int& day)
{
	//insert base values
	tt.tt[anticip_EXP] = tt.tt[PK];
	tt.alpha[0] = alpha_base[0];
	tt.alpha[1] = alpha_base[1];
	tt.alpha[2] = alpha_base[2];

	tt.counter = 1;
	tt.convergence = 2.0f;

	if (day != 1)
	{
		//load ODSLdep and replace base values when found
		map<id_type, Travel_time>::iterator odsl = tt_rec.find(tt_odsl);
		if (odsl != tt_rec.end())
		{
			tt.tt[anticip_EXP] = odsl->second.tt[anticip_EXP];
			tt.alpha[RTI] = odsl->second.alpha[RTI];
			tt.alpha[EXP] = odsl->second.alpha[EXP];
			tt.alpha[PK] = odsl->second.alpha[PK];
		}
	}
};

typedef map<ODSL, Travel_time>::iterator wt_map_iterator;
typedef map<ODSLL, Travel_time>::iterator ivt_map_iterator;

Day2day::Day2day (int nr_of_reps_)
{
	wt_alpha_base[RTI] = 0.5f;
	wt_alpha_base[EXP] = 0.0f;
	wt_alpha_base[PK] = 0.5f;

	ivt_alpha_base[EXP] = 0.0f;
	ivt_alpha_base[PK] = 1.0f;
	ivt_alpha_base[crowding] = 1.0f;

	v = 2.0f;

	nr_of_reps = nr_of_reps_;
	aggregate = false;

	if (theParameters->pass_day_to_day_indicator == 2)
		individual_wt = true;
	else
		individual_wt = false;

	if (theParameters->in_vehicle_d2d_indicator == 2)
		individual_ivt = true;
	else
		individual_ivt = false;
}

void Day2day::update_day (int d)
{
	kapa[RTI] = 1.0f/d;
	kapa[EXP] = 1.0f/d;
	kapa[PK] = 1.0f/d;
	kapa[anticip] = 1.0f/d;
	day = d;
	wt_day.clear();
	ivt_day.clear();
}

map<ODSL, Travel_time>& Day2day::process_wt_replication (vector<ODstops*>& odstops, map<ODSL, Travel_time> wt_rec)
{
	map<ODSL, Travel_time> wt_rep; //record of ODSL data for the current replication

	for (vector<ODstops*>::iterator od_iter = odstops.begin(); od_iter != odstops.end(); od_iter++)
	{
		map <Passenger*,list<Pass_waiting_experience>> pass_list = (*od_iter)->get_waiting_output();
		for (map<Passenger*,list<Pass_waiting_experience>>::iterator pass_iter1 = pass_list.begin(); pass_iter1 != pass_list.end(); pass_iter1++)
		{
			list<Pass_waiting_experience> waiting_experience_list = (*pass_iter1).second;
			for (list<Pass_waiting_experience>::iterator exp_iter = waiting_experience_list.begin(); exp_iter != waiting_experience_list.end(); exp_iter++)
			{
				Travel_time wt;

				int pid = exp_iter->pass_id;
				int orig = exp_iter->original_origin;
				int dest = exp_iter->destination_stop;
				int stop = exp_iter->stop_id;
				int line = exp_iter->line_id;
				wt.tt[PK] = exp_iter->expected_WT_PK;
				wt.tt[RTI] = exp_iter->projected_WT_RTI;
				wt.tt[EXP] = exp_iter->experienced_WT;

				if (aggregate)
				{
					pid = 0;
					orig = 0;
					dest = 0;
				}
				if (!individual_wt)
				{
					pid = 0;
				}

				const ODSL wt_odsl = {pid, orig, dest, stop, line};

				//insert base values and replace when previous experience found
				insert_alphas(wt_odsl, wt, wt_rec, wt_alpha_base, day);

				//calculate anticipated waiting time and add to experience for this replication
				calc_anticipated_wt(wt);
				pair<const ODSL, Travel_time> wt_row (wt_odsl, wt);
				wt_rep += wt_row; //if existing ODSL is found, data is added, else a new row is inserted
			}
		}
	}

	if (nr_of_reps > 1)
	{
		wt_day += wt_rep; //add repetition to day data
	}
	else
	{
		wt_day = wt_rep;
	}

	return wt_day;
}

map<ODSLL, Travel_time>& Day2day::process_ivt_replication (vector<ODstops*>& odstops, map<ODSLL, Travel_time> ivt_rec)
{
	map<ODSLL, Travel_time> ivt_rep; //record of ODSL data for the current replication

	for (vector<ODstops*>::iterator od_iter = odstops.begin(); od_iter != odstops.end(); od_iter++)
	{
		map <Passenger*,list<Pass_onboard_experience>> pass_list = (*od_iter)->get_onboard_output();
		for (map<Passenger*,list<Pass_onboard_experience>>::iterator pass_iter1 = pass_list.begin(); pass_iter1 != pass_list.end(); pass_iter1++)
		{
			list<Pass_onboard_experience> onboard_experience_list = (*pass_iter1).second;
			for (list<Pass_onboard_experience>::iterator exp_iter = onboard_experience_list.begin(); exp_iter != onboard_experience_list.end(); exp_iter++)
			{
				Travel_time ivt;

				int pid = exp_iter->pass_id;
				int orig = exp_iter->original_origin;
				int dest = exp_iter->destination_stop;
				int stop = exp_iter->stop_id;
				int line = exp_iter->line_id;
				int leg = exp_iter->leg_id;
				ivt.tt[PK] = exp_iter->expected_ivt;
				ivt.tt[EXP] = exp_iter->experienced_ivt.first;
				ivt.tt[crowding] = exp_iter->experienced_ivt.second;

				if (aggregate)
				{
					orig = 0;
					dest = 0;
				}
				if (!individual_ivt)
				{
					pid = 0;
				}
				const ODSLL ivt_odsl = {pid, orig, dest, stop, line, leg};

				//insert base values and replace when previous experience found
				insert_alphas(ivt_odsl, ivt, ivt_rec, ivt_alpha_base, day);

				//calculate anticipated waiting time and add to experience for this replication
				calc_anticipated_ivt(ivt);
				pair<const ODSLL, Travel_time> ivt_row (ivt_odsl, ivt);
				ivt_rep += ivt_row; //if existing ODSL is found, data is added, else a new row is inserted
			}
		}
	}

	if (nr_of_reps > 1)
	{
		ivt_day += ivt_rep; //add repetition to day data
	}
	else
	{
		ivt_day = ivt_rep;
	}

	return ivt_day;
}

void Day2day::calc_anticipated_wt (Travel_time& row)
{
	float& wtPK = row.tt[PK];
	float& wtRTI = row.tt[RTI];
	float& wtEXP = row.tt[EXP];
	float& awtEXP = row.tt[anticip_EXP];
	float& alphaRTI = row.alpha[RTI];
	float& alphaEXP = row.alpha[EXP];
	float& alphaPK = row.alpha[PK];
	float& awtG = row.tt[anticip];

	//calc awt
	if (wtEXP == 0) wtEXP = 1.0; //to avoid division by zero
	awtG = alphaRTI * wtRTI + alphaEXP * awtEXP + alphaPK * wtPK;
	float kapaAWT = 1 / (1 + day / pow(abs(awtEXP / wtEXP - 1) + 1, v));
	awtEXP = (1 - kapaAWT) * awtEXP + kapaAWT * wtEXP;

	//calc alphas
	alphaRTI = (1 - kapa[RTI]) * alphaRTI + kapa[RTI] / pow(abs(wtRTI / wtEXP - 1) + 1, v);
	alphaEXP = (1 - kapa[EXP]) * alphaEXP + kapa[EXP] / pow(abs(awtEXP / wtEXP - 1) + 1, v);
	alphaPK = (1 - kapa[PK]) * alphaPK + kapa[PK] / pow(abs(wtPK / wtEXP - 1) + 1, v);

	if (day == 1)
		alphaEXP = max(0.0, 1.0 - alphaPK- alphaRTI);

	//normalize alphas
	float fnorm = 1 / (alphaRTI + alphaEXP + alphaPK);
	alphaRTI = fnorm * alphaRTI;
	alphaEXP = fnorm * alphaEXP;
	alphaPK = 1 - alphaRTI - alphaEXP;
}

void Day2day::calc_anticipated_ivt (Travel_time& row)
{
	float& ivtPK = row.tt[PK];
	float& ivtEXP = row.tt[EXP];
	float& aivtEXP = row.tt[anticip_EXP];
	float& alphaEXP = row.alpha[EXP];
	float& alphaPK = row.alpha[PK];
	float& aivtG = row.tt[anticip];
	float& crowdingEXP = row.tt[crowding];
	float& acrowdingEXP = row.alpha[crowding];

	//calc aivt
	float kapaAWT = 1 / (1 + day / pow(abs(aivtEXP / ivtEXP - 1) + 1, v));
	aivtEXP = (1 - kapaAWT) * aivtEXP + kapaAWT * ivtEXP;
	float kapaCrowd = 1 / (1 + day / pow(abs(acrowdingEXP / crowdingEXP - 1) + 1, v));
	acrowdingEXP = (1 - kapaCrowd) * acrowdingEXP + kapaCrowd * crowdingEXP;

	//calc alphas
	alphaEXP = (1 - kapa[EXP]) * alphaEXP + kapa[EXP] / pow(abs(aivtEXP / ivtEXP - 1) + 1, v);
	alphaPK = (1 - kapa[PK]) * alphaPK + kapa[PK] / pow(abs(ivtPK / ivtEXP - 1) + 1, v);

	if (day == 1)
		alphaEXP = max(0.0, 1.0 - alphaPK);

	//normalize alphas
	float fnorm = 1 / (alphaEXP + alphaPK);
	alphaEXP = fnorm * alphaEXP;
	alphaPK = 1 - alphaEXP;
	aivtG = acrowdingEXP *(alphaEXP * aivtEXP + alphaPK * ivtPK);
}

