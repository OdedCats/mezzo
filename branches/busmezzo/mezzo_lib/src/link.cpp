
#include "link.h"
#include <list>
#include <string>
#include "grid.h"
#include <typeinfo> // for comparing different classes (Bus ans Vehicle)

Link::Link(int id_, Node* in_, Node* out_, int length_, int nr_lanes_, Sdfunc* sdfunc_): id(id_), in_node (in_),
		out_node(out_), length(length_), nr_lanes(nr_lanes_), sdfunc(sdfunc_)
{
	maxcap=static_cast<int> (length*nr_lanes/theParameters->standard_veh_length);
#ifdef _DEBUG_LINK	
	cout << "link " << id << " maxcap " << maxcap << endl;
#endif //_DEBUG_LINK
	
#ifdef _COLLECT_TRAVELTIMES
	const int nr_fields=3;
	string names[nr_fields]={"lid","time","traveltime"};
    vector <string> fields(names,names+nr_fields);
	grid=new Grid(nr_fields,fields);
#endif // _COLLECT_TRAVELTIMES
#ifdef _COLLECT_ALL
	const int nr_fields=8;
	string names[nr_fields]= {"time","vid","lid","entry?","density","qlength","speed","earliest_exit_time"};
   vector <string> fields(names,names+nr_fields);
	grid=new Grid(nr_fields,fields);
#endif //_COLLECT_ALL	
	avg_time=0.0;
	avgtimes=new LinkTime();
	avgtimes->id=id;
	histtimes=NULL;
	nr_passed=0;
	running_percentage=0.0;
	queue_percentage=0.0;
	blocked=false;
	moe_speed=new MOE(theParameters->moe_speed_update, 3.6);
	moe_inflow=new MOE(theParameters->moe_inflow_update, (3600.0/theParameters->moe_inflow_update));
	moe_outflow=new MOE(theParameters->moe_outflow_update, (3600.0/theParameters->moe_outflow_update));
	moe_queue=new MOE(theParameters->moe_queue_update);
	moe_density=new MOE(theParameters->moe_density_update);
  blocked_until=-1.0; // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
  nr_exits_blocked=0; // set by the turning movements if they are blocked
  freeflowtime=(length/(sdfunc->speed(0.0)));
  if (freeflowtime < 1.0)
      freeflowtime=1.0;
  queue=new Q(maxcap, freeflowtime);    
  use_ass_matrix = false;
  selected = false;
}


Link::Link()
{
	moe_speed=new MOE(theParameters->moe_speed_update);
	moe_inflow=new MOE(theParameters->moe_inflow_update);
	moe_outflow=new MOE(theParameters->moe_outflow_update);
	moe_queue=new MOE(theParameters->moe_queue_update);
	moe_density=new MOE(theParameters->moe_density_update);
	blocked_until=-1.0; // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
	nr_exits_blocked=0; // set by the turning movements if they are blocked
	freeflowtime=1.0;	
	selected = false;
}		

Link::~Link()
{
	delete(queue);
	delete(moe_speed);
	delete(moe_outflow);
	delete(moe_inflow);
	delete(moe_queue);
	delete(moe_density);
#ifdef _COLLECT_TRAVELTIMES
	if (grid!=null )
		delete(grid);
#endif
 #ifdef _COLLECT_ALL
	if (grid!=null )
		delete(grid);
#endif
	if (histtimes!=NULL)
		delete (histtimes);
    if (avgtimes!=NULL)
      delete (avgtimes);
	
}

void Link::reset()
{
	avg_time=0.0;
	curr_period=0;
	tmp_avg=0.0;
	tmp_passed=0;
	// Reset avgtimes
	//avgtimes->reset();
	delete avgtimes;
	avgtimes = new LinkTime(*histtimes);
	nr_passed=0;
	running_percentage=0.0;
	queue_percentage=0.0;
	blocked=false;
	moe_speed->reset();
	moe_inflow->reset();
	moe_outflow->reset();
	moe_queue->reset();
	moe_density->reset();
	blocked_until=-1.0; // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
	nr_exits_blocked=0; // set by the turning movements if they are blocked
	freeflowtime=(length/(sdfunc->speed(0.0)));
	if (freeflowtime < 1.0)
      freeflowtime=1.0;
	queue->reset();    
	use_ass_matrix = false;
	selected = false;
	ass_matrix.clear();
}

void Link::end_of_simulation()
{	
	// store times for current(last) period in avgtimes and make sure all periods have values.
	if (tmp_avg==0.0)
		tmp_avg = histtimes->times [curr_period];
		//tmp_avg=freeflowtime;
	avgtimes->times[curr_period] = tmp_avg;
	int i= 0;
	for (i;i<histtimes->nrperiods;i++)
	{
		if (avgtimes->times.count(i)==0) // no avg time exists, should be impossible, but to be sure...
		{
			avgtimes->times[i] = histtimes->times[i];
		}
		else
		{
			if (avgtimes->times [i] == 0.0) // should be impossible
				avgtimes->times[i] = histtimes->times[i];
		}

	}
	

}

const int Link::get_out_node_id ()
	{return out_node->get_id();}
	
const int Link::get_in_node_id()
	{return in_node->get_id();}

const bool Link::full()
	{
    return queue->full();
  }

  
const bool Link::full(double time)

{
  moe_density->report_value(density(),time); // to make sure even values are reported when queue is full
   if (blocked_until==-2.0)
       return true;

   if (queue->full())
   {    
     if (nr_exits_blocked >0)     // IF one of the turning movements is blocked
         blocked_until=-2.0; // blocked until further notice.
         // TO DO: make turnings unblock when the specific queue for that exit < lookback for other turnings
         
     return true; // IF the queue is full this returns true. IF ALSO at least one exit is blocked, blocked_until=-2.0 and this will stay blocked untill a shockwave.
   }
   
   if (blocked_until==-1.0)
     return false; // no queue
   else  
   {
      if (blocked_until <= time) // queue front has reached upstream node
      {
         blocked_until=-1.0;  // unblock the link
         return false;
      }
      else
         return true; // queue is dissipating, but shockwave front has not reached upstream node yet.
   }   
}
     
const bool Link::empty()
	{return queue->empty();}		

const double Link::next_action (double time)
	{ double newtime;
      if (queue->empty() )
			newtime= (time+freeflowtime);
		else
			newtime= queue->next();
      if (newtime <= time)
        cout << "Link::nextaction: newtime= " << newtime << " and time = " << time <<  " and freeflowtime = " << freeflowtime << endl;
      return newtime;
	}	 // optimisation: return freeflow time if queue is empty

const int Link::size()
	{return queue->size();}	
	
	
void Link::add_alternative(int dest, vector<Link*> route)
	{queue->add_alternative(dest, route);}	
void Link::add_alternative_route(Route* route) 
{
	queue->add_alternative_route(route);
	vector<Link*> route_v = route->get_downstream_links (id);
	int dest =route->get_destination()->get_id(); 
	queue->add_alternative (dest, route_v);
}

void Link::register_route (Route* route) 
{
	int dest_id = route->get_destination()->get_id();
	routemap.insert(pair<int,Route*>(dest_id,route));

} // STUB to be implemented later 2008-01-29

void Link::set_selected (const bool sel) 
{	
	selected = sel;
#ifndef _NO_GUI	
	icon->set_selected(selected);
#endif // _NO_GUI	
}

pair<double,double> Link::set_output_moe_thickness(unsigned int val)// sets the output MOE for the link icon
{
#ifndef _NO_GUI	
	int nr_periods;
	//double min, max;
	switch (val)
	{
		case (1):
			icon->setMOE_thickness(moe_outflow);
			nr_periods= static_cast<int>(theParameters->moe_outflow_update/theParameters->running_time);
			moe_outflow->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_outflow->get_min(), moe_outflow->get_max());
			break;
		case (2):
			icon->setMOE_thickness(moe_inflow);
			nr_periods= static_cast<int>(theParameters->moe_inflow_update/theParameters->running_time);
			moe_inflow->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_inflow->get_min(), moe_inflow->get_max());
			break;
		case (3):
			icon->setMOE_thickness(moe_speed);
			nr_periods= static_cast<int>(theParameters->moe_speed_update/theParameters->running_time);
			moe_speed->fill_missing(nr_periods, this->speed_density(0.0));
			return pair <double,double> ( moe_speed->get_min(), moe_speed->get_max());
			break;
		case (4):
			icon->setMOE_thickness(moe_density);
			nr_periods= static_cast<int>(theParameters->moe_density_update/theParameters->running_time);
			moe_density->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_density->get_min(), moe_density->get_max());
			break;
		case (5):
			icon->setMOE_thickness(moe_queue);
			nr_periods= static_cast<int>(theParameters->moe_queue_update/theParameters->running_time);
			moe_queue->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_queue->get_min(), moe_queue->get_max());
			break;
		case (0):
			icon->setMOE_thickness(NULL);

	}
	#endif // _NO_GUI	
	return pair <double, double> (0,0);
}
pair <double,double> Link::set_output_moe_colour(unsigned int val)// sets the output MOE for the link icon
{
#ifndef _NO_GUI	
	int nr_periods;
	//double min, max;
	switch (val)
	{
		case (1):
			icon->setMOE_colour(moe_outflow);
			nr_periods= static_cast<int>(theParameters->moe_outflow_update/theParameters->running_time);
			moe_outflow->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_outflow->get_min(), moe_outflow->get_max());
			break;
		case (2):
			icon->setMOE_colour(moe_inflow);
			nr_periods= static_cast<int>(theParameters->moe_inflow_update/theParameters->running_time);
			moe_inflow->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_inflow->get_min(), moe_inflow->get_max());
			break;
		case (3):
			icon->setMOE_colour(moe_speed);
			nr_periods= static_cast<int>(theParameters->moe_speed_update/theParameters->running_time);
			moe_speed->fill_missing(nr_periods, this->speed_density(0.0));
			return pair <double,double> ( moe_speed->get_min(), moe_speed->get_max());
			break;
		case (4):
			icon->setMOE_colour(moe_density);
			nr_periods= static_cast<int>(theParameters->moe_density_update/theParameters->running_time);
			moe_density->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_density->get_min(), moe_density->get_max());
			break;
		case (5):
			icon->setMOE_colour(moe_queue);
			nr_periods= static_cast<int>(theParameters->moe_queue_update/theParameters->running_time);
			moe_queue->fill_missing(nr_periods, 0.0);
			return pair <double,double> ( moe_queue->get_min(), moe_queue->get_max());
			break;
		case (0):
			icon->setMOE_colour(NULL);

	}
	#endif // _NO_GUI	
	return pair <double, double> (0,0);
}

void Link::update_exit_times(double time,Link* nextlink, int lookback)
{
   // Not good enough (2004-01-26) need to use the upstream density to calc exit speed & shockwave speeds
   double kjam=142.0;
   double v_exit, v_shockwave, v_dnstr;
   double k_dnstr= nextlink->density();
   v_exit= sdfunc->speed(kjam);   
   v_dnstr=nextlink->speed_density(k_dnstr);    // downstream speeds can be ridiculously low in case Mitsim provides them
                                                                                 // for instance when a 'gap' in the queue crosses the border (the queue
                                                                           // compresses
    if (v_dnstr <= v_exit )                                                                             
           v_dnstr=v_exit-1.0;;
   // v_shockwave=deltaQ/deltaK
   if (k_dnstr >= sdfunc->get_romax())           // just another test
       v_shockwave=v_exit-1.0;
   else
       v_shockwave= (k_dnstr*v_dnstr)/(kjam-k_dnstr); //re-ordered to give a positive value (*-1)

   if (v_shockwave >= v_exit) // due to the limited domain of the speed/density functions we need to check
       v_shockwave=v_exit - 1.0;
   if (v_shockwave==0.0)
	   v_shockwave = v_exit;
   
   /*
    v_exit=sdfunc->speed(k_dnstr);
    v_dnstr=nextlink->speed_density(k_dnstr);
    if (v_dnstr <= v_exit)
    {
           // v_dnstr=v_exit;
           blocked_until=-1.0;

           return;
     }      
   if (k_dnstr >= sdfunc->get_romax())
   {
          // k_dnstr is outside the domain of the speed-density funciton
         
         
          v_shockwave=v_dnstr-0.5;
   }
    
   if (k_dnstr>=kjam)
  {
      cout << " Big trouble!  upstream density >= downstream in queue dissipation..." << endl;
      blocked_until=-1.0;
      return; // no updating of times
    }
    
   else
   {
        v_shockwave= v_dnstr* (k_dnstr/(kjam-k_dnstr)) ;           
   }
    */
#ifdef _DEBUG_LINK
    cout << " VEXIT = " << v_exit << endl;
    cout << " VSHOCKWAVE = " << v_shockwave << endl;
#endif //_DEBUG_LINK
	//double v_shockwave=((kjam*vjam*3.6 - k_dnstr*v_exit*3.6)/(kjam-k_dnstr))/3.6;
   //double v_shockwave=5.0;
   //if (v_shockwave < 0.0)
  //        v_shockwave= - v_shockwave; // can't remember when it's pos or neg...
   queue->update_exit_times(time,nextlink,lookback,v_exit, v_shockwave);
   if (blocked_until==-2.0) // if the link is completely blocked until further notice
   {
          blocked_until=(length/v_shockwave)+time; // time when shockwave reaches the upstream node
#ifdef _DEBUG_LINK
          cout << "shockwave at link " << id << ", blocked until " << blocked_until <<". Time now " << time ;
          cout << ", v_exit = " << v_exit << ", v_shockwave = " << v_shockwave << ", kdownstr = " << k_dnstr << endl;
#endif // _DEBUG_LINK
   }
}   // exit speed is based on downstream density
                                    
  
double Link::speed(double time)
{	
	double ro=0.0;
#ifdef _RUNNING     // if running segment is seperate density is calculated on that part only
	ro=density_running(time);
#else
	#ifdef _RUNNING_ONLY
      ro=density_running_only(time);
	#else	
		ro=density();
	#endif	// _RUNNING_ONLY
#endif // _RUNNING
//	cout << "Link::speed: ro_runningonly = " << ro << " speed is " << sdfunc->speed(ro) ;
//	cout << " ro normal is " << density() <<" nr running " << queue->nr_running(time) <<  endl;
//	cout << " the ro for density_running " << density_running(time) << endl;
	
	return (sdfunc->speed(ro));	
}


double Link::speed_density(double density_)
{
    return sdfunc->speed(density_);
}

bool Link::enter_veh(Vehicle* veh, double time)
{
   double ro=0.0;
#ifdef _RUNNING     // if running segment is seperate density is calculated on that part only
	ro=density_running(time);
#else
	#ifdef _RUNNING_ONLY
		ro=density_running_only(time);
	#else	
		ro=density();
	#endif	//_RUNNING_ONLY
#endif  //_RUNNING
	double speed=sdfunc->speed(ro);	
	//moe_speed->report_value(speed,time);
	moe_density->report_value(density(),time);
	moe_queue->report_value((queue->queue(time)),time);
	moe_inflow->report_value(time);
	double exit_time=(time+(length/speed)) ;
  #ifdef _USE_EXPECTED_DELAY
    double exp_delay=0.0;
    exp_delay=1.44*(queue->queue(time)) / nr_lanes;
    /*
    double current_outflow=nr_lanes*(moe_outflow->get_last_value());
    if (current_outflow > 0.0)          // if < 0.0 then there is no value for outflow, so no queue delay either
      exp_delay=3600.0*(queue->queue(time))/current_outflow;  */
    exit_time=exit_time+exp_delay;
    cout << "link_enter:: exp_delay = " << exp_delay << endl;
    #endif //_USE_EXPECTED_DELAY
   veh->set_exit_time(exit_time);
   veh->set_curr_link(this);
   veh->set_entry_time(time);
   update_icon(time);	
#ifdef _COLLECT_ALL	
	list <double> collector;	
	collector.push_back(time);
	collector.push_back(veh->get_id());
	collector.push_back(id);
	collector.push_back(1.0);
	collector.push_back(ro);
	collector.push_back((queue->size()+1));
	collector.push_back(speed);
	collector.push_back(exit_time);
	grid->insert_row(collector);
#endif //_COLLECT_ALL
#ifdef _DEBUG_LINK
		cout <<"Link::enter_veh lid:  " << id;
		cout <<" time: " << time;
		cout <<" density: " << ro ;
       cout << "  speed:  "<< speed << " m/s";
		cout << "  exit_time: " << exit_time;
	   	cout << " queue size: " << (queue->size()+1) << endl;
#endif //_DEBUG_LINK	
	// report the ass_matrix contribution
	if (use_ass_matrix)
	{
		int cur_l_period = static_cast <int> (time / theParameters->ass_link_period);
		int od_period = static_cast <int> (veh->get_start_time() / theParameters->ass_od_period);
		const odval& od_pair = veh->get_odids();
		// Test first if the location has been used yet, since we only allocate used places in memory
		if ( (ass_matrix[cur_l_period].empty()) || (ass_matrix[cur_l_period] [od_pair].empty()) ||
			 (ass_matrix[cur_l_period] [od_pair].find(od_period) == ass_matrix[cur_l_period] [od_pair].end()))
		{
			ass_matrix [cur_l_period] [od_pair] [od_period] = 1;
		}
		else
		{
			ass_matrix [cur_l_period] [od_pair] [od_period] ++;
		}
	}
// add here the BUSSTOP FUNCTIONS
//#define _BUSES
#ifdef _BUSES
	//if (typeid (*veh) == typeid (Bus))
	if (veh->get_type() == 4)
	{
		// Calc time to stop
		Bus* bus =  (Bus*)(veh); // so we can do Bus operations
		//vector <Start_trip*>::iterator curr_trip = bus->get_curr_trip();
		Bustrip* trip = bus->get_curr_trip();
		if (trip->check_end_trip() == false)
		{
			Visit_stop* next_stop1 = *(trip->get_next_stop());
			if (id == (next_stop1->first->get_link_id()))
			{
				double stop_position = (next_stop1->first)->get_position();
				double time_to_stop = time + ((exit_time - time) * (stop_position / length)) + trip->get_line()->extra_disruption_on_segment(next_stop1->first, time);

				// book  stop visit
				trip->book_stop_visit (time_to_stop);
				return true;
			}
		}
	}
// test for type ID and if bus:
	// Calc time to stop
	// book time to stop

#endif

// END BUSSTOP FUNCTIONS

	if (veh->get_curr_link() != this)
		cout << "Wrong link!" << endl;
	return queue->enter_veh(veh);
}

bool Link::veh_exiting(double time, Link* nextlink, int lookback)
	{return queue->veh_exiting(time,nextlink,lookback);}

Vehicle* Link::exit_veh(double time, Link* nextlink, int lookback)
{
	ok=false;
	if (!empty())
	{
		Vehicle* veh=queue->exit_veh(time, nextlink, lookback);
		if (queue->exit_ok())
		{
			double entrytime=veh->get_entry_time();
			double traveltime=(time-entrytime);
			avg_time=(nr_passed*avg_time + traveltime)/(nr_passed+1); // update of the average
			if ((curr_period+1)*(avgtimes->periodlength) < entrytime )
			{			 	
			 	if (tmp_avg==0.0)
					tmp_avg = histtimes->times [curr_period];
					//tmp_avg=freeflowtime;
			 	//avgtimes->times.push_back(tmp_avg);
				avgtimes->times [curr_period] = tmp_avg;
			 	curr_period++;
			 	tmp_avg=0.0;
			 	tmp_passed=0;

			}
			else
			{
				tmp_avg=(tmp_passed*tmp_avg + traveltime)/(tmp_passed+1); // update of the average			
				tmp_passed++;
			}
			nr_passed++;
          list <double> collector;
#ifdef _COLLECT_ALL
			double speed=sdfunc->speed(density());
          double exit_time=veh->get_exit_time();				
			collector.push_back(time);
			collector.push_back(id);
			collector.push_back(0.0);
			collector.push_back(density());
			collector.push_back((queue->size()));
			collector.push_back(speed);	
      		collector.push_back(exit_time);
      		grid->insert_row(collector);
#else
	#ifdef _COLLECT_TRAVELTIMES
			collector.push_back(id);
			collector.push_back(time);			
			collector.push_back(traveltime);			
      		grid->insert_row(collector);
	#endif // _COLLECT_TRAVELTIMES
#endif // _COLLECT_ALL     		
	      update_icon(time);
	      ok=true;
	      veh->add_meters(length);
	      moe_outflow->report_value(time);
	      moe_speed->report_value((length/traveltime),time);
			return veh;
		}

	}
  else
  	return NULL;
  return NULL; 
}



Vehicle* Link::exit_veh(double time)
{
	ok=false;
	if (!empty())
	{
		Vehicle* veh=queue->exit_veh(time);
		if (queue->exit_ok())
		{
			double entrytime=veh->get_entry_time();
			double traveltime=(time-entrytime);
			avg_time=(nr_passed*avg_time + traveltime)/(nr_passed+1); // update of the average
			if ((curr_period+1)*(avgtimes->periodlength) < entrytime )
			{		
				if (tmp_avg==0.0)
					tmp_avg = histtimes->times [curr_period];
				//	tmp_avg=freeflowtime;
			 	//avgtimes->times.push_back(tmp_avg);
				avgtimes->times [curr_period] = tmp_avg;
			 	curr_period++;
			 	tmp_avg=0.0;

			 	tmp_passed=0;
			}
			else
			{

				tmp_avg=(tmp_passed*tmp_avg + traveltime)/(tmp_passed+1); // update of the average			
				tmp_passed++;
			}			
			nr_passed++;
			list <double> collector;
#ifdef _COLLECT_ALL
			collector.push_back(time);
			collector.push_back(id);
			collector.push_back(0.0);
			collector.push_back(density());
			collector.push_back((queue->size()));
			double speed=sdfunc->speed(density());
 	   		collector.push_back(speed);
	   		double exit_time=veh->get_exit_time();
      		collector.push_back(exit_time);
	  		grid->insert_row(collector);
#else
	#ifdef _COLLECT_TRAVELTIMES
			collector.push_back(id);
			collector.push_back(time);
			
			collector.push_back(traveltime);
	   		grid->insert_row(collector);  		
	#endif //_COLLECT_TRAVELTIMES
#endif //_COLLECT_ALL	
	   		update_icon(time); // update the icon's queue length
	   		ok=true;
	   		veh->add_meters(length);
	   		 moe_outflow->report_value(time);
	   		 moe_speed->report_value((length/traveltime),time);
			return veh;
		}
	}
	else
		return NULL;
	return NULL;
}

const double Link::density()
{
   int qsize=queue->size() ;
   	return (qsize/(nr_lanes*(length/1000.0))); // veh/km/lane and length is in meters	
}

const double Link::density_running(double time)
{
 	int nr_running=queue->nr_running(time);
   return (nr_running/(nr_lanes*(length/1000.0))); // veh/km and length is in meters
}

// 2002-07-25: new: queue space calculated using individual vehicles lengths
const double Link::density_running_only(double time)
{
  int nr_running=queue->nr_running(time);// nr cars in running segment
 // double queue_space=(queue->size()-nr_running)*theParameters->standard_veh_length; // length of queue
  double queue_space=queue->calc_space(time)/nr_lanes; // 2003_11_29: corrected : div by nr_lanes!
  double running_length=length-queue_space;
 
  	#ifdef _DEBUG_LINK
   	 // cout << "density_running_only:: nr_running " << nr_running;
   	  cout << " queue_space " << queue_space << endl;
   	#endif
    if (nr_running && running_length)
   	{
   	  return (nr_running / (running_length*(nr_lanes/1000.0) ) );
	}
	else
		return 0;
}


bool Link::write(ostream&)      // unused ostream& out
{
#ifndef _COLLECT_ALL
#ifndef _COLLECT_TRAVELTIMES
	return false;
#else
	return grid->write_empty(out);
#endif //_COLLECT_TRAVELTIMES
#endif //_COLLECT_ALL
	
}	


void Link::write_ass_matrix (ostream & out, int linkflowperiod)
{
	if (use_ass_matrix)
	{
		
		int no_entries=0;
		out << "link_id: " << id << endl;
	//	double factor=3600/theParameters->ass_link_period;
		map <int,int> ::iterator ass_iter2;
		ass_iter = ass_matrix [linkflowperiod].begin();
		// calculate the number of non-zero entries
		
		for (unsigned int h=0; h<ass_matrix[linkflowperiod].size(); ++h)
		{
			no_entries += ass_iter->second.size();
			ass_iter++;
		}
		
		out << "no_entries: " << no_entries << endl;
		
		// and reset ass_iter to start writing the entries
		ass_iter = ass_matrix [linkflowperiod].begin();
		for (unsigned int i=0; i<ass_matrix[linkflowperiod].size(); ++i)
		{
			ass_iter2=ass_iter->second.begin();
			for (unsigned int j=0; j<ass_iter->second.size(); ++j)
			{
				out << "{\t" << ass_iter->first.first << "\t" << ass_iter->first.second << "\t" 
					<< ass_iter2->first << "\t" << (ass_iter2->second)/*factor*/ << "\t}" << endl;
				ass_iter2++;
			}
			ass_iter++;
		}
	}
}



void Link::write_time(ostream& out)
{
	double newtime=0.0;
	int i = 0;
	this->end_of_simulation(); // check everything is stored as it should, including current temp values, no empty values in avg times, etc.
	out << "{\t" << id ;
	for (i; i<histtimes->nrperiods;i++)
	{
		newtime=time_alpha * (avgtimes->times[i]) + (1-time_alpha) * (histtimes->times[i]);
		out << "\t"<< newtime;
	}
	out << "\t}" << endl;
/* ************ Old crap
//#ifdef _COLLECT_TRAVELTIMES

  // TEST IF THERE ARE ANY AVG TIMES,    

  if (avgtimes->times.size()==0) // to make sure the old times are rewritten, in case there are no avg times
  {
    if (histtimes) // test if the histtimes exist (they don't if there was a problem reading the file)
      {
          for (vector<double>::iterator iter=(histtimes->times).begin();iter<(histtimes->times).end();iter++)
          {
            newtime=(*iter);
            out << "\t"<< newtime;
          }
        }	
  }
  else
  {
    if (avgtimes->nrperiods > (int)avgtimes->times.size()) // if there are more periods than data in the times vector
    {
          for (int j=0; j < ((avgtimes->nrperiods) -  (int)(avgtimes->times.size())); j++)
              avgtimes->times.push_back(avgtimes->times.back()); // copy the data from the last known position
    }
      //	for (vector<double>::iterator iter=(avgtimes->times).begin();iter<(avgtimes->times).end();iter++)
      for (int period=0;period < (avgtimes->nrperiods); period++)
      {
        double avgtime= 0.0;
		if (unsigned(avgtimes->nrperiods)> avgtimes->times.size())
		{

		}
		else
		{
			avgtime= avgtimes->times[period];
			newtime=(time_alpha)*avgtime+(1-time_alpha)*(histtimes->times[period]);
		}
		if (avgtime == 0) 
		{
			if (histtimes->times[period] > 0 )
				newtime = histtimes->times [period];
			else
				newtime = 1.0;
		}
        //newtime=(*iter);
		if (newtime < 1.0)
			newtime = 1.0;
        out << "\t"<< newtime;
      }
  }
  out << "\t}" << endl;
  */
	//TODO rewrite this procedure with new travel times
//#endif _COLLECT_TRAVELTIMES	
}

void Link::update_icon(double time)
{
	double cap;
	cap=maxcap;
	double totalsize, runningsize, queuesize;
	totalsize=queue->size();
	runningsize=queue->nr_running(time);
	queuesize=totalsize-runningsize;
	queue_percentage=(queuesize/cap);
	//running_percentage=(runningsize/cap);
	running_percentage = density_running_only(time)/140; // density as percentage of jamdensity
#ifdef _DEBUG_LINK	
	cout << " runningsize " << runningsize;
	cout << "  queueingsize " << queuesize << endl;
#endif //_DEBUG_LINK	
}
// INCIDENT FUNCTIONS

vector <Route*> Link::get_routes_to_dest(int dest) 
{
	multimap <int, Route*>::iterator start,stop; // CHECK OUT if we can simply pass the iterators...
	start = routemap.lower_bound(dest);
	stop = routemap.upper_bound(dest);
	vector <Route*> return_routes;
	for (start; start != stop; start++)
	{
		return_routes.push_back((*start).second);
	}
 return return_routes;

}
unsigned int Link::nr_alternative_routes(int dest, int incidentlink_id)
{
	unsigned int count = 0;
	vector <Route*> routes_to_dest=get_routes_to_dest(dest);
	vector <Route*>::iterator route_iter= routes_to_dest.begin();
	for (route_iter; route_iter!=routes_to_dest.end();route_iter++)
	{	
		if ( ! ((*route_iter)->has_link(incidentlink_id)) )
		{
			 count++;
			 add_alternative_route(*route_iter);
		}
	}
  return count;
}
void Link::set_incident(Sdfunc* sdptr, bool blocked_, double blocked_until_)
{
	temp_sdfunc=sdfunc;
	sdfunc=sdptr;
	blocked=blocked_;
	blocked_until=-2.0; 
	// NOTE : Check why blocked_until_ is ignored!
}

void Link::unset_incident()
{
	sdfunc=temp_sdfunc;
	blocked=false;
	blocked_until=-1.0; // unblocked
	cout << "the incident on link " << id << " is over. " << endl;
}


void Link::broadcast_incident_start(int lid, vector <double> parameters)
{
	queue->broadcast_incident_start(lid, parameters);
}



void Link::receive_broadcast(Vehicle* veh, int lid, vector <double> parameters)

{queue->receive_broadcast(veh,lid,parameters);}



double Link::calc_diff_input_output_linktimes ()
 {
	if (avgtimes->times.size() > 0)
		{
		double total = 0.0;
//		map <int,double>::iterator newtime=avgtimes->times.begin();
//		map <int,double>::iterator oldtime=histtimes->times.begin();
		/*
		for (newtime, oldtime; (newtime < avgtimes->times.end()) && (oldtime < histtimes->times.end()); newtime++, oldtime++)
		{
			if ((newtime < avgtimes->times.end()) && (oldtime < histtimes->times.end()) &&((*newtime) > 0))
			{
				total+= (*newtime) - (*oldtime);
			}
		}
		*/
		for (int i=0; i<avgtimes->nrperiods; i++)
			total += avgtimes->times [i] - histtimes->times [i];
		return total;
	}
	else
		return 0.0;
 }

double Link::calc_sumsq_input_output_linktimes ()
 {
	if (avgtimes->times.size() > 0)
	{
		double total = 0.0;
		/*
		vector <double>::iterator newtime=avgtimes->times.begin();
		vector <double>::iterator oldtime=histtimes->times.begin();
		for (newtime, oldtime; (newtime < avgtimes->times.end()) && (oldtime < histtimes->times.end()); newtime++, oldtime++)
		{
			if ((newtime < avgtimes->times.end()) && (oldtime < histtimes->times.end()) && ((*newtime) > 0))
			{
				total+= ((*newtime) - (*oldtime)) * ((*newtime) - (*oldtime)) ;
		
			}
		}
		*/
		for (int i=0; i<avgtimes->nrperiods; i++)
			total += (avgtimes->times [i] - histtimes->times [i])*(avgtimes->times [i] - histtimes->times [i]);
		return total;
	}
	else return 0.0;
}

bool Link::copy_linktimes_out_in()
{
	// 'safe' way of copying the output to input travel times. 
	// If no value in output (for certain time period), input value is kept. 
	// If no value in input, no value is copied from output. (to preserve same length)
	if (avgtimes->times.size() > 0)
	{
		for (int i=0; i<avgtimes->nrperiods; i++)
			histtimes->times [i] =time_alpha*avgtimes->times [i] + (1-time_alpha)* histtimes->times [i];
		return true;
	}
	else 
		return false;
}


// InputLink functions
InputLink::InputLink(int id_, Origin* out_)
{
    id=id_;
	out_node=out_;
	maxcap=65535; //
	length=0;
    nr_lanes=0;
    queue=new Q(maxcap, 1.0);
    histtimes=NULL;
    avgtimes=NULL;
}

void InputLink::reset()
{
	blocked_until=-1.0; // -1.0 = not blocked, -2.0 = blocked until further notice, other value= blocked until value
	nr_exits_blocked=0; // set by the turning movements if they are blocked

	queue->reset();
}

InputLink::~InputLink()
{
	//delete queue;
}

bool InputLink::enter_veh(Vehicle* veh, double time)
{
  veh->set_exit_time(time);
  veh->set_curr_link(this);
  veh->set_entry_time(time);
  return queue->enter_veh(veh);
}

Vehicle* InputLink::exit_veh(double time, Link* link, int lookback)
{
	ok=false;
	if (!empty())
	{
		Vehicle* veh=queue->exit_veh(time, link, lookback);
		if (queue->exit_ok())
		{
          ok=true;
			return veh;
		}
	}
    return NULL;
}


// Virtual Link functions

VirtualLink::~VirtualLink()

{
//	this->Link::~Link();
/*
	delete(queue);
#ifdef _COLLECT_TRAVELTIMES
	if (grid!=null )
		delete(grid);
#endif
 #ifdef _COLLECT_ALL
	if (grid!=null )
		delete(grid);
#endif
	if (histtimes!=NULL)
		delete (histtimes);*/
}

 VirtualLink::VirtualLink(int id_, Node* in_, Node* out_, int length_, int nr_lanes_, Sdfunc* sdfunc_) : Link(id_,in_,out_,length_,nr_lanes_,sdfunc_)
 {
   blocked=false;
   linkdensity=0.0;
   linkspeed=this->sdfunc->speed(linkdensity);
   freeflowtime=(length/(sdfunc->speed(0.0)));
   if (freeflowtime < 1.0)
      freeflowtime=1.0;
#ifdef _VISSIMCOM
   pathid=id;
#endif //_VISSIMCOM
 }

const  bool VirtualLink::full()
 {
  	return blocked;
 }

 const bool VirtualLink::full(double ) // double time unused
 {
   return blocked;
 }

 double VirtualLink::speed_density(double)    //unused double density
{
    return linkspeed;
}
 
 bool VirtualLink::enter_veh(Vehicle* veh, double time)
 {
  in_headways.push_back(time);
  return in_node->process_veh(veh,time);
  
 }

 bool VirtualLink::exit_veh(Vehicle* veh, double time)
 {
			double entrytime=veh->get_entry_time();
			double traveltime=(time-entrytime);
			//cout << "virtuallink::exit_veh: entrytime: " << entrytime << " exittime " << time << endl;
			avg_time=(nr_passed*avg_time + traveltime)/(nr_passed+1); // update of the average
			if ((curr_period+1)*(avgtimes->periodlength) < entrytime )
			{	
				if (tmp_avg==0.0)
					tmp_avg = histtimes->times [curr_period];
					//tmp_avg=freeflowtime;
//			 	avgtimes->times.push_back(tmp_avg);
				avgtimes->times [curr_period] = tmp_avg;
			 	curr_period++;
			 	tmp_avg=0.0;
			 	tmp_passed=0;
			}
			else
			{
				tmp_avg=(tmp_passed*tmp_avg + traveltime)/(tmp_passed+1); // update of the average			
				tmp_passed++;
			}
       out_headways.push_back(time);
			return true;	

  }


  void VirtualLink::write_in_headways(ostream & out)
  {
   for (list <double>::iterator iter=in_headways.begin(); iter != in_headways.end(); iter++)
   {
     out << (*iter) << endl;
   }
  }

void VirtualLink::write_out_headways(ostream & out)
  {
   for (list <double>::iterator iter=out_headways.begin(); iter != out_headways.end(); iter++)
   {
     out << (*iter) << endl;

   }
  }
