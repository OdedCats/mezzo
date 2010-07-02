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

/* Communication class that talks to the VISSIM API. On initialisation it starts VISSIM.
	It is derived from action, since it will book itself in the event list each update step (say 0.1 sec), and when called
	it sends all the vehicles from the boundary outnodes and receives vehicles and puts them into the boundaryin nodes.
	It works slightly different than the PVM class, which communicates with Mitsim, in that it initialises and controls the communication
	throughout the simulation run, whereas the PVM class follows the directions from Mitsim (time-step, synchro, etc.)
*/

#ifndef VISSIMCOM_
#define VISSIMCOM_

#ifdef _VISSIMCOM


#include "eventlist.h"
#include "node.h"
#include "parameters.h"
#include "signature.h"


#define _WIN32_DCOM
#include <objbase.h>

#include <iostream>
#include <vector>


using namespace std;

// import of all VISSIM COM server interfaces

#import "C:\Program Files\PTV_Vision\VISSIM520\Exe\vissim.exe" 

//using namespace VISSIM_COMSERVERLib;

// Not using the VISSIM-COM namespace, due to name clashes 'Link' and 'Node'


//#include "gettime.h"

// sendtag== receivetag== nodeID

class BoundaryOut;
class BoundaryIn;

class VISSIMCOM : public Action
{
	public:
		VISSIMCOM(const string & configfile) ;
		~VISSIMCOM() ;
		bool init (const string & configfile, const int runtime);
		const bool execute(Eventlist* eventlist, const double time);   // to do what has to be done and book itself regularly in the eventlist
		void register_boundaryouts( vector <BoundaryOut*> * boundaryouts_) {boundaryouts=boundaryouts_;}
		void register_boundaryins( vector <BoundaryIn*> * boundaryins_) {boundaryins=boundaryins_;}
		void register_virtuallinks( vector <VirtualLink*> * virtuallinks_) {virtuallinks=virtuallinks_;}
		int send (double time);
		bool add_veh(unsigned long vehtype, unsigned long parkinglot, long pathid, Signature* sig);
		int receive (double time);
		bool is_connected() {return connected;}
	protected:
		vector <BoundaryOut*>  * boundaryouts;
		vector <BoundaryIn*> * boundaryins;
		vector <VirtualLink*> * virtuallinks;
		vector <Signature*> signatures;
		int sendcount;
		bool exit;
		bool connected;
		string vissim_inputfile; // vissim network file
		string vissim_layout; // vissim layout file
		unsigned long vissim_sim_period; // simulation period == runtime
		unsigned long vissim_resolution; // steps per second. default = 10
		bool booked; // true if already in eventlist
		double next_mime_comm_step; // next time the MIME communication will be performed
		double last_timestamp; // to keep realtime ratio.
		vector <Signature*> vehicles_in_vissim; // contains all the vehicles in Vissim
		long nr_veh_entered, nr_veh_exited; // counters of nr vehicles entered/exited in Vissim
		// VISSIMCOM specific
		VISSIM_COMSERVERLib::IVissimPtr spVissim;
		VISSIM_COMSERVERLib::ISimulationPtr spSim;
		VISSIM_COMSERVERLib::INetPtr spNet;
		VISSIM_COMSERVERLib::ILinksPtr spLinks;
		VISSIM_COMSERVERLib::IPathsPtr spPaths;
		VISSIM_COMSERVERLib::IVehiclesPtr spVehicles; //  DYNAMIC, NEEDS TO BE RE_ASSIGNED
		VISSIM_COMSERVERLib::IVehiclesPtr spArrived; // DYNAMIC, NEEDS TO BE RE_ASSIGNED
		VISSIM_COMSERVERLib::IVehiclesPtr spParked; // DYNAMIC, NEEDS TO BE RE_ASSIGNED
};

#endif //_VISSIMCOM

#endif //VISSIMCOM_
