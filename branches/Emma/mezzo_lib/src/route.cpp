#include "route.h"
#include <algorithm>
#include <math.h>
#include "parameters.h"

template<class T>
struct compare
{
 compare(int id_):id(id_) {}
 bool operator () (T* thing)

 	{
 	 return (thing->get_id()==id);
 	}

 int id;
};

Route::Route(const int id_, Origin* const origin_, Destination* const destination_, vector <Link*> & links_):	id(id_), origin(origin_), destination(destination_),sumcost(0.0)
{
 	last_calc_time=0.0;
	links=links_;
	vector <Link*>::iterator iter = links.begin();
	for (iter; iter < links.end(); iter++)
	{
			Link* link=(*iter);
			link->register_route(this);
		//	linkmap [link->get_id()]=link;
	}
#ifdef _DEBUG_ROUTE 	
 	eout << "new route: rid,oid,did : lid* " << id << ","<< origin->get_id();
	eout << "," << destination->get_id() << " : ";
	for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
	{
		eout << (*iter)->get_id() << " ";
	}
	eout << endl;
#endif // _DEBUG_ROUTE		
	routeflows=vector <int> (theParameters->od_loadtimes.size(),0);
}



Route::Route(const int id_, Route* const route, const vector<Link*> & links_): id(id_)
{
	sumcost=0.0;
	last_calc_time=0.0;
	links=links_;
	origin=route->get_origin();
	destination=route->get_destination();
	vector<Link*> oldlinks=route->get_links();
	vector<Link*>::iterator stop=(find_if(oldlinks.begin(), oldlinks.end(), compare <Link> ((links_.front())->get_id()) ));
	if (stop > oldlinks.begin() )
		{
		stop--;
		for (vector<Link*>::iterator iter=stop;iter !=oldlinks.begin(); iter--)
			links.insert(links.begin(),(*iter));
		}
//	int me=(links.front())->get_id();
	if ( (links.front())->get_id() != (oldlinks.front())->get_id() )	
		links.insert(links.begin(),  oldlinks.front() );
	routeflows=vector <int> (theParameters->od_loadtimes.size(),0);
}

void Route::reset()
{
	sumcost=0.0;
	last_calc_time=0.0;
	prev_routeflows=routeflows;
	routeflows=vector <int> (theParameters->od_loadtimes.size(),0);

}

const ODVal Route::get_oid_did() const
  {return  ODVal(origin->get_id(), destination->get_id());}	

const int Route::get_od_period(const double time) const
{	
	int i = 0;
	int max_i=theParameters->od_loadtimes.size(); 
	for (i; i < max_i ; i++)
	{
		if (theParameters->od_loadtimes[i] > time)
			return (i-1); 
	}
	return max_i-1;
}

const int Route::get_abs_diff_routeflows() const
{
	int abs_diff=0;
	int i = 0;
	int max_i=min (routeflows.size(), prev_routeflows.size());
	for (i ;i < max_i; i++)
	{
		abs_diff += abs(routeflows [i]-prev_routeflows [i]);
	}
	return abs_diff;
}

const int Route::get_sum_prev_routeflows() const
{
	int sum=0;
	int i = 0;
	int max_i= prev_routeflows.size();
	for (i ;i < max_i; i++)
	{
		sum+= prev_routeflows [i];
	}
	return sum;

}

const int Route::get_sum_routeflows() const
{
	int sum=0;
	int i = 0;
	int max_i= routeflows.size();
	for (i ;i < max_i; i++)
	{
		sum+= routeflows [i];
	}
	return sum;

}

void Route::register_veh_departure(const double time)
{	
	int period = get_od_period(time);
	routeflows [period] = routeflows [period] +1;
}

 const bool Route::check (const int oid, const int did) const 
	{ return ( (origin->get_id()==oid) && (destination->get_id()==did) );}

 const bool Route::less_than (const Route*const  route) const
	 // returns true if origin_id of this route is less than origin_id of the "route" parameter, or
	 // if the origins are equal, if the destination_id is less than that of the "route" parameter provided
 {
	 int id_or1=origin->get_id();
	 int id_or2= route->get_origin()->get_id();
	 if (id_or1 > id_or2 )
		 return false;
	 else
	 {
		if (id_or1 < id_or2)
			return true;
		else // same origins
		{
			if (destination ->get_id() >= route->get_destination()->get_id())
				return false;
			else
				return true;
		}
	 }
 }

 const vector<Link*>  Route::get_upstream_links(const int link_id)const 
 {
		return (vector <Link*> (links.begin(), find_if (links.begin(),links.end(), compare <Link> (link_id))));
 }

 const vector<Link*> Route::get_downstream_links(const int link_id) const 
 {
	    return (vector <Link*> (find_if (links.begin(),links.end(), compare <Link> (link_id)),links.end()  ));
 }


 void Route::set_selected(const bool selected) // sets the links' selected attribute
 {
	 vector <Link*>::iterator iter = links.begin();
	 for (iter;iter < links.end(); iter++)
	 {
		 (*iter)->set_selected(selected);
	 }
 }

#ifndef _NO_GUI
 void Route::set_selected_color(const QColor & selcolor)
 {
	vector <Link*>::iterator iter = links.begin();
	 for (iter;iter < links.end(); iter++)
	 {
		 (*iter)->set_selected_color(selcolor);
	 }
 }
#endif

 const  bool Route::equals (const  Route& route) const  // returns true if same route 
 {
	 ODVal val=route.get_oid_did();
	 if ((val.first != origin->get_id()) || (val.second != destination->get_id()))
		 return false;
	 else
		return ( (route.get_links())==(get_links()) );
 }
 
  		
Link* const Route::nextlink(Link* const currentlink) const
{
 vector<Link*>::const_iterator iter=find(links.begin(), links.end(), currentlink);
 iter++;
  if (iter<links.end())
 {
 	return *iter;
 }
 eout << "Route::nextlink: error! there is no next link! " << endl;
 return NULL;
}

vector <Link*>::const_iterator Route::nextlink_iter(Link* const currentlink) 
{
	vector<Link*>::const_iterator iter=find(links.begin(), links.end(), currentlink);
	iter++;
	if (iter==links.end())
		eout << "Route::nextlink_iter: error! there is no next link! route id "<< id << endl;
	return iter;
}
	



const bool Route::has_link(const int lid) const 
{
  return ( (find_if (links.begin(),links.end(), compare <Link> (lid))) < links.end() ); // the link exists
/*	if (linkmap.count(lid))
		return true;
	else
		return false;*/
}


const bool Route::has_link_after(const int lid, const int curr_lid) const 
{
  vector <Link*>:: const_iterator iter =(find_if (links.begin(),links.end(), compare <Link> (curr_lid))) ; // find curr_lid
  iter++;
   if (iter < links.end())
	   return ((find_if (iter,links.end(), compare <Link> (lid))) < links.end() ); // the link lid exists after curr_lid
   return false;
}

const double Route::cost(const double time) 
{
	if ((sumcost>0.0) & (last_calc_time+theParameters->update_interval_routes < time))
		return sumcost;// this means the cost has already been calculated once this update interval, return cached value.
	else
	{
		double temp_time=time;
       last_calc_time=time;
		for(vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
		{
	#ifdef _DISTANCE_BASED
			sumcost+=(*iter)->get_length();	
	#else
			
			 LinkTime* lt=(*iter)->get_histtimes();
			 if     (lt)
			 	temp_time+=lt->cost(temp_time);
			 else
			 	temp_time+=(*iter)->get_freeflow_time();	
			//sumcost+=(*iter)->get_hist_time();
	#endif //_DISTANCE_BASED
		}
		sumcost=temp_time-time;
	   return sumcost;
	}
}

const double Route::utility (const double time) 
{
#ifdef _MULTINOMIAL_LOGIT
	return exp(theParameters->mnl_theta*(cost(time)) );
#else
	return pow (cost(time),theParameters->kirchoff_alpha);
#endif
}

/**
* compute the length of the route
**/
const int Route::computeRouteLength() const 
{	
	int routelength=0;
	for(unsigned i=0; i<links.size();i++)
		routelength+=links[i]->get_length();
	return routelength;
}

void Route::write(ostream& out) const
{
	out << "{ " << id << " " << origin->get_id() << " " << destination->get_id() << " " << links.size() << "{";
	for (vector <Link*>::const_iterator iter=links.begin(); iter<links.end();iter++)
	{
	 	out <<" " <<(*iter)->get_id();
	}
	out << "} }" << endl;
}

void Route::write_routeflows(ostream &out) const
{
	out << id << '\t' ;
	vector <int>::const_iterator iter;
	for ( iter = routeflows.begin(); iter != routeflows.end(); iter++)
	{
		out << (*iter) << '\t' ;
	}
	out << endl;


}

// EmmaRoute

	EmmaRoute::EmmaRoute(const int id_, Origin*const  origin_, Destination* const destination_, vector <Link*> & links_) :
	  Route (id_, origin_, destination_, links_) {links.push_back(origin_->get_links().front());} // EMMAROUTE !!! TO FIX FOR OTHER NETWORKS!!!
	/*	
	Link* const EmmaRoute::nextlink(Link* const currentlink) const
	{

		return NULL;
	
	}*/

	/*vector <Link*>::const_iterator EmmaRoute::nextlink_iter(Link* const currentlink)  //!< returns const_iterator to the next link of the route, given currentlink.

	{
		vector<Link*>::const_iterator iter=find(links.begin(), links.end(), currentlink);
		return ++iter;
	}*/

	void EmmaRoute::generate_nextlink(Link* const currentlink)
	{

		vector<Link*>::const_iterator iter=find(links.begin(), links.end(), currentlink);
		int cur_id=currentlink->get_id();
		int nextlinkid= probas.sample_nextlink(cur_id);
		links.push_back(probas.linkmap [nextlinkid]);
	}

	void Route::generate_nextlink(Link* const currentlink) // Dummy
	{}