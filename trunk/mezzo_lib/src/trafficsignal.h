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

#ifndef SIGNAL_H
#define SIGNAL_H

//#define _DEBUG_SIGNALS

#include "turning.h"
#include "eventlist.h"


class Stage;
class SignalPlan;

class SignalControl : public Action
{
public:
	SignalControl (int id_):id(id_) {active = false;}
	void add_signal_plan (SignalPlan* signalplan_) { signalplans.push_back(signalplan_);}
	virtual bool execute(Eventlist* eventlist, double time);
protected:
	vector<SignalPlan*> signalplans;
	vector<SignalPlan*>::iterator current_signal_plan;
	bool active;
	int id;
};

class SignalPlan : public Action
{
public:
	SignalPlan (int id_, double start_, double stop_, double cycletime_, double offset_)  :
	  id(id_), cycletime(cycletime_), offset(offset_), start(start_), stop(stop_)
			{active=false;}
	virtual bool execute(Eventlist* eventlist, double time);
	double get_start() {return start;}
	void add_stage(Stage* stage_) {stages.push_back(stage_);}
protected:
	int id;
	double cycletime;
	double offset;
	double start,stop;
	bool active; // true if current Signalplan is active
	vector <Stage*> stages;
	vector <Stage*> :: iterator current_stage; // keeps track of current stage

};

class Stage : public Action
{
public:
	Stage (int id_, double start_, double duration_): id(id_), start(start_), duration(duration_) { active = false;}
	int get_id() {return id;}
	double get_start() {return start;}
	double get_duration() {return duration;}
	void stop (); // stops the stage (turns it to red).
	void add_turning (Turning* turning) // add turning and stop it, since it is now regulated by the signal
		{turnings.push_back(turning); turning->stop();} 
	virtual bool execute(Eventlist* eventlist, double time); // starts and stops the turnings
protected:
	int id;
	double start,duration;
	bool active; // true if current Stage is active
	//double intergreen;
	vector <Turning*> turnings;
};

#endif // SIGNAL_H