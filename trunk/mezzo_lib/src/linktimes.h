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

/***********************************************************************************************************
LinkTime contains the time-variant travel time for a link.

LinkTimeInfo contains the LinkTime for all links in the network. It is used by the shortest path algorithm.

***********************************************************************************************************/
 #ifndef LINKTIMES
 #define LINKTIMES

 #include <vector>
#include <map>
#include "Random.h"
#include "parameters.h"

 using namespace std;

 class LinkTime
 {
  public:
  // CONSTRUCTORS; DESTRUCTORS AND INIT
	LinkTime(): id(0), nrperiods(0), periodlength (0.0) {totaltime =0.0; init_random();}
	LinkTime(const LinkTime& lt) : id(lt.id) , nrperiods(lt.nrperiods), periodlength(lt.periodlength), times_(lt.get_times()), totaltime(lt.totaltime) {init_random();}
	LinkTime(const int id_, const int nrperiods_, const double periodlength_) : id(id_), nrperiods(nrperiods_), periodlength(periodlength_)   
				{	totaltime = nrperiods*periodlength;
					init_random();
				}
	 void reset () {	times_.clear();	}
 
	 void init_random()	 {
										rand = new Random();
										if (randseed != 0)
											rand->seed(randseed);
										else
											rand->randomize();
									 }
	 ~LinkTime() {delete rand;}


 // generates the random disturbances vector, where each is element is a disturbance of the associated link travel time
// disturbances are zero mean: uniform (-0.5,0.5) * 2*theParameters->linktime_disturbance * times_ [i];  linktime_disturbance is the percentace of disturbance, relative to link times.
 void generate_disturbances (); 

  // GETS and SETS
  const int get_id() const {return id;}
  void set_id(const int id_) {id=id_;};
  const int get_nrperiods() const {return nrperiods;}
  void set_nr_periods(const int nrperiods_) {nrperiods= nrperiods_;}
  const double get_periodlength() const {return periodlength;}
  void set_periodlength(const double periodlength_) {periodlength=periodlength_;}
  const double get_totaltime () const {return totaltime;}
  void set_totaltime(const double totaltime_) {totaltime=totaltime_;}
  
  const map <int,double> & get_times() const {return times_;}
  map <int,double> & times()  {return times_;} // INSECURE !

  void set_times (const  map <int,double> & intimes_) {times_=intimes_;}
  const map <int,double> & get_disturbances() const {return disturbances;}
  void set_disturbances(const map<int,double> & disturbances_) {disturbances=disturbances_;}


  // MEAN & SUM
  const double mean ();
  const double sum ();
  
  //COST
  const double cost(const double time) ;            // this function returns the correct travel time (cost) based on the entry time
 
 private:
	 int id;   // link id
	 int nrperiods;   // number of time periods
	 double periodlength; //periodlength
	 double totaltime;
	 map <int,double> times_; // maps period to link time
	 map <int,double> disturbances; // is for use with random disturbances of link times.
	 Random* rand;

 } ;

 class LinkTimeInfo
 {
 	public:
 	const double cost (const int i, const double time=0.0);
	const double graph_cost (const int i, const double time=0.0);
	void set_graphlink_to_link (const map <int,int> & map) {graphlink_to_link = map;}
	void generate_disturbances ();
	const double mean ();
	const double sum();

 	//vector <LinkTime*> times;
	map <int, LinkTime*> times;
	map <int, int>graphlink_to_link;
 };
	 	

#endif
