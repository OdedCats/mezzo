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

#ifndef SIGNATURE
#define SIGNATURE
#include <iostream>
using namespace std;
/* 
This class is only used in MiMe application of Mezzo. In that case either _PVM or _COM needs to be defined
  
	The signature class takes care of sending the signatures of the vehicles
from and to boundary nodes. In the case of _PVM it is involved in both the 
Mezzo and the Mitsim part, which is why it uses the more general PVM_Service class

  In the case of COM (Mezzo+Vissim for the moment) Mezzo creates vehicles directly in Vissim by calling the appropriate COM functions
  so instead of having to read or write, it will simply store the relevant information from the boundary nodes, so that the COM module
  can use them.

*/



#ifdef _PVM 
#include "/home/wilco/view/simlab/IO/PVM_Service.h"


class PVM_Service;

class Signature
{
	public:
	Signature (int id_=0, int speed_=0, double timestamp_=0.0, double entrytime_=0.0, double starttime_=0.0, int meters_=0, int origin_=0, int destination_=0, int type_=1, double length_=6.0, int path_=1, int tmppath_=1, int tmpor_=2, int tmpdest_=3);
	Signature(PVM_Service * com);
	bool read (PVM_Service* com);
	bool send (PVM_Service* com);
  void display (ostream& out);
	//private:
		int id;
		double speed;
		double timestamp;
		double entrytime;
		double starttime;
		int meters;
		int origin;
		int destination;
		int type;
		double length;
		int path;
    int tmppath;
		int tmporigin;
		int tmpdestination;
};
#endif// _PVM 	

#ifdef _VISSIMCOM
//#include something

class COM;

class Signature
{
	public:
	Signature (int id_=0, int speed_=0, double timestamp_=0.0, double entrytime_=0.0, double starttime_=0.0, int meters_=0, int origin_=0, int destination_=0, int type_=1, double length_=6.0, int path_=1, int tmppath_=1, int tmpor_=2, int tmpdest_=3);
	void display (ostream& out);
	int get_id() {return v_id;}
	//private:
		int id;
		int v_id; // id of vehicle in VISSIM
		long v_path; // path id in VISSIM, comes from virtual link
		long v_parkinglot; // parking lot in VISSIM, comes from virtual link
		long v_exitparkinglot; // exit parking lot in VISSIM
		double speed;
		double timestamp;
		double entrytime;
		double starttime;
		int meters;
		int origin;
		int destination;
		int type;
		double length;
		int path; // Path id in Mezzo
    int tmppath; // Virtual LInk ID
		int tmporigin;
		int tmpdestination;
};




#endif //_VISSIMCOM




#endif
