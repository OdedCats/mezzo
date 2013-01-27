#include "emitmodel.h"
#include "Math.h"
#include <iostream>

/* 
	constructor of Emission
*/
Emission::Emission(int link_id, int t_stamp, int t_val): link_id_(link_id), time_stamp_(t_stamp),
	time_val_(t_val), CO_(0), FC_(0), NOx_(0), PM10_(0)
{}

/* 
	constructor of EmissionModel
*/
EmissionModel::EmissionModel(int n_inp, int t_update): n_invars_(n_inp), t_update_(t_update)
{}

/* 
	constructor of Average Velocity Model
*/
AvgVelocityModel::AvgVelocityModel(int t_update):EmissionModel(1,t_update)
{
	// the model is only based on averge link speed
	
}

/* 
	function to run Average Velocity Model for a link
*/
const Emission* AvgVelocityModel::run_link(int linkid, int time, vector<double> inputs)
{
	if (t_update_<=0) return 0;

	// calculate emission factor in g/km using the Euro II case
	double avgvel=inputs[0];
	double CO_fac=(60.526+0.152*avgvel-0.000168*avgvel*avgvel)/(1+3.499*avgvel-0.0252*avgvel*avgvel);
	double FC_fac=(346.79+2.726*avgvel+0.00428*avgvel*avgvel)/(1+0.2168*avgvel-0.00091*avgvel*avgvel);
	double NOx_fac=(0.2836-0.00869*avgvel+0.00011*avgvel*avgvel)/(1-0.0234*avgvel+0.00044*avgvel*avgvel);
	double PM10_fac=0.0866-0.00142*avgvel+1.06e-5*avgvel*avgvel;
	
	double dist=avgvel*t_update_/1000;
	Emission* cur_emit=new Emission(linkid, time, t_update_);
	cur_emit->set_CO(CO_fac*dist);
	cur_emit->set_CO(CO_fac*dist);
	cur_emit->set_CO(CO_fac*dist);
	cur_emit->set_CO(CO_fac*dist);

	return cur_emit;
}

/* 
	function run Average Velocity Model for a network
*/
const list<Emission*> AvgVelocityModel::run_net(int time, list<int> links, list<vector<double>> inputs)
{
	return list<Emission*>();
}

/* 
	constructor of Drive Cycle based Model
*/
DriveCycleModel::DriveCycleModel(int n_inp, int t_update): EmissionModel(n_inp,t_update)
{
  // need to be further developed	
}

/* 
	virtual function run of Drive Cycle based Model
*/
const Emission* DriveCycleModel::run_link(int linkid, int time, vector<double> inputs)
{

	return 0;
}

/* 
	function run Drive Cycle Model for a network
*/
const list<Emission*> DriveCycleModel::run_net(int time, list<int> links, list<vector<double>> inputs)
{
	return list<Emission*>();
}
