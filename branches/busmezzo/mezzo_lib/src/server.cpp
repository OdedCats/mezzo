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
  cout << " deleting server id " << id << endl;
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

double Server::next(const double time)
{
	if (theParameters->server_type == 3)
	{
		double result = mu + random->lnrandom(delay, sd);
		//double delay_std1 = theParameters->sd_server_scale * sqrt(delay_std) * sqrt (mu + delay); // This is specifically for the Tel-Aviv case study use - delay_std = total driving time for this busline
		return (time + result) ;	
	}
	else
	{
		double temp= _MAX(min_hdway,random->nrandom(mu,sd));
 		return time+temp;
	}
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


double ODServer::next(const double time)
{
	if (theParameters->od_servers_deterministic)
		return (time+mu);
	else
		return time+(random->rrandom(mu));
	/****
#ifndef _DETERMINISTIC_OD_SERVERS
 return time+(random->rrandom(mu));
#else
 return time+mu;
#endif
 ***/
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

double OServer::next(const double time)
{
	double bound=random->urandom();
	double temp;
	if (bound < alpha)
	{
		temp=_MAX(min_hdway,random->nrandom(cf_hdway,sd_cf_hdway));  //
		//cout << "bound : " << temp << endl;
	}
	else	
	{
		temp=max_cf_hdway+(random->rrandom(mu_unbound-max_cf_hdway)); // shifted neg exp.
		//cout << "unbound : " << temp << endl;
	}
	temp=temp/lanes; // make it per lane.
	return time+temp;
}

double LogNormalDelayServer::next (const double time)
{
	double result = delay + random->lnrandom(mu, sd);
	//double delay_std1 = theParameters->sd_server_scale * sqrt(delay_std) * sqrt (mu + delay); // This is specifically for the Tel-Aviv case study use - delay_std = total driving time for this busline
	return time + result ;
}


double LogLogisticDelayServer::next (const double time)
{
	return time + random->loglogisticrandom(mu,sd);
}

ChangeRateAction::ChangeRateAction (Eventlist* eventlist_, double time_, Server* server_, double mu_, double sd_)
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
		
bool ChangeRateAction::execute(Eventlist* , double)     //unused  Eventlist* eventlist, double time
{
   if (mu > 0.0)
   		server->set_mu(mu);
   if (sd > 0.0)
   		server->set_sd(sd);
 return true;
}

