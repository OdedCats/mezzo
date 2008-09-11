/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SERVER_HH
#define SERVER_HH

#include "eventlist.h"
#include "Random.h"
#include "parameters.h"

// #define _DEBUG_SERVERS // uncomment to output debug info for servers

/*******************************************************************************
THis class defines the random time-headway servers that are used throughout the 
program. 
*******************************************************************************/



// At this moment the standard time headway server creates N(mu,sd) distributed headways which are
// to be bigger than min_hdway. In the case the random draw generates a number smaller than min_hdway,
// the min_hdway value is returned.  I wonder how this affects the randomness and the spread
// of the generated numbers. Maybe a redraw would be better.
//
// The random number generator is initialised and randomised at the creation of the object

class Server  // standard n(mu,sd2) server  type: 1
{
  public:
  Server(const int id_, const int type_, const double mu_, const double sd_, const double delay_);
  virtual ~Server();
  const int get_id() const;
  virtual  double next(const double time);
  const double get_mu() const;
	void set_mu(const double mu_);
	const double get_sd() const;
	void set_sd(const double sd_);
	const double get_delay () {return delay;}
  protected:
  Random* random;
  
  int id;
  int type; // dummy, later more subclasses of server
  double mu;
  double sd;
  double delay;
};

class ConstServer : public Server // constant server that copies input to output  type: 0
/*
   after every successfull action directly a new action at the same time is performed with
	0 delay, untill all vehicles in the incoming link that have arrived at the end are processed and
	the next time is taken from the next vehicle to arrive at the stopline.
*/
{
	public:
		ConstServer(const int id_, const int type_, const double mu_, const double sd_, const double delay_):Server(id_,type_,mu_,sd_,delay_) {}		
		double next(const double time) {return time;}
} ;

class ODServer : public Server
/* specially designed for generating OD demand
	neg exponential.
*/
{
	public:
		ODServer(const int id_, const int type_, const double mu_, const double sd_);
		double next(const double time);
		void set_rate(double mu_, double sd_) {mu=mu_; sd=sd_;}
	private:
		
};


class DetServer : public Server
/* Delivers deterministic service time
  */
{
public:
	DetServer (const int id_, const int type_, const double mu_, const double sd_, const double delay_) :
		Server (id_,type_,mu_,sd_,delay_) {}
	double next (const double time) {return time+mu+delay;}
};

class OServer : public Server
/*   NOTE not in use anymore, traffic is generated directly onto inputqueue by OD pairs.

   Especially designed for input flows. Designed to be a combination of a N(mu,sd) and EXP(beta) server,
	: output=alpha(N(mu_bound,sd_bound)) + (1-alpha)Exp(mu_unbound)
	- alpha: proportion of carfollowing headways
	- 1-alpha: proportion of non carfollowing headways
	- mu= alpha*mu_bound+(1-alpha)mu_unbound
	- One O server per lane!
*/

{
	public:
		OServer(const int id_, const int type_, const double mu_, const double sd_);
		double next(const double time);
		void set_lanes(double l) {lanes=l;}
		double get_lanes() {return lanes;}
	protected:
		double alpha;  // proportion of cars in carfollowing
		double mu_unbound; // mu for the unbound traffic, calculated from (mu-alpha*cf_hdway)/(1-alpha)
		double lanes;

};

class ChangeRateAction: public Action
// This class changes the rate of a server, once it's 'time' has come in the eventlist.

{
	public:
		ChangeRateAction (Eventlist* eventlist, double time, Server* server_, double mu_, double sd_);
		bool execute(Eventlist* eventlist, double time);
	private:
		Server* server;
		double mu, sd;
};


#endif

