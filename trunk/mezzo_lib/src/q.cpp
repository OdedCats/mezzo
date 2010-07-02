#include "q.h"
#include <math.h>



Q::Q(const double maxcap_, const double freeflowtime_):maxcap(maxcap_), freeflowtime(freeflowtime_), ok(false), next_action(0.0)
{
	random=new Random();
	if (randseed != 0)
	   random->seed(randseed);
	else
		random->randomize();	
}

Q::~Q()
{
	delete random;
	for (vector <Route*>::iterator iter=routes.begin();iter<routes.end();)
	{			
		delete (*iter); // calls automatically destructor
		iter=routes.erase(iter);
		// delete the vehicles:
	}
	list  <Veh_in_Q>::iterator veh_iter=vehicles.begin();
	for (veh_iter; veh_iter!=vehicles.end(); )
	{
		delete (*veh_iter).second;
		veh_iter=vehicles.erase(veh_iter);
	}
}

void Q::reset()
{
	ok = false;
	next_action = 0.0;
	// delete the vehicles:
	list  <Veh_in_Q>::iterator veh_iter=vehicles.begin();
	for (veh_iter; veh_iter!=vehicles.end(); )
	{
		delete (*veh_iter).second;
		veh_iter=vehicles.erase(veh_iter);
	}
	viter = vehicles.begin();
	ttime=1.0;
	vehicle = NULL;
}


const bool Q::enter_veh(Vehicle*const veh)
{
	if (!full())
	{
	   ttime=veh->get_exit_time();

 		if (empty())
		{
		   vehicles.insert(vehicles.end(),Veh_in_Q(ttime , veh));
		   return true;
		}
		viter=vehicles.end();
		viter--;
		while ( (viter->first >ttime ) && (viter !=vehicles.begin()) )
			viter--;
		vehicles.insert(++viter,Veh_in_Q(ttime , veh));
		return true;
     }
    else
    	 return false;	
}

void Q::update_exit_times(const double time, const Link* const nextlink, const int lookback, const double v_exit, const double v_shockwave)
/* Calculates the shockwave that propagates upstream when a queue dissipates and re-calculates the exit times for the vehicles
 *       NOTE: if we return last t0 value in case the shockwave reaches the last veh, it can be used together with the
 *                    distance the veh has travelled, to calculate the time the shockwave reaches the upstream node (given the inflow rate)
 * v_exit= speed at capacity, v_shockwave = shockwave speed
 *
*/
{
  n=0; // number of vehicles looked back in queue.
//  double v_shockwave=6.3; // shockwave speed m/s
  double t0=time, t1=1.0, t2=0.0, newtime=time;
  if (!empty())
  {
      viter=vehicles.begin();
      nextid=nextlink->get_id();
      while (viter != vehicles.end())
      {
        if (viter->first > t0)
            return; // exit when first vehicle with earliest exit time larger than t0 is reached
        vehicle=viter->second;
        if (n < lookback) // for all vehicles within the lookback: update if going to nextlink, otherwise not
        {
          n++;
          vnextid=(vehicle->nextlink())->get_id();
          if (vnextid==nextid)
          {
            // for all vehicles up to the lookback, a single lane (pocket) is assumed.
            t1=vehicle->get_length()/v_shockwave; // time it takes for shockwave to travel from previous vehicle to current
            t2=vehicle->get_length()/v_exit; // extra time it takes to drive to the stopline (in addition to the time included in t0)
             // not completely correct: it should be the length of the vehicle in front. But on average it doesnt matter
            newtime=t0+t1+t2;
            viter->first=newtime;
            vehicle->set_exit_time(newtime);
            t0=newtime; // new 'earliest exit time' for next vehicle
          }
          viter++;
        }
        else  // for all vehicles past the lookback, update the times
        {
          n++;
          t1=vehicle->get_length()/v_shockwave; // time it takes for shockwave to travel from previous vehicle to current
          t2=vehicle->get_length()/v_exit; //  extra time it takes to drive to the stopline (in addition to the time included in t0)
                                // not completely correct: it should be the length of the vehicle in front. But on average it doesnt matter
          newtime=t0+t1+t2;
          int nr_lanes=vehicle->get_curr_link()->get_nr_lanes();
          for (int i=0; (i<nr_lanes) && (viter!=vehicles.end()) ; i++) // for each lane there is a shockwave
          {
            viter->first=newtime;
            viter->second->set_exit_time(newtime);
            viter++;
          }
          t0=newtime; // new 'earliest exit time' for next vehicle
        }
      }
   }
}

const bool Q::veh_exiting (const double time, const Link* const nextlink, const int lookback)
{
	int next = 0;
	if (empty())
		return false;
	else
	{
	    int i=0;
		list <Veh_in_Q>::iterator v_it = vehicles.begin();
		for (v_it; (v_it != vehicles.end()) && (i<lookback);v_it++,i++)
		{
			next=v_it->second->nextlink()->get_id();
			if (nextlink->get_id() == next)
			{
				if (v_it->first > time)
					return false;
				else
					return true;
			}
			
		}
		return false;
	}
}

Vehicle* const Q::exit_veh(const double time, const Link* const nextlink, const int lookback)

{
	ok=false;
	next_action=-1.0;
    if (empty())
        return NULL;
    else
    {
      n=0; // number of vehicles looked back in queue.
      // lookback until you find a vehicle for this direction
      viter=vehicles.begin();
      nextid=nextlink->get_id();
      while (!ok && (viter!=vehicles.end()) )
      {
          vehicle=viter->second;
	      vnextid=(vehicle->nextlink())->get_id();
          if (vnextid==nextid)
          {
              ok=true;	 // vehicle found, exit loop
              break;
          }
          else // keep looking
          {
             n++;
             viter++;
          }
       }
       // now check 1. is one found, 2  Does it have an earliest exit time < now? 3 .is it withn the lookback?
      if (ok) // vehicle found
      {
         if ((viter->first) <= time) // it can actually exit
         {
             if (n < lookback) // within the lookback boundary
             {
               vehicles.erase(viter);
               return vehicle;
             }
             else     // past the lookback boundary  and it has an earliest exit time < time... what is the next time I should try?
             {
               ok=false;
               next_action=time+1.0*(n/((vehicle->nextlink())->get_nr_lanes()));// ????????????? Could also do something like lookback*time per vehicle /nr_lanes
               return NULL;
             }
         }
         else // it cannot yet exit, so the next try should be when it (theoretically) can
         {
           ok=false;
           next_action=viter->first;
           return NULL;
         }     
      }
      else // no vehicle found viter==vehicles.end(), and the list of vehicles is not empty (otherwise it would have returned NULL before)
      {
        ok=false;
        next_action=time+freeflowtime;
        return NULL;
      }
    }
}

  


Vehicle* const Q::exit_veh(const double time)
{
	ok=false;
	next_action=0.0;
 	if (!empty())
 	{
 		value=vehicles.front();
 		if (value.first<=time)
 		{
 			vehicles.pop_front();
 			ok=true;
 			return value.second;
 		}
 		else
 		{
 			ok=false;
 			next_action=value.first;
 		}
 	}
  else
  	return NULL;
  return NULL;
}

void Q::broadcast_incident_start(const int lid, const vector <double> & parameters)
{
 // for all vehicles
 	for (list <Veh_in_Q>::iterator iter1=vehicles.begin();iter1!=vehicles.end();iter1++)
 	{	
 		double receive=random->urandom();	
 		if (receive < perc_receive_broadcast) // if vehicle receives the message '
 		{
 		   receive_broadcast(iter1->second,lid,parameters);
 		}
 	}	
} 		

void Q::receive_broadcast(Vehicle* const veh, const int lid, const vector <double> & parameters) 		
{
 			int curr_lid=(veh->get_curr_link())->get_id();
 			Route* curr_route=veh->get_route();
 			Route* alt_route;
 			bool can_change=false;
 			// check if link is on route AFTER currlink
 			if (curr_route->has_link_after(lid, curr_lid))
 			{
 				ODVal ODValue=curr_route->get_oid_did();
 				// find an alternative route with the same o and d; 
				// TODO change to a map <> structure instead
 		   		vector <Route*>::iterator iter2=(find_if (routes.begin(),routes.end(), compare_route (ODValue))) ;
	 		    if (iter2<routes.end())
 			    {
 			    	  	alt_route=(*iter2);
 			    	  	can_change=true;
 			    }
 			    else
 			    {
 			     	// find alternative and insert in list as new route
 			     	vector <alternativetype>::iterator iter3, alt;
 			     	alt=alternatives.end();
 			     	for ( iter3=alternatives.begin();iter3<alternatives.end(); iter3++)
 			     	{
 			     		// if destination of alternative == destination 0f Vehicle
 			     	 	if   ((*iter3).first == ODValue.second)
 			     	 	{ 	
 			     	 	 	alt=iter3;
 			     	 	} 	
 			     	} 	
 			     	if (alt < alternatives.end())	
		 		    { 	
	 		     		alt_route=new Route(9999,curr_route,(*alt).second);
		 		    	if (!(alt_route->equals(*curr_route)) )
		 		     	{
		 		     		routes.insert(routes.begin(),alt_route ); 		
		 		       	can_change=true;
		 		       }
		 		       else
		 		       	delete alt_route;
		 		    }	
	 		     }
	 			if (can_change)
	 			{
	 				 switchroute (veh, curr_route, alt_route, parameters);
	 			 }
     		}
}


void Q::switchroute(Vehicle* const veh, Route* const curr_route, Route* const alt_route, const vector <double> & parameters)
// parameters contain the mu and sd's of all  beta's and X1
{
 	veh->set_switched(-1); // sets the default of a negative switching decision (staying on habitual road)
  	double Va, Vh, beta1, beta2, beta3, beta4, X1, X2, X3, Pa;
 	// 1. calculate Va=b2*X2
	beta2 = random->nrandom(parameters[2],parameters[3]);
	X2=((alt_route->cost() ) - ( curr_route->cost() )) / 60;
	Va=beta2*X2;
 // 2. calculate Vh=b3*X3+b4 OR Vh=b1*X1+b4
 	beta4= random->nrandom(parameters[6],parameters[7]);
 	X3=0.0;
 	if (random->urandom() <= dont_know_delay )
 	{
 		X3=1.0;
 		beta3 = random->nrandom(parameters[4],parameters[5]);
 		Vh=beta3*X3+beta4;
 	}
 	else
 	{
 	    beta1= random->nrandom(parameters[0],parameters[1]);
 	    X1=random->lnrandom(parameters[8],parameters[9]);
 	    Vh=beta1*X1+beta4; 	
 	}
	//eout << "Route change: Vh= " << Vh << " and Va= " << Va << endl;
	Pa=(exp(Va)/(exp(Va)+exp(Vh)));
	if (random->urandom() <= Pa)
		veh->set_route(alt_route); // switched=1 automatically set by changing route
}

//calculates the space occupied by the queueing part of the vehicles, taking the individual vehicle lengths
 const double Q::calc_space(const double time) const
 {
	if (empty())
		return 0.0;
	double space=0.0;
	list <Veh_in_Q>::const_iterator iter1=vehicles.end();
   	while (iter1!=vehicles.begin())
   	{
   		iter1--;
   		if (iter1->first > time)
   	  		return space;
   	 	space+=(iter1->second)->get_length();
   	
   	}
	return space;
 }

 const double Q::calc_total_space() const  // returns the total space occupied by queue;
 {
   if (empty())
		return 0.0;
	double space=0.0;
	list <Veh_in_Q>::const_iterator iter1=vehicles.end();
   	while (iter1!=vehicles.begin())
   	{
   		iter1--;
      space+=(iter1->second)->get_length();
    }
 return space;
 }
 
