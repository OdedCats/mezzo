#include "signal.h"

bool SignalControl::execute(Eventlist* eventlist, double time)
{
	if (!active)
	{
		current_signal_plan = signalplans.begin(); // start at the first stage when this plan becomes active
		assert (signalplans.size() > 0);
		active = true;
#ifdef _DEBUG_SIGNALS
		cout << "Signal control " << id << " activated. " << endl;
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

bool SignalPlan::execute(Eventlist* eventlist, double time)
{
	if (!active)
	{
		current_stage = stages.begin(); // start at the first stage when this plan becomes active
		active = true;
#ifdef _DEBUG_SIGNALS
		cout << "Signal plan " << id << " activated. Nr of stages " << stages.size() << endl;
#endif // _DEBUG_SIGNALS	

	}
	if (time < stop)
	{
#ifdef _DEBUG_SIGNALS
		cout << "Calling stage " << (*current_stage)->get_id() << endl;
#endif // _DEBUG_SIGNALS
		(*current_stage)->execute(eventlist,time); // activate stage. It will book an event for itself to determine when to stop.
		double next_stage_time= time + ((*current_stage)->get_duration());
		current_stage++; // next stage
		if (current_stage == stages.end())
		{
			current_stage=stages.begin(); // cycle round
		}
		// book the activation of the next stage
#ifdef _DEBUG_SIGNALS
		cout << "Next stage will be " << (*current_stage)->get_id() << " at time " << next_stage_time << endl;
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


bool Stage::execute(Eventlist* eventlist, double time)
{
	if (!active) // activate all turnings and rebook for stop time
	{
		for (vector<Turning*>::iterator iter = turnings.begin(); iter < turnings.end(); iter++)
		{	
			(*iter)->activate(eventlist,time);	
		}
		active = true;
#ifdef _DEBUG_SIGNALS
		cout << "Stage " << id << " activated. " << endl;
#endif // _DEBUG_SIGNALS	

	    eventlist->add_event(time+duration,this); // add the event to stop this stage
	}
	else // disactivate all turnings
	{
		stop();
#ifdef _DEBUG_SIGNALS
		cout << "Stage " << id << " stopped. " << endl;
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