#include "trafficsignal.h"

const bool SignalControl::execute(Eventlist* eventlist, const double time)
{
	if (!active)
	{
		current_signal_plan = signalplans.begin(); // start at the first stage when this plan becomes active
		assert (signalplans.size() > 0);
		active = true;
#ifdef _DEBUG_SIGNALS
		eout << "Signal control " << id << " activated. " << endl;
#endif // _DEBUG_SIGNALS	

	}
	(*current_signal_plan)->execute(eventlist,time);
	current_signal_plan++;
	if (current_signal_plan < signalplans.end())
	{
		eventlist->add_event((*current_signal_plan)->get_start(), this);
	}
	return true;
}

void  SignalControl::reset()
{
	active = false;
	vector<SignalPlan*>::iterator sp_iter=signalplans.begin();
	for (sp_iter; sp_iter != signalplans.end(); sp_iter++)
	{
		(*sp_iter)->reset();
	}

}

//Signalplan functions
void SignalPlan::add_stage(Stage* stage_) 
{	
	stages.push_back(stage_);
	stage_->set_parent(this);
}

const bool SignalPlan::execute(Eventlist* eventlist, const double time)
{
	if (!active)
	{	
		// activate all the stages
		for (current_stage = stages.begin(); current_stage != stages.end(); current_stage++) // start at the first stage when this plan becomes active
		{
			double book_time=time+offset+(*current_stage)->get_start();
			eventlist->add_event(book_time,(*current_stage));
		}
		active = true;
#ifdef _DEBUG_SIGNALS
		eout << "Signal plan " << id << " activated. Nr of stages " << stages.size() << endl;
#endif // _DEBUG_SIGNALS	
		eventlist->add_event(stop,this);
	}
	else // at end of signal plan turn all stages to red
	{
		active = false;
		for (vector<Stage*>::iterator iter= stages.begin(); iter < stages.end(); iter++)
		{
			(*iter)->stop();
		}

	}
	
	return true;
}

/****************** OLD ******************
const bool SignalPlan::execute(Eventlist* eventlist, const double time)
{
	if (!active)
	{
		current_stage = stages.begin(); // start at the first stage when this plan becomes active
		next_stage = current_stage;
		next_stage++;
		if (next_stage == stages.end())
		{
			next_stage=stages.begin(); // cycle round
		}
		active = true;
#ifdef _DEBUG_SIGNALS
		eout << "Signal plan " << id << " activated. Nr of stages " << stages.size() << endl;
#endif // _DEBUG_SIGNALS	

	}
	if (time < stop)
	{
#ifdef _DEBUG_SIGNALS
		eout << "Calling stage " << (*current_stage)->get_id() << endl;
#endif // _DEBUG_SIGNALS
		(*current_stage)->execute(eventlist,time); // activate stage. It will book an event for itself to determine when to stop.
		double next_stage_time= time + ((*current_stage)->get_duration());
		// set the 'hold_green' attributes where appropriate.
		(*current_stage)->hold_turnings((*next_stage)->get_turnings());

		//advance to next stage
		current_stage=next_stage;
		next_stage++;
		if (next_stage == stages.end())
		{
			next_stage=stages.begin(); // cycle round
		}
		// book the activation of the next stage
#ifdef _DEBUG_SIGNALS
		eout << "Next stage will be " << (*current_stage)->get_id() << " at time " << next_stage_time << endl;
#endif // _DEBUG_SIGNALS

		eventlist->add_event(next_stage_time,this);
	}
	else // at end of signal plan turn all stages to red
	{
		active = false;
		for (vector<Stage*>::iterator iter= stages.begin(); iter < stages.end(); iter++)
		{
			(*iter)->stop();
		}

	}
	
	return true;
}
*********************/
void SignalPlan::reset()
{
	active = false;
	vector <Stage*>::iterator s_iter=stages.begin();
	for (s_iter;s_iter != stages.end(); s_iter++)
	{
		(*s_iter)->reset();
	}
}


const bool Stage::execute(Eventlist* eventlist, const double time)
{
	if (!active) // activate all turnings and rebook for stop time
	{
		for (vector<Turning*>::iterator iter = turnings.begin(); iter < turnings.end(); iter++)
		{	
			(*iter)->activate(eventlist,time);	
		}
		active = true;
#ifdef _DEBUG_SIGNALS
		eout << "Stage " << id << " activated. " << endl;
#endif // _DEBUG_SIGNALS	

	    eventlist->add_event(time+duration,this); // add the event to stop this stage
	}
	else // disactivate turnings
	{
		stop();
		double book_time=time+parent->get_cycletime()-duration;
		eventlist->add_event(book_time,this); // add the event to re-start this stage
#ifdef _DEBUG_SIGNALS
		eout << "Stage " << id << " stopped. " << endl;
#endif // _DEBUG_SIGNALS	

	}
	
	return true;
}

void Stage::stop()
{
	active=false;
	for (vector<Turning*>::iterator iter = turnings.begin(); iter < turnings.end(); iter++)
	  {	
			(*iter)->stop();
	  }
}

void Stage::hold_turnings( const vector<Turning*> & nextturnings)
{
	for (vector<Turning*>::iterator curr_stage_turn=turnings.begin(); curr_stage_turn != turnings.end(); curr_stage_turn++)
	{
		bool found = false;
		for (vector<Turning*>::const_iterator next_stage_turn=nextturnings.begin(); next_stage_turn!=nextturnings.end(); next_stage_turn++)
		{
			// if the same set hold to true otherwise 
			if ((*curr_stage_turn)->get_id() == (*next_stage_turn)->get_id())
			{
				found = true;
			}
		}
		(*curr_stage_turn)->set_hold_green(found); // if found
			
	}
	
}