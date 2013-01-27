/*
 Emitmodel class defines interfaces for emission models

*/

#ifndef EMITMODEL_HH
#define EMITMODEL_HH
#include <list>
#include <string>
#include <vector>
#include <map>
#include "parameters.h"
#include <iostream>

using namespace std;

class Emission
{
    public:
		Emission(int link_id, int t_stamp, int t_val);
		~Emission() {}
		
		const int get_time_stamp(){return time_stamp_;}
		const double CO(){return CO_;}
		const double FC(){return FC_;}
		const double NOX(){return NOx_;}
		const double PM10(){return PM10_;}

		void set_timestamp(int t_stamp){time_stamp_=t_stamp;}
		void set_CO(double co_val) {CO_=co_val;}
		void set_FC(double fc_val) {FC_=fc_val;}
		void set_NOx(double nox_val) {NOx_=nox_val;}
		void set_PM10(double pm10_val) {PM10_=pm10_val;}
		  
	private:

		int link_id_;
		int time_stamp_;
		int time_val_;
		double CO_;
		double NOx_;
		double FC_;
		double PM10_;
};

class EmissionModel 
{
    public:
		EmissionModel(int n_inp, int t_update);
		virtual ~EmissionModel();

		const int get_t_update(){return t_update_;}
		void set_t_update(const int t_value){t_update_=t_value;}

		// virtual functions
		virtual const Emission* run_link(int linkid, int time, vector<double> inputs);
		virtual const list<Emission*> run_net(int time, list<int> links, list<vector<double>> inputs);

	protected:
		int n_invars_; // the number of variables that the model depends on 
		int t_update_; //sec
};

class AvgVelocityModel : public EmissionModel
{
    public:
		AvgVelocityModel(int t_update); //only input is velocity
		~AvgVelocityModel() {}

		const Emission* run_link(int linkid, int time, vector<double> inputs);
		const list<Emission*> run_net(int time, list<int> links, list<vector<double>> inputs);

	private:
      
};

class DriveCycleModel : public EmissionModel
{
    public:
		DriveCycleModel(int n_inp, int t_update);
		~DriveCycleModel() {}

		const map<int, double> get_drive_cycle(){return drive_cycle_;}
		void set_drive_cycle(map<int, double> dc){drive_cycle_=dc;}

		const Emission* run_link(int linkid, int time, vector<double> inputs);
		const list<Emission*> run_net(int time, list<int> links, list<vector<double>> inputs);

	protected:
		map<int, double> drive_cycle_;

};


#endif
