#include "node.h"
#include <algorithm>

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

Coord::Coord():x(0.0),y(0.0)
{}

Coord::Coord (double x_ , double y_):x(x_),y(y_)
{}


Node::Node(int id_): id(id_)
{}

Node::~Node() {}

const int Node::get_id()
{
	return id;
}

void Node::setxy(double x, double y)
{
	position.x=x;
	position.y=y;
}

const Coord Node::getxy()
{
	return position;
}

#ifndef _NO_GUI
void Node::set_icon(NodeIcon* icon_)
{
	icon=icon_;
}
#endif // _NO_GUI

bool  Node::process_veh(Vehicle* , double )       //unused Vehicle* veh, double time
{
	return false;
}



Origin::Origin(int id_):Node(id_)
{
  inputqueue=new InputLink(id_,this);
  incident=false;
  incident_link=-1;
}

Origin::~Origin()
{
	if (inputqueue)
		delete inputqueue;
	// TAKE OUT THE OACTIONS (COMMENT THEM OUT SO WE CAN USE THEM LATER
	for (vector <Oaction*>::iterator iter=oactions.begin();iter<oactions.end();)
	{	
		delete (*iter);
		iter=oactions.erase(iter);	
	}
	for (vector <OServer*>::iterator iter1=servers.begin(); iter1<servers.end(); )
	{
	
	  delete (*iter1);
	  iter1=servers.erase(iter1);
	}
	currentperiod = 0;
}

void Origin::get_link_rates()
{
	
	vector<rateval> route_rates;
	for (vector<ODpair*>::iterator iter=odpairs.begin();iter<odpairs.end(); iter++)
	{
		route_rates=	(*iter)->get_route_rates();
		for (vector<rateval>::iterator iter2=route_rates.begin();iter2<route_rates.end();iter2++)
		{		
			linkrate temp ( (*iter2).first->firstlink(), (*iter2).second );
			link_rates.push_back(temp);
		}
		
	}
   	// !!! !!! Sort out the mess of link*, rate that we have in te vector of link_rates !! !! !! !!
 	vector<linkrate>::iterator temp=link_rates.begin(); // temp
	vector<linkrate>::iterator start=temp; // the startpoint
//	bool stop=false;
	while (temp < link_rates.end())
	{
		start=temp;
		start++; // start comparing from the NEXT position;
		for (vector<linkrate>::iterator iter=start; iter<link_rates.end();) // compare all link-rates from start
		{
			if ( (*iter).first->get_id() == (*temp).first->get_id() ) // if iter has the same link as temp
			{
				(*temp).second+=(*iter).second; // then add the rate to that of temp
				iter=link_rates.erase(iter); // and delete the rate from the list (automatically advancing the iterator)
			}
			else
				iter++;
		}
		temp++; // look at the next
	}

}




void Origin::register_links(vector <Link*> links)
/*
 	Registers all links that are outgoing from this origin, creates the Oservers with the right rates
	which are provided by the OD pairs for each route, and creates the Oactions that will transfer the
	vehicles from the InputQueue to the outgoing links, hopefully in a somewhat realistic manner (one action per
	link, so there'll be ' a whole lotta action' going on with actions creating vehicles, actions inserting vehicles &c
*/
{
	// register the links
	for (vector <Link*>::iterator iter=links.begin(); iter < links.end(); iter++)
 	{
      	if ((*iter)->get_in_node_id()==id)
 		{
			outgoing.insert(outgoing.begin(), *iter);
   		#ifdef _DEBUG_NODE
 	 			cout << "Registered link " << (*iter)->get_id() << " @ origin " << id << endl;
 	 		#endif //_DEBUG_NODE
		}
 	 }	
}

void Origin::initialise() // initialises the Oservers for each outgoing link based on the rates from the OD pairs
{
   get_link_rates();    // get the rates per outgoing link from the OD pairs
	// if all links have a route starting there then (link_rates.size()==outgoing.size()) should be true
#ifdef _DEBUG_NODE
	cout << "number of outgoing links : " << outgoing.size() << endl;
	cout << "number of link rates registered " << link_rates.size() <<endl;
#endif //_DEBUG_NODE
	for (vector<linkrate>::iterator iter=link_rates.begin();iter<link_rates.end();iter++) // make the oactions for each link in link_rates
	{
		double mu=(3600.0/(*iter).second);
		#ifdef _DEBUG_NODE
			cout << " Origin::initialise mu = " << mu << endl;
		#endif //_DEBUG_NODE	
		servers.insert (servers.begin(),new OServer(3000, 2,mu , 0.5));
		for (int i=0;i<((*iter).first->get_nr_lanes());i++)
		{				
 			oactions.insert(oactions.begin(),new Oaction((*iter).first,this,servers.front()));  //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    	}
	}
}

bool Origin::execute(Eventlist* eventlist, double time)
{
#ifdef _DEBUG_NODE
	cout << "Origin execute" << endl;
#endif //_DEBUG_NODE
	bool ok=true;
 	for (vector<Oaction*>::iterator iter=oactions.begin(); iter<oactions.end(); iter++)
		ok=ok && ( (*iter)->execute(eventlist, time) );	
	return ok;
}

/*
bool Origin::insert_veh(Vehicle* veh, double time)
{
	if (!inputqueue->full())
	{
		return inputqueue->enter_veh(veh, time);
	
	}
	else
		return false;
}

bool Origin::transfer_veh(Link* link, double time)
{
	if (!(inputqueue->empty()))
	{
		if (!(link->full()))
		{
			bool ok=false;
			Vehicle* veh=inputqueue->exit_veh(time, link, 600);
			
			ok=inputqueue->exit_ok();
			if (ok)
			{
				veh->set_entered();
				if (incident)
					link->receive_broadcast(veh,incident_link,incident_parameters);
				return link->enter_veh(veh,time);
			}
		}
	}
return false;
}
*/
void Origin::write_v_queues(ostream& out)
{
	for (unsigned int i = 0 ; i < v_queue_lengths.size(); i++)
	{
		if (v_queue_lengths [i] > 0)
		{
			out << "{\t" << i << '\t' << id << '\t' << v_queue_lengths[i] << "\t}" << endl;
		}
	}
}

bool Origin::insert_veh(Vehicle* veh, double time)
{
	if (veh && !(inputqueue->full()) )
	{
		Link* link=veh->get_route()->firstlink();
		if (!link)
		{
		 cout << " first link is corrupt " << endl;
		 return false;		
		}
		
		// 2005-11-24 added logging of virtual queue lengths
		if (time >= ((currentperiod+1)*theParameters->ass_od_period)  )
		{
			v_queue_lengths.push_back(inputqueue->size());
			currentperiod++;
		}

		if (link->full() )
		{
			if (  inputqueue->enter_veh(veh,time) ) 
			{
				/*
				if (!((inputqueue->size()) % 100) )
					cout << time << " : At origin " << id << " virtual queue is " << inputqueue->size() << endl;
					*/
				
				return true;
			}
			else
			{
			 	cout << " Origin:: insertveh inputqueue->enterveh doesnt work " << endl;
	    		return false;
			}
		}	
       else
       {
        	if (inputqueue->empty())
        	{
        		
				if (link->enter_veh(veh,time))
                {
					if (incident)
						link->receive_broadcast(veh,incident_link,incident_parameters);
					veh->set_entered();	
        			return true;
        		}
				else
        		{
	    	   		cout << " Origin:: insertveh link->enterveh doesnt work " << endl;
			      	return false;
	        	}
        	}
        	else
        	{
        		if (inputqueue->enter_veh(veh,time))
	        		if (transfer_veh(link,time))
	        			return true;
	        		else
	        			{
	    	   			  cout << " Origin:: insertveh transferveh doesnt work " << endl;
			        		return false;
	        			}
	        	else
	        	{
			  		cout << " Origin:: insertveh inputqueue->enterveh doesnt work " << endl;
	        		return false;
	      		}
        	}
       }
  	}
  	else
  	{
  		cout << " Origin:: insertveh either no veh, or input queue full " << endl;
  		return false;
  	}
}

bool Origin::transfer_veh(Link* link, double time)
{
	
	if (link)
	{
	 	if (inputqueue->empty() || link->full())
	 		return true;
	 	else
	 	{
	 		bool stop=false;
	 		while (!(stop))
	 		{
	 	 		if (inputqueue->empty() || link->full())
					 		return true;
	 	 		Vehicle* veh=inputqueue->exit_veh(time, link, inputqueue->size());			
				if (inputqueue->exit_ok())
				{
				 	if (veh)
				 	{
				 	 	
						if (link->enter_veh(veh,time))
						{
							veh->set_entered();
							if (incident)
								link->receive_broadcast(veh,incident_link,incident_parameters);
					    	 //return true;
						}
						else
						{
							cout << "transfer: could not enter vehicle in link " << endl;
							return false;
						}
				 	}
				 	else
			   	 	{
					    cout << "transfer: vehicle disappeared " << endl;
						return false;
					}
				}
				else
					stop=true;
			}
			return true;
	 	}	
	}
	else
	{
		cout << "transfer: no valid link " << endl;
		return false;
	}
}


void Origin::broadcast_incident_start(int lid, vector <double> parameters)
{
	incident_parameters=parameters;
  incident_link=lid;
  incident=true;
}

void Origin::broadcast_incident_stop(int)          // unused int lid
{
	incident=false;
}



void Origin::add_odpair(ODpair* odpair)
{
 odpairs.insert(odpairs.begin(),odpair);
}

void Junction::register_links(vector <Link*> links)
{
	// register the links
	for (vector <Link*>::iterator iter=links.begin(); iter < links.end(); iter++)
 	{
      	if ((*iter)->get_in_node_id()==id)
 		{
			outgoing.insert(outgoing.begin(), *iter);
           #ifdef _DEBUG_NODE
 	 			cout << "Registered outgoing link " << (*iter)->get_id() << " @ junction " << id << endl;
 	 		#endif //_DEBUG_NODE
		}
		else if ((*iter)->get_out_node_id()==id)
		{
			incoming.insert(incoming.begin(), *iter);
           #ifdef _DEBUG_NODE
 	 			cout << "Registered incoming link " << (*iter)->get_id() << " @ junction " << id << endl;
 	 		#endif //_DEBUG_NODE
		}
 	 }	
}

Destination::Destination (int id_, Server* server_) : Node(id_)
{
 	server=server_;
}


Destination::Destination(int id_):Node(id_)
{
	server=new Server(-20000, 0, 0.1, 0.5, 0.001); //STANDARD OUTPUT SERVER
	// REPLACED BY SPECIFIC SERVER! see above :-) This method is kept as default & for older networks
}


Destination::~Destination()
{
  if (server->get_id()==-20000) // created locally
		delete server;
	for (vector <Daction*>::iterator iter=dactions.begin();iter<dactions.end();)
	{	
		delete (*iter);
		iter=dactions.erase(iter);	
	}
}

void Destination::register_links(vector <Link*> links)
{
		for (vector <Link*>::iterator iter=links.begin(); iter < links.end(); iter++)
 	{
 	      	if ((*iter)->get_out_node_id()==id)
 		{
 			incoming.insert(incoming.begin(), *iter);
      for (int i=0; i< (*iter)->get_nr_lanes(); i++)
	    	dactions.insert(dactions.begin(),new Daction((*iter),this,server)); // CHANGED 2003-10-10: 1 for each lane
 		#ifdef _DEBUG_NODE
 	 		cout << "Registered link " << (*iter)->get_id() << " @ dest " << id << endl;
 	 	#endif //_DEBUG_NODE
 	 	}
 	}
}

bool Destination::execute(Eventlist* eventlist, double time)
{
#ifdef _DEBUG_NODE
	cout << "Destination execute" << endl;
#endif //_DEBUG_NODE
	bool ok=true;
 	for (vector<Daction*>::iterator iter=dactions.begin(); iter<dactions.end(); iter++)
		ok=ok && ( (*iter)->execute(eventlist, time) );	
	return ok;
}


// BoundaryOut functions


BoundaryOut::BoundaryOut (int id_) : Junction (id_)
{blocked_=false;
#ifdef _MIME
	signatures.clear();
#endif //_MIME
}

BoundaryOut::~BoundaryOut ()
{
	this->Junction::~Junction();
}



bool BoundaryOut::process_veh(Vehicle* veh, double time)
{
	#ifdef _MIME
		Link* lptr=NULL;
		Link* clptr=NULL;
		clptr=veh->get_curr_link();
		lptr=veh->nextlink();
		if (lptr)
		{
 				// make the signature and put it in the BoundaryOut
				int tmpori = lptr->get_in_node_id();     // temp origin for MITSIM
				int tmpdest = lptr->get_out_node_id(); // temp destination for MITSIM
				int tmppath = lptr->get_id(); // temp path for MITSIM
				double spd=theParameters->mime_queue_dis_speed;
				if (clptr->size() < theParameters->mime_min_queue_length)
					spd=clptr->speed(time);
				Signature* sig=new Signature(veh->get_id(), static_cast<int>(spd), time,time,veh->get_start_time(), veh->get_meters(),
					veh->get_oid(),veh->get_did(),veh->get_type(), veh->get_length(), (veh->get_route()->get_id()),tmppath, tmpori, tmpdest);
#ifdef _VISSIMCOM
				sig->v_parkinglot=lptr->parkinglot;
				sig->v_path=lptr->pathid;
#endif // _VISSIMCOM
				
				add_signature(sig);
		}
	#endif//_MIME
		recycler.addVehicle(veh);
		return true;
}

#ifdef _MIME
void BoundaryOut::add_signature (Signature* sig)
{
 	signatures.insert (signatures.end(),sig);
}

vector <Signature*> & BoundaryOut::get_signatures() 
{
	//cout << " returning signatures " << signatures.size() << endl;
	return signatures;
}

#endif //_MIME


void BoundaryOut::block(int code,double speed) // spread the news to the virtual links (setting full to true/false)
{
   vector <VirtualLink*>::iterator iter;
  for ( iter=vlinks.begin();iter<vlinks.end();iter++)
  {
   		(*iter)->block(code);
       if (code < 0)   // -1 = block
         blocked_=true;
       else
       {
         blocked_=false;
         (*iter)->set_density(code);
         (*iter)->set_speed(speed);
         cout << " set the density " << code << " for virtual link " << (*iter)->get_id() << endl; 
       }
    }
}


#ifdef _PVM
int BoundaryOut::send_message(PVM* com)

{
		(*com) << id; // first is always the node id
		int sigcount=signatures.size(); // put number of signatures
		(*com) << sigcount ;
		// put the signatures
		for (vector<Signature*>::iterator iter=signatures.begin(); iter<signatures.end(); )
		{
		 	(*iter)->send(com); // send the signature
		 	delete (*iter);
		 	iter=signatures.erase(iter);        // and delete it
		}
		return sigcount;
}
#endif //_PVM
// NOTE BoundaryOut::sendmessage, BoundaryIn::sendmessage and BoundaryIn::receivemessage need to be reimplemented for
// the VISSIM case.



// BoundaryIn functions
BoundaryIn::BoundaryIn (int id_):Origin(id_)
{
	lastveh=NULL;
}

BoundaryIn::~BoundaryIn ()
{
//  this->Origin::~Origin();
}

#ifdef _VISSIMCOM
	// put here the equivalent of the PVM functions
double BoundaryIn::get_speed(double time) // sends the speed assigned to the last vehicle that entered
{
	double speed=30.0;
	//double distance=500.0;
	if (lastveh!=NULL)
	{
	    Link* lpt =lastveh ->get_curr_link();
	    if (lpt->full(time))
	    {
	    	speed=0.0;
	    	//distance=0.0;	
	    }	
	 	 else
		 {
			speed = ((lastveh ->get_curr_link())->speed(time));
	//		distance =(time -  lastveh->get_entry_time() )* speed;
		}
	}
	else
	{
	    speed = 30.0;
//	    distance = 500;
	}
	return speed;
}

#endif //_VISSIMCOM


#ifdef _PVM
int BoundaryIn::send_message(PVM* com, double time)
{
	double speed, distance;
	(*com) << id; // first is always the node id
	if (lastveh!=NULL)
	{
	    Link* lpt =lastveh ->get_curr_link();
	    if (lpt->full(time))
	    {
	    	speed=0.0;
	    	distance=0.0;	
	    }	
	 //   cout <<" looking for speed at link number : " << lpt->get_id() << endl;
		 else
		 {
			speed = ((lastveh ->get_curr_link())->speed(time));
			distance =(time -  lastveh->get_entry_time() )* speed;
		}
//		cout << " Mezzo boundary in:: speed " << speed << " distance " << distance << " time now " << time << " entrytime  " << lastveh->get_entry_time() << endl;
	}
	else
	{
	    speed = 30.0;
	    distance = 500;
	}
	(*com) << speed ;
	(*com) << distance;
 	return 1;
}
#endif //_PVM

#ifdef _PVM
bool BoundaryIn::receive_message(PVM* com)
/* CHANGED: from 1 message per BoundaryIn per timestep into:
	1 message per timestep, that includes ALL BoundaryIn nodes.
	*/

{
	    int rid, signr;
	    bool ok=false;
		(*com) >> rid; // first is always the node id
		if (id!=rid)
			return false;
		(*com) >> signr; // number of signatures
		if (signr > 0)
			for (int i=0; i<signr;i++)
			{
			  // make the signatures
			  Signature* sig=new Signature(com);
			  ok = newvehicle(sig);
			  delete sig;
			}
		// put the signatures
		return ok;
}
#endif //_PVM

#ifdef _MIME
bool BoundaryIn::newvehicle(Signature* sig)
 
{
 			ODpair* odptr=NULL;
			  //find the odpair
			 for (vector <ODpair*>::iterator iter=ods->begin();iter < ods->end(); iter++)
			  {
			  		odval odvalue=(*iter)->odids();
			   		if ((odvalue.first==sig->origin) && (odvalue.second==sig->destination))
			   		{
			    		odptr=(*iter);
			    		break;
			   		}
			  }
			  // find the route
			  if (odptr)
			  {
			  		//cout << "BoundaryIn:: found od" << endl;
			  		Route* rptr=*(find_if(routes->begin(), routes->end(), compare <Route> (sig->path)));
			  		if (rptr)
			  		{
			  			//create vehicle
			  			Vehicle* veh= new Vehicle(sig->id,sig->type,sig->length,rptr,odptr,sig->starttime);
			  			veh->set_entry_time(sig->entrytime);
			  			veh->set_meters(sig->meters);
			  			veh->set_entered();
			  			// register the time on the virtual link
			  		   VirtualLink* vlptr=NULL;
			  		   Link* lptr=NULL;
			  		   for (vector<VirtualLink*>::iterator iter=vlinks.begin(); iter<vlinks.end(); iter++)
			  		   {
			  		     if ((*iter)->get_in_node_id()==sig->tmporigin)
			  		     	vlptr=*iter;
			  		   }
			  			if (vlptr)
			  			{
				  			vlptr->exit_veh(veh,sig->timestamp);
			  			    lptr=rptr->nextlink(vlptr);
			  			    if (lptr)
			  			    {
				  			    lptr->enter_veh(veh,sig->timestamp);   // and enter vehicle
				  			    lastveh=veh; // keep track of the last vehicle entered, for MITSIM/VISSIM
			 					return true;
			 				}
			 				else
			 					return false;
			 			}
			 			else
			 				return false;
			 		}
			 		else
			 			return false;
			 	}
			 	else
			 		return false;
}
#endif //_MIME
// Junction functions

Junction::Junction(int id_):Node(id_) {}

Oaction::Oaction(Link* link_, Origin* origin_, OServer* server_): link(link_), origin(origin_), server(server_)

{}

Oaction::~Oaction()
{
//		delete server;
}

bool Oaction::execute(Eventlist*, double) // unused    Eventlist* eventlist, double time
// not used anymore. Vehicles are put directly onto the link by OD action
{
/*
// process vehicles if any
	bool exec_ok=origin->transfer_veh(link,time);
	if (!exec_ok)
		cout << "Oaction:: couldn't transfer vehicle" << endl;
	// get new time from server
	double new_time=server->next(time);
   eventlist->add_event(new_time,this); */

   // taken out 2002-11-26 to eliminate the vehicle loading process. Now OD pairs generate traffic that is loaded directly onto the links.
	return true;
}



Daction::Daction(Link* link_, Destination* destination_, Server* server_):link(link_),
	destination(destination_), server(server_)
{}

Daction::~Daction()
{}

bool Daction::execute(Eventlist* eventlist, double time)
{
	// process vehicles if any
	double new_time=0.0;
	bool exec_ok=process_veh(time);
	// get new time from server or from the link.
	if (!exec_ok)
		new_time=link->next_action(time);
	else
	  new_time=server->next(time);
#ifdef _DEBUG_NODE
	  cout <<"Daction::execute...result: " << exec_ok;
   	  cout << " ...next Daction @ " << new_time << endl;
#endif //_DEBUG_NODE
	// book myself in Eventlist with new time;
	eventlist->add_event(new_time,this);
	return true;
}

bool Daction::process_veh(double time)
{
  // do all sorts of things and finally...
  Vehicle* veh=link->exit_veh(time) ;
  bool ok=link->exit_ok();	
  if (ok)
  {
 #ifdef _DEBUG_NODE
 	cout << "Daction::process_veh detete veh" << endl;
 	cout << "Vehicle exit time " << (veh->get_exit_time());
   cout << "link exit ok?: " << link-> exit_ok() << endl;
 	cout << "link id "<<link->get_id()<< endl;
 #endif //_DEBUG_NODE
 		veh->report(time);
  	recycler.addVehicle(veh);
  }
  return ok;
}

 /*
// BOaction funcitons

BOaction::BOaction (Link* link_, BoundaryOut* boundaryout_, Server* server_):Daction(link_,boundaryout_,server_)
{
	boundaryout=boundaryout_;
}

BOaction::~BOaction()
{}

bool BOaction::process_veh(double time)
{
	if (boundaryout->blocked())
	{
	  cout << "exit blocked " << endl;
	  return false;	
	}
	else
	{
 		Vehicle* veh=link->exit_veh(time) ;
  		bool ok=link->exit_ok();
  		if (ok)
  		{
			 #ifdef _DEBUG_NODE
			cout << "BOaction::process_veh detete veh" << endl;
			cout << "Vehicle exit time " << (veh->get_exit_time());
		  	cout << "link exit ok?: " << link-> exit_ok() << endl;
 			cout << "link id "<<link->get_id()<< endl;
			 #endif // _DEBUG_NODE
			veh->report(time);
			Link* lptr=NULL;
			lptr=veh->nextlink();
			if (lptr)
			{
 				// make the signature and put it in the BoundaryOut
				int tmpori = lptr->get_in_node_id();     // temp origin for MITSIM
				int tmpdest = lptr->get_out_node_id(); // temp destination for MITSIM
				Signature* sig=new Signature(veh->get_id(), link->speed(time), time,time,veh->get_start_time(), veh->get_meters(),
					veh->get_oid(),veh->get_did(),veh->get_type(), veh->get_length(), (veh->get_route()->get_id()), tmpori, tmpdest);
				boundaryout->add_signature(sig);
			}
		delete veh; // kill kill kill the vehicle. Later we can recycle.
		}
		return ok;
	}	
}
    */
