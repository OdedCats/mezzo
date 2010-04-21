#include "vissimcom.h"
#ifdef _VISSIMCOM

#include <cstring>
#include <algorithm>
#include <fstream>
#include <assert.h>


using namespace VISSIM_COMSERVERLib;




// help functions



#include <sys/timeb.h>
double timest () // returns timestamp in seconds (time elapsed in sec. since 1 jan 1970, 0:00)

{
	timeb* tb=new timeb;
	ftime(tb);
	double seconds=double(tb->time);
	double ms=tb->millitm;
	delete tb;
	return (seconds*1000 + ms)/1000;
}





IPathPtr AddPath(const long pathid, const vector<long> ids,IPathsPtr & spPaths  )
{
	VARIANT var;
    VariantInit(&var);
    SAFEARRAY *psa;
    SAFEARRAYBOUND rgsabound[1];
    long rgindice[1];
    rgsabound[0].lLbound = 0;
    rgsabound[0].cElements = ids.size();
    psa = SafeArrayCreate(VT_VARIANT, 1, rgsabound);
    for (long i = 0; i < long(ids.size()); i++) {
      rgindice[0] = i;
      var.lVal = ids[i];
      var.vt = VT_I4;
      SafeArrayPutElement(psa, rgindice, &var);
    }

    var.parray = psa;
    var.vt = VT_ARRAY | VT_VARIANT;  
    return spPaths->AddPathAsNodeSequence(pathid, var);
};


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

// VISSIMCOM functions
VISSIMCOM::VISSIMCOM(const string & configfile)

{
	sendcount=0;
	exit=false;
	connected=false;
	booked = false;

	// open the VISSIM COM server & connect
	HRESULT hr;

//	hr = CoInitialize(NULL);

	hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
	if (FAILED(hr))
	{
		eout << "VISSIMCOM Error: problem with CoInitialize() " << endl;
	}

	try 
	{
		// Create the Vissim object (connection to VISSIM COM server)
		eout << "Mezzo: Creating a Vissim instance " << endl;
		spVissim=IVissimPtr(__uuidof(Vissim));

		// Create a Simulation object
		spSim = spVissim->GetSimulation();

		// set the connected param to true, to indicate that the communication works ok
		connected=true;
		nr_veh_entered=0;
		nr_veh_exited=0;
	}
	catch (_com_error &error) 
	{
		eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
	}
}

bool VISSIMCOM::init(const string& configfile, const int runtime)
{
	// TODO read the vissimcom config file
	// FOR NOW: Master file defines vissim input file and Parameters file defines comm_step etc.
		vissim_inputfile = configfile;
		vissim_layout = "";
		vissim_sim_period=runtime;
		vissim_resolution = static_cast <int> ((1/theParameters->mime_comm_step) + 0.5); 
	try
	{
		spVissim->LoadNet (vissim_inputfile.c_str(), 0);
		//spVissim->LoadLayout(vissim_layout.c_str());

		// set the smart pointers to the network, links, paths
		spNet=spVissim->GetNet();
		spLinks = spNet->GetLinks();
		spPaths = spNet->GetPaths();  
		
		// set the sim parameters
		spSim->PutPeriod(vissim_sim_period);
		spSim->PutResolution(vissim_resolution);

		// set the paths in VISSIM
		
		vector <VirtualLink*>::iterator iter0 = virtuallinks->begin();
/*		
		assert (iter0<virtuallinks->end());
		for (iter0;iter0<virtuallinks->end();iter0++)
		{
	
			spPaths = spNet->GetPaths();  // to make sure it contains the latest set of paths
			eout << " Adding path nr " << (*iter0)->pathid << endl;
			IPathPtr p= AddPath((*iter0)->pathid,(*iter0)->get_v_path_ids(), spPaths);
			eout << "Successfully added path " << (*iter0)->pathid << endl;
		}
		long nr = spPaths->GetCount();
		eout << "VISSIM contains " << nr <<" paths "<<  endl;
*/
	}
	catch (_com_error &error) 
	{
		if (error.ErrorInfo())
		{
			eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		}
		else
			eout << "VISSIMCOM Error, No Description available" << endl;
		return false;
	}
	return true;
}
VISSIMCOM::~VISSIMCOM()
{
 // clean up
	eout << "Mezzo: Deleting the Vissim instance. " << nr_veh_entered << " vehicles entered, and " << 
		nr_veh_exited << " vehicles exited." << endl;
	spSim->Stop();
	//CoUninitialize ();
}

int VISSIMCOM::send (double time)
{
// Sends the vehicles from Mezzo Boundary Outs to VISSIM
// Checks which VISSIM entry parking lots are full, and blocks/unblocks the respective BoundaryOut and Virtual Links
	try
	{
		long count=0;
		long buffersize=5;
		
		vector <Signature*>::iterator iter0;
		bool ok=true;
		IParkingLotsPtr p_lots = spNet->GetParkingLots();
		// for each boundary out node
		for (vector <BoundaryOut*>::iterator iter1=(*boundaryouts).begin();iter1<(*boundaryouts).end();iter1++)
		{
			long remaining_space = buffersize;
			if ((*iter1)->get_signatures().size() > 0) // only check space on park lots for boundaries that actually want to send vehicles
			{				
				//Check which parking lots are full, and block/unblock if needed.
				set<long>::iterator pl_iter = (*iter1)->parkinglots.begin();
				for (pl_iter; pl_iter!=(*iter1)->parkinglots.end(); pl_iter++)
				{
					IParkingLotPtr parking = p_lots->GetParkingLotByNumber(*pl_iter); 
					long nr_parked= parking->GetAttValue("NVEHICLES");
					// << "nr vehicles on  entry parking " << *pl_iter << " is " << nr_parked << endl;
					remaining_space = buffersize-nr_parked; // should generalize this using a map with values for all parking lots, but for now there is only one parking lot per boundary out anyway.

					if (nr_parked > buffersize) // buffer to make up for communication delay of Block/unblock messages
					{
						(*iter1)->block(-1,10.0); // block the BoundaryOut, which will notify the respective virtual links.
					}
					else
					{
						if ((*iter1)->blocked()) // if blocked, unblock, with respective density and dissipation speed
						{
							(*iter1)->block(50,20.0);
						}
					}
				}
			}

			// SEND the vehicles for this Boundary Out
			count = 0;
			vector <Signature*> & signatures =(*iter1)->get_signatures() ;
			//eout << " Boundaryout: sending signatures : " << signatures.size() << endl;
			for (iter0=signatures.begin();iter0<signatures.end();)
			{
				// checking space
				if (count <= remaining_space) 
				{
					//then add vehicle
					ok &= add_veh((*iter0)->type,(*iter0)->v_parkinglot,(*iter0)->v_path,(*iter0));
					// and take it out of the signatures list
					iter0=signatures.erase(iter0); // auto advances to next
					count ++;
				}
				else 
					iter0=signatures.end(); // will force an exit from loop
			}
			
		}
	// set the virtual vehicles for all boundaries


		for (vector <BoundaryIn*>::iterator iter3=(*boundaryins).begin();iter3<(*boundaryins).end();iter3++)
		{
			// set the speeds of the exiting vehicles according to downstream conditions
			double speed = 3.6 * ((*iter3)->get_speed(time)); // speed ahead in km/h
			if (speed < 20) // to limit the number of COM calls
			{
				for (set<long>::iterator l_iter = (*iter3)->lastlinks.begin(); l_iter!=(*iter3)->lastlinks.end(); l_iter++)
				{

					ILinkPtr lastlink=spLinks->GetLinkByNumber(*l_iter);
				
					long length=lastlink->GetAttValue("LENGTH");
					long numlanes= lastlink->GetAttValue("NUMLANES");
					IVehiclesPtr vehs =lastlink->GetVehicles();
					long vehcount= vehs->GetCount();
				
					// regulate the speeds 
					if (vehcount >= numlanes) //if there are less vehicles than lanes than I assume the downstream link is open.
					{
						// get the vehicles in an array
						IEnumVARIANTPtr vehenum(vehs->Get_NewEnum());
						unsigned long ulFetched;
						_variant_t* avar= new _variant_t[vehcount];
						vehenum->Reset ();
						vehenum->Next(vehcount, avar, &ulFetched);
						for (int a=0; a < vehcount-1; a++) //  adjust the speed of vehicles
						{
							IVehiclePtr veh=(IVehiclePtr)avar[a].pdispVal;
							long pos = veh->GetAttValue("LINKCOORD");
							
							if ((length-pos) < 15.0)
							{
								
								veh->PutAttValue("SPEED",speed); // this includes setting speed to 0.0 is downstream is blocked.
								veh->PutAttValue("DESIREDSPEED",speed); 
							}
							else
							{
								if ((length-pos) < 50.0)
								{
									// Set the desired speed only
									veh->PutAttValue("DESIREDSPEED",speed); 
								}
							}
						}
					}	
				}
			}
		}
	
		return count; // temporary return till function is filled in
	}
	catch (_com_error &error) 
	{
		if (error.ErrorInfo())
		{
			eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		}
		else
			eout << "VISSIMCOM Error, No Description available" << endl;
		return 0;
	}

}


bool VISSIMCOM::add_veh(unsigned long vehtype, unsigned long parkinglot, long pathid, Signature* sig)
{
	try 
	{
			
		spVehicles=spNet->GetVehicles(); // gets the CURRENT list of vehicles
	/*
		// The original way
		IVehiclePtr spVehicle = spVehicles->AddVehicleInParkingLot(vehtype,parkinglot); // add vehicle in parking lot
		spVehicle->PutAttValue("PATH",(_variant_t) pathid); // assign path
	*/

		
		// test with zones and dynamic paths
		IVehiclePtr spVehicle = spVehicles->AddVehicleInParkingLot(vehtype,parkinglot); // add vehicle in parking lot
	
		spVehicle->PutAttValue("DESTPARKLOT", (_variant_t) sig->v_exitparkinglot);

		//update signature and add to vehicle list

		sig->v_id=spVehicle->GetID();
		vehicles_in_vissim.push_back(sig);

		nr_veh_entered++;
		return true;
	}
	catch (_com_error &error) 
	{
		eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		return false;
	}
}



int VISSIMCOM::receive (double time)
{
// for all vehicles that arrived in parking lots
	bool ok=true;
	try
	{
		spVehicles=spNet->GetVehicles(); // get the current vehicles
		spArrived=spVehicles->GetArrived(); // get the vehicles that have arrived in the last timestep
		spParked=spVehicles->GetParked(); // get the parked vehicles;
		long nr_parked=spParked->GetCount();
//		long nr_arrived=spArrived->GetCount();

/*	
		if (nr_arrived > 0)
		{
			// get the arrived vehicles in an array
			IEnumVARIANTPtr vehenum(spArrived->Get_NewEnum());
			unsigned long ulFetched;
			_variant_t* avar= new _variant_t[nr_arrived];
			vehenum->Reset ();
			vehenum->Next(nr_arrived, avar, &ulFetched);
			for (long a=0; a < nr_arrived; a++) // for all vehicles in the Arrived array avar
			{
				IVehiclePtr spVehArrived=(IVehiclePtr)avar[a].pdispVal;
				long id = spVehArrived->GetID();
*/
		// temporary fix to avoid  missing vehicles with GetArrived.
		// uses GetParked and checks if the current link they're on is a 'finishing link'
		if (nr_parked > 0)
		{
			// get the arrived vehicles in an array
			IEnumVARIANTPtr vehenum(spParked->Get_NewEnum());
			unsigned long ulFetched;
			_variant_t* avar= new _variant_t[nr_parked];
			vehenum->Reset ();
			vehenum->Next(nr_parked, avar, &ulFetched);
			for (long a=0; a < nr_parked; a++) // for all vehicles in the Arrived array avar
			{
				IVehiclePtr spVehArrived=(IVehiclePtr)avar[a].pdispVal;
// check current link
				long curlink=spVehArrived->GetAttValue("LINK");
				bool at_last_link=false;
				for (vector <VirtualLink*>::iterator iter = virtuallinks->begin(); iter != virtuallinks->end(); iter++)
				{
					
					if ( (*iter)->lastlink == curlink)
					{
						at_last_link=true;
						break;
					}
				}
				if (at_last_link)
				{
					long id = spVehArrived->GetID();
					// find vehicle in signatures list.
					vector <Signature*>::iterator sig_iter = (find_if(vehicles_in_vissim.begin(), vehicles_in_vissim.end(), compare<Signature>(id)));
					Signature* sig = *sig_iter;
					assert (sig != NULL);
					
					//update timestamp in signature
					sig->timestamp = time;

					// find the boundaryIn node
					vector <BoundaryIn*>::iterator iptr=find_if(boundaryins->begin(),boundaryins->end(),compare <BoundaryIn> (sig->tmpdestination));
					if (iptr < (*boundaryins).end() )
					{
						
						ok &=(*iptr)->newvehicle(sig);
						if (!ok)
							eout << " could not create vehicle " << id << " from signature" << endl;
						// delete signature	
						vehicles_in_vissim.erase(sig_iter);
						delete sig;
						// delete vehicle in VISSIM
						spVehicles->RemoveVehicle(id);
						//eout << "Vehicle arrived " << id << endl;
						nr_veh_exited++;
					}
					else
						eout << "cannot find boundaryin node " << sig->tmpdestination << endl;	
				}
			}
		}
	
	}
	catch (_com_error &error) 
	{
	//	eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		if (error.ErrorInfo())
		{
			eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		}
		else
			eout << "VISSIMCOM Error, No Description available" << endl;
	}



// then check the block/unblock and density values 


	return ok; // temp return value
	
}

bool VISSIMCOM::execute(Eventlist* eventlist, double time)
{
 	if (booked)
	{
		bool ok=false;
		// SIMPLE way to keep realtime factor constant. Can only slow down though, not speed up.
		if (theParameters->sim_speed_factor > 0) 
		{
			double current_timestamp = timest();
			double expected_timestamp = (last_timestamp + ((theParameters->vissim_step)/ theParameters->sim_speed_factor));
			//eout << "synching. current time " << current_timestamp << " waiting till " << expected_timestamp << endl;
			while ( expected_timestamp > current_timestamp)
			{
				current_timestamp = timest();
				//eout << "synching. current time " << current_timestamp << " waiting  for " << expected_timestamp-current_timestamp << " secs "
				//<< endl;
			}
			last_timestamp = current_timestamp;
		
		}
		try
		{
			spSim->RunSingleStep(); // advance the VISSIM simulation one step
		}
		catch (_com_error &error) 
		{
			if (error.ErrorInfo())
			{
				eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
			}
			else
				eout << "VISSIMCOM Error, No Description available" << endl;
		}
		
		// do this always!
		int rec=receive(time);	// receive the info from VISSIM
		// do this only at certain intervals.
		if (time > next_mime_comm_step)
		{
			int sigcount=send(time); // send the info to VISSIM
			next_mime_comm_step+=theParameters->mime_comm_step;
		}
	}
	else 
	{
		booked=true;
		last_timestamp = timest(); // start the timestamp series.
		next_mime_comm_step = 0.0;
	}
	double new_time=time+theParameters->vissim_step; // re-book myself every VISSIM COM STEP in the event list
	if (!exit)
		eventlist->add_event(new_time,this);	// if the exit flag is turned on, no more messages will come...
	//return ok;
	
	return true; // temp return value
}


#endif //_VISSIMCOM
