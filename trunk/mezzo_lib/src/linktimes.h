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

LinkCostInfo contains the Generalized link costs for all links in the network. It is used by the shortest path algorithm.


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


	void generate_disturbances (); //!< generates the random disturbances vector, where each is element is a disturbance of the associated link travel time
								//!< disturbances are zero mean: uniform (-0.5,0.5) * 2*theParameters->linktime_disturbance * times_ [i];  linktime_disturbance is the percentace of disturbance, relative to link times.

	void zero_disturbances(); //!< sets disturbances to zero

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

 class LinkCostInfo
	 /* contains the time dependent generalized cost for all links 
	 * For each user class, the attributes need to be set
	 *
	 *
	 */
 {
 	public:
		virtual ~LinkCostInfo ();
 	const double cost (const int i, const double time=0.0); //!< returns the generalized link cost for link i
	const double graph_cost (const int i, const double time=0.0);//!< returns the generalized link cost for graph_link i (mapped to mezzo links)
	void set_graphlink_to_link (const map <int,int> & map) {graphlink_to_link = map;}//!< sets the mapping for graph_links to links (graph links have to be numbered continuously from 0, links can have any numbering)
	void generate_disturbances (); //!< generates the distrurbances for link travel times, which reflect small perception errors
	void zero_disturbances (); //!< sets the disturbances to 0
	const double mean (); //!< returns the mean of the travel times
	const double sum();//!< returns the sum of the travel times.
	map <int, LinkTime*>& times () {return times_;} // returns the travel times map
	map <int, double>& distances () {return distances_;} //!< returns the link distances (m)
	map <int, double>& tolls() {return tolls_;} //!< returns the link tolls (SEK)

	void set_link_distance(const int i, const double distance) {distances_ [i]=distance;}
	const double get_link_distance(const int i)  {if (distances_.count(i)) return (distances_[i]); else return 0.0;}

	void set_link_toll(const int i, const double toll) {tolls_[i]=toll;} //!< for now the toll is fixed, will make it time dependent (such as LinkTime) when we actually start using it
	const double get_link_toll(const int i)  {if(tolls_.count(i)) return (tolls_ [i]); else return 0.0;}
 private:
 	map <int, LinkTime*> times_; //!< contains the link times (s)
	map <int, double> distances_; //!< contains the link distances (m)
	map <int, double> tolls_; //!< contains the link tolls (SEK)

	map <int, int>graphlink_to_link;

	double value_of_time; // in SEK/s
	double value_of_distance; // in SEK/m
 };
	 	

#endif
