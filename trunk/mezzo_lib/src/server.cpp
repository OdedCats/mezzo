#include "server.h"
#include "Math.h"
#include <iostream>


Server::Server(const int id_, const int type_, const double mu_, const double sd_, const double delay_): id(id_), type(type_), mu(mu_),
		sd(sd_), delay(delay_)
{
	random=new (Random);
	if (randseed != 0)
	   random->seed(randseed);
	else
		random->randomize();	
}

Server::~Server()
{
  #ifdef _DEBUG_SERVERS
  eout << " deleting server id " << id << endl;
  #endif // _DEBUG SERVERS
	delete (random);
}

void Server::reset()
{
	if (randseed != 0)
	   random->seed(randseed);
	else
		random->randomize();
}
const double Server::next(const double time)
{
 	double temp= _MAX(min_hdway,random->nrandom(mu,sd));
 	return time+temp;

}


const int Server::get_id() const
{
	return id;
}

const double Server::get_mu() const
{                	
	return mu;
}

void Server::set_mu(const double mu_)
{
	mu=mu_;
}

const double Server::get_sd() const
{
	return sd;
}

void Server::set_sd(const double sd_)
{
	sd=sd_;
}

ODServer::ODServer(const int id_, const int type_, const double mu_, const double sd_):Server(id_,type_,mu_,sd_,0.0)
{}


const double ODServer::next(const double time)
{
	if (theParameters->od_servers_deterministic)
		return (time+mu);
	else
		return time+(random->rrandom(mu));

}

OServer::OServer(const int id_, const int type_, const double mu_, const double sd_):Server(id_,type_,mu_,sd_,0.0)
{
	if (mu<=cf_hdway)
		alpha=1.0; // All traffic is in carfollowing
   	else
   	{
		alpha=cf_hdway/mu; // proportion of bound traffic
		mu_unbound=(mu-alpha*cf_hdway)/(1-alpha)  ;
	}
	lanes=1.0;
}

const double OServer::next(const double time)
{
	double bound=random->urandom();
	double temp;
	if (bound < alpha)
	{
		temp=_MAX(min_hdway,random->nrandom(cf_hdway,sd_cf_hdway));  //
		//eout << "bound : " << temp << endl;
	}
	else	
	{
		temp=max_cf_hdway+(random->rrandom(mu_unbound-max_cf_hdway)); // shifted neg exp.
		//eout << "unbound : " << temp << endl;
	}
	temp=temp/lanes; // make it per lane.
	return time+temp;
}


ChangeRateAction::ChangeRateAction (Eventlist* eventlist_, const double time_, Server* server_, const double mu_, const double sd_)
{
   mu=mu_;
   sd=sd_;
   server=server_;
   time=time_;
   eventlist = eventlist_;
   eventlist->add_event(time, this);
}

void ChangeRateAction::reset()
{
	eventlist->add_event(time, this);
}
		
const bool ChangeRateAction::execute(Eventlist* , const double)     //unused  Eventlist* eventlist, double time
{
   if (mu > 0.0)
   		server->set_mu(mu);
   if (sd > 0.0)
   		server->set_sd(sd);
 return true;
}

