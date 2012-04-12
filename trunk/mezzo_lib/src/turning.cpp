#include "turning.h"
#include "network.h"



Turning::Turning(int id_, Node* node_, Server* server_, Link* inlink_, Link* outlink_, int size_):
	id(id_), node(node_), server(server_), inlink(inlink_), outlink(outlink_), size(size_)

  // one turnaction for MIN (nr_lanes_in, nr_lanes_out).

	{
		int nr_servers=1;
		if (theParameters->implicit_nr_servers)
			nr_servers = _MIN(outlink->get_nr_lanes(),inlink->get_nr_lanes());
		for (int i=0; i < nr_servers; i++)
			turnactions.push_back(new TurnAction(this));
		delay=server->get_delay();
#ifdef _DEBUG_TURNING	
		eout << "new turning: tid,nid,sid,in,out " << id << node->get_id();
		eout << server->get_id() << inlink->get_id() << outlink->get_id() << endl;
#endif //_DEBUG_TURNING
		waiting_since = 0.0;
		waiting = false;
		blocked=false;
		active = true; // turning is active by default
		out_full = false;
		hold_green=false;
		((Junction*) node) ->add_turning(this); // add to turnings in node
	}

Turning::~Turning()
{

 for (vector <TurnAction*>::iterator iter=turnactions.begin();iter<turnactions.end();)
	{
		delete (*iter); // calls automatically destructor
		iter=turnactions.erase(iter);
	}

}


void Turning::reset()
{
	blocked = false;
	active = true;
	ok = false;
	out_full = false;
}

bool Turning::check_links( const int in,  const int out)
{ 
	return ( (in == inlink->get_id()) && (out == outlink->get_id()));
}



bool Turning::process_veh(double time)

{
  ok=true;
  out_full=outlink->full(time);
  nexttime=time;
  if (out_full) // if the outlink is full
  {
    if (!blocked) // block if not blocked
    {
      blocked=true;
      inlink->add_blocked_exit(); // tell the link
    }
    nexttime=time+1.0; // next time the turning should try to put something in. Maybe this can be optimized.
    return false;
  }
  else   // outlink is not full
  {
    // unblock if necessary
    if (blocked)
    {
      blocked=false;
      inlink->update_exit_times(time,outlink,size); // update the exit times
      inlink->remove_blocked_exit(); // tell the link that this turning is not blocked anymore.
    }
    if (inlink->empty())
    {
      nexttime=time+(inlink->get_freeflow_time());
      return false;
    }
    else	
	{	double next_outlink_time=outlink->next_accept_time();
		if (time>= next_outlink_time) // if outlink can accept vehicles
		{
			if (check_controlling(time))
			{
				Vehicle* veh=inlink->exit_veh(time, outlink, size);
				if (inlink->exit_ok())
				{
					ok=outlink->enter_veh(veh, time+delay);	
				   if (ok)
					 return true;
				   else
				   {
					 eout << " turning " << id << " dropped a vehicle, because outlink couldnt enter vehicle" << endl;
					 nexttime=inlink->next_action(time);
					 delete veh;
					 return false;
				   }           
				}
			   else
				{
					nexttime=inlink->next_action(time);
				   if (nexttime <= time)
					   eout << "nexttime <= time in Turning::process_veh, from inlink::next_action " << endl;
				  return false;
				}
			}
			else // vehicle has to give way, needs to wait
			{
				// ask when to try again?
				// simply next time in the server for now,. Maybe we can do something smarter later
				nexttime = next_time(time);
			}
		}
		else // outlink cannot yet accept vehicles
		{
			nexttime=next_outlink_time;
			ok=false;
		}
	 }
   }
  return ok;
}
bool Turning::check_controlling(double time) 
{
	if ((!theParameters->use_giveway) || (controlling_turnings.size() == 0 )) // bypass
		return true; // can pass
	if (waiting && ((time-waiting_since) > theParameters->max_wait)) // if maximum wait has passed return true, reset counters
	{	
		waiting=false;
		waiting_since = 0.0;
		return true; // can pass
	}
	bool can_pass = true; // initialize at true
	// Check all controlling turnings if vehicle can pass
	vector <Turning*>::iterator gv = controlling_turnings.begin();
	for (gv ; gv != controlling_turnings.end(); gv++)
	{
		can_pass = can_pass && (*gv)->giveway_can_pass(time); // if any of the controlling turnings gives false, can_pass is false
	}
	if (!can_pass)
	{
		if (!waiting) // if not already waiting, set the counter 
		{
			waiting = true;
			waiting_since=time;
		}
		return false; // cannot pass
	}
	else
	{
		waiting=false;
		waiting_since = 0.0; //reset counter.
		return true; // can pass
	}
}

bool Turning::giveway_can_pass(double time) // returns true if vehicle from minor turning can pass
{
	if (!active)// if turning is not active (red light)
		return true; // can pass
	else
	{
		// Simplest possible implementation
		// check if there is any vehicle in the incoming link ready to make this turn.
		return !(inlink->veh_exiting(time,outlink,size));
	}
	return true;
}


double Turning::next_time(double time)
{
	return server->next(time) ;
}

bool Turning::execute(Eventlist* eventlist, double time)
{
  bool ok=true;
  for (vector<TurnAction*>::iterator iter=turnactions.begin(); iter < turnactions.end(); iter++)
  {
   ok=ok && (*iter)->execute(eventlist, time);
  }
  return ok;
}

bool Turning::init(Eventlist* eventlist, double time)
{
  bool ok=true;
  for (vector<TurnAction*>::iterator iter=turnactions.begin(); iter < turnactions.end(); iter++)
  {
   ok=ok && (*iter)->execute(eventlist, time);
   time += 0.00000003; // to make sure time is unique in eventlist
  }
  return ok;
}

void Turning::activate (Eventlist* eventlist,double time)
{
	active = true;
	for (vector<TurnAction*>::iterator iter=turnactions.begin(); iter < turnactions.end(); iter++)
	{
		 (*iter)->execute(eventlist, time);
		 time += 0.00000003; // to make sure time is unique in eventlist
	}
} 

double Turning::link_next_time(double time)
{
	return inlink->next_action(time);
}

// TurnAction methods

TurnAction::TurnAction(Turning* turning_):Action(), turning(turning_) {}

const bool TurnAction::execute (Eventlist* eventlist, const double time)
{
	
	// process vehicle if any  FOR THIS DIRECTION
	if (turning->is_active())
	{
		bool exec_ok=turning->process_veh(time);
		// get new time from server if exec_ok; otherwise get it from link if applicable.
	   if (exec_ok)
			new_time=turning->next_time(time); // the action went fine, get time for next action
	   else
		{
    		new_time=turning->nexttime; // otherwise the function process_veh has provided a next time.
			if (new_time < time)
			{
				eout << "trouble in the turning action:: exectute. newtime < current time : " << time << endl;
				new_time=time+0.1;
			}
		 }
    
	#ifdef _DEBUG_TURNING	
		eout <<"Turnaction::execute...result: " << exec_ok;
   		eout << " ...next turnaction @ " << new_time << endl;
	#endif //_DEBUG_TURNING
		// book myself in Eventlist with new time;
		eventlist->add_event(new_time,this);
	}
	 return true;
}

void Turning::write(ostream& out)
{
	out << "{ " << id <<" "<< node->get_id()<< " " << server->get_id() << " " << inlink->get_id() << " " << outlink->get_id();
	out << " " << size << " }" << endl;
}

