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

 using namespace std;

 class LinkTime
 {
  public:
  LinkTime(): id(0), nrperiods(0), periodlength (0.0) {totaltime =0.0;}
  LinkTime(LinkTime& lt) : id(lt.id) , nrperiods(lt.nrperiods), periodlength(lt.periodlength), times(lt.times), totaltime(lt.totaltime) {}
  LinkTime(int id_, int nrperiods_, double periodlength_) : id(id_), nrperiods(nrperiods_), periodlength(periodlength_)   
				{	totaltime = nrperiods*periodlength;
					//times.resize(nrperiods);
				}
  void reset () {	times.clear();
					//times.resize(nrperiods); 
				}
  int id;   // link id
  const int get_id() {return id;}
  const double mean ();
  const double sum ();
  int nrperiods;   // number of time periods
  double periodlength; //periodlength
  //vector <double> times;  // the vector of times
  map <int,double> times; // maps period to link time
  const double cost(const double time) ;            // this function returns the correct travel time (cost) based on the entry time
  double totaltime;

 } ;

 class LinkTimeInfo
 {
 	public:
 	const double cost (const int i, const double time=0.0);
	const double mean ();
	const double sum();

 	//vector <LinkTime*> times;
	map <int, LinkTime*> times;
 };
	 	

#endif
