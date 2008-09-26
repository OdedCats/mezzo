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

Route::Route(int id_, Origin* origin_, Destination* destination_, vector <Link*> links_):	id(id_), origin(origin_), destination(destination_),sumcost(0.0)
{
 	last_calc_time=0.0;
	links=links_;
	vector <Link*>::iterator iter = links.begin();
	for (iter; iter < links.end(); iter++)
	{
			Link* link=(*iter);
			link->register_route(this);
			linkmap [link->get_id()]=link;
	}
#ifdef _DEBUG_ROUTE 	
 	cout << "new route: rid,oid,did : lid* " << id << ","<< origin->get_id();
	cout << "," << destination->get_id() << " : ";
	for (vector<Link*>::iterator iter=links.begin();iter<links.end();iter++)
	{
		cout << (*iter)->get_id() << " ";
	}
	cout << endl;
#endif // _DEBUG_ROUTE		
}



Route::Route(int id_, Route* route, vector<Link*> links_): id(id_)
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
}

void Route::reset()
{
	sumcost=0.0;
	last_calc_time=0.0;

}

odval Route::get_oid_did()
  {return odval(origin->get_id(), destination->get_id());}		

 bool Route::check (int oid, int did)
	{ return ( (origin->get_id()==oid) && (destination->get_id()==did) );}

 bool Route::less_than (Route* route) 
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

 vector<Link*> Route::get_upstream_links(int link_id)
 {
		return (vector <Link*> (links.begin(), find_if (links.begin(),links.end(), compare <Link> (link_id))));
 }

 vector<Link*> Route::get_downstream_links(int link_id)
 {
	    return (vector <Link*> (find_if (links.begin(),links.end(), compare <Link> (link_id)),links.end()  ));
 }


 void Route::set_selected(bool selected) // sets the links' selected attribute
 {
	 vector <Link*>::iterator iter = links.begin();
	 for (iter;iter < links.end(); iter++)
	 {
		 (*iter)->set_selected(selected);
	 }
 }

#ifndef _NO_GUI
 void Route::set_selected_color(QColor selcolor)
 {
	vector <Link*>::iterator iter = links.begin();
	 for (iter;iter < links.end(); iter++)
	 {
		 (*iter)->set_selected_color(selcolor);
	 }
 }
#endif

 bool Route::equals (Route& route) // returns true if same route 
 {
	 odval val=route.get_oid_did();
	 if ((val.first != origin->get_id()) || (val.second != destination->get_id()))
		 return false;
	 else
		return ( (route.get_links())==(get_links()) );
 }
 
  		
Link* Route::nextlink(Link* currentlink)
{
 vector<Link*>::iterator iter=find(links.begin(), links.end(), currentlink);
 iter++;
  if (iter<links.end())
 {
 	return *iter;
 }
 cout << "Route::nextlink: error! there is no next link! " << endl;
 return NULL;
}




bool Route::has_link(int lid)
{
  //return ( (find_if (links.begin(),links.end(), compare <Link> (lid))) < links.end() ); // the link exists
	if (linkmap.count(lid))
		return true;
	else
		return false;
}


bool Route::has_link_after(int lid, int curr_lid)
{
  vector <Link*>:: iterator iter =(find_if (links.begin(),links.end(), compare <Link> (curr_lid))) ; // find curr_lid
  iter++;
   if (iter < links.end())
	   return ((find_if (iter,links.end(), compare <Link> (lid))) < links.end() ); // the link lid exists after curr_lid
   return false;
}

double Route::cost(double time)
{
	if ((sumcost>0.0) & (last_calc_time+theParameters->update_interval_routes < time))
		return sumcost;// this means the cost has already been calculated once this update interval
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

double Route::utility (double time)
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
int Route::computeRouteLength()
{	
	int routelength=0;
	for(unsigned i=0; i<links.size();i++)
		routelength+=links[i]->get_length();
	return routelength;
}

void Route::write(ostream& out)
{
	out << "{ " << id << " " << origin->get_id() << " " << destination->get_id() << " " << links.size() << "{";
	for (vector <Link*>::iterator iter=links.begin(); iter<links.end();iter++)
	{
	 	out <<" " <<(*iter)->get_id();
	}
	out << "} }" << endl;
}

