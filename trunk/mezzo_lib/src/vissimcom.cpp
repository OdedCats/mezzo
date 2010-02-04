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
		// TO DO change this so it is done for all paths.
		vector <VirtualLink*>::iterator iter0 = virtuallinks->begin();
		assert (iter0<virtuallinks->end());
		for (iter0;iter0<virtuallinks->end();iter0++)
		{
	
			spPaths = spNet->GetPaths();  // to make sure it contains the latest set of paths
			spPaths->GetCount();
			eout << "gotten paths" << endl;
			IPathPtr p= AddPath((*iter0)->pathid,(*iter0)->get_v_path_ids(), spPaths);
			eout << "added path " << (*iter0)->pathid << endl;
		}

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
// put the vehicle signatures in
	try
	{
		unsigned short count=0;
		vector <Signature*>::iterator iter0;
		bool ok=true;
		IParkingLotsPtr p_lots = spNet->GetParkingLots();
		for (vector <BoundaryOut*>::iterator iter1=(*boundaryouts).begin();iter1<(*boundaryouts).end();iter1++)
		{
			vector <VirtualLink*>::iterator iter2;
			vector <VirtualLink*> vlinks=(*iter1)->get_virtual_links();
			for (iter2=vlinks.begin(); iter2< vlinks.end(); iter2++)
			{
		
				IParkingLotPtr parking = p_lots->GetParkingLotByNumber((*iter2)->parkinglot); // change this to get it from the virtual link
				long nr_parked= parking->GetAttValue("NVEHICLES");
				//eout << "nr vehicles on parking is " << nr_parked << endl;
				// !!!!!!!!!! CHANGE to do only for particular vlink!!!!!!!!!
				if (nr_parked > 5) // buffer to make up for communication delay of Block/unblock messages
				{
					(*iter1)->block(-1,10.0);
				}
				else
				{
					if ((*iter1)->blocked())
					{
						(*iter1)->block(50,20.0);
					}
				}
			}			
			
		
			// put the vehicles 
			vector <Signature*> & signatures =(*iter1)->get_signatures() ;
			//eout << " Boundaryout: sending signatures : " << signatures.size() << endl;
			for (iter0=signatures.begin();iter0<signatures.end();)
			{
				//check space in VISSIM
				if (count < 3) // change to reflect nr of lanes, simulation step etc etc...
				{
					//then add vehicle
					ok &= add_veh((*iter0)->type,(*iter0)->v_parkinglot,(*iter0)->v_path,(*iter0));
					// and take it out of the signatures list
					iter0=signatures.erase(iter0);
					count ++;
				}
				else 
					iter0=signatures.end(); // will force an exit from code
			}
			
		}
	// set the virtual vehicles for all boundaries


		for (vector <BoundaryIn*>::iterator iter3=(*boundaryins).begin();iter3<(*boundaryins).end();iter3++)
		{
			// set the correct conditions 
			double speed = 3.6 * ((*iter3)->get_speed(time)); // speed ahead in km/h
			
			// for each virtual link belonging to the boundary (for now just one)!!!!
			// !!! CHANGE !!!
			if ( ((*iter3)->get_virtual_links()).size() !=0)
			{
				vector<VirtualLink*>::iterator iter4= ((*iter3)->get_virtual_links()).begin();
				long lastlinkid= (*iter4)->lastlink;
				
				ILinkPtr lastlink=spLinks->GetLinkByNumber(lastlinkid);
		
				long length=lastlink->GetAttValue("LENGTH");
				long numlanes= lastlink->GetAttValue("NUMLANES");
				IVehiclesPtr vehs =lastlink->GetVehicles();
				long vehcount= vehs->GetCount();
				//eout << "At this moment there are " << vehcount << " vehicles on the link" << endl;
				// virtual vehicles
				if (vehcount >= numlanes) //if there are more vehicles than lanes
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
						if ((length-pos) < 20.0)
						{
							// Set both the desired speed and current speed
							veh->PutAttValue("SPEED",speed); // this includes setting speed to 0.0 is downstream is blocked.
							veh->PutAttValue("DESIREDSPEED",speed); 
						}
						else
						{
							if ((length-pos) < 50.0)
							{
								//long newspeed = speed + (oldspeed-speed)/2; // reduce the speed difference by 1/2
								// Set the desired speed only
								veh->PutAttValue("DESIREDSPEED",speed); 
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
		IVehiclePtr spVehicle = spVehicles->AddVehicleInParkingLot(vehtype,parkinglot); // add vehicle in parking lot
		spVehicle->PutAttValue("PATH",(_variant_t) pathid); // assign path
		
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
		long nr_arrived=spArrived->GetCount();
		if (nr_arrived > 0)
		{
			// get the arrived vehicles in an array
			IEnumVARIANTPtr vehenum(spArrived->Get_NewEnum());
			unsigned long ulFetched;
			_variant_t* avar= new _variant_t[nr_arrived];
			vehenum->Reset ();
			vehenum->Next(nr_arrived, avar, &ulFetched);
			for (int a=0; a < nr_arrived-1; a++) //  adjust the speed of vehicles
			{
				IVehiclePtr spVehArrived=(IVehiclePtr)avar[a].pdispVal;
						
			/* OLD WAY
			IVehiclePtr spVehArrived;
			_variant_t m=1L;
			for (long l=0; l < nr_arrived; m=l,l++)
			{
				
				spVehArrived=spArrived->GetItem(m);
			*/	
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
						eout << " could not create vehicle from signature" << endl;
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
/* OLD METHOD
bool VISSIMCOM::execute(Eventlist* eventlist, double time)
{
	if (booked)
	{
		bool ok=false;
		try
		{
			spSim->RunSingleStep(); // advance the VISSIM simulation one step
		}
		catch (_com_error &error) 
		{
			eout << "VISSIMCOM Error: " << (char*)(error.Description()) << endl;
		}
		int sigcount=send(time); // send the info to VISSIM
		int rec=receive(time);	// receive the info from VISSIM
	}
	else 
		booked=true;

	double new_time=time+theParameters->mime_comm_step; // re-book myself in the event list
	if (!exit)
		eventlist->add_event(new_time,this);	// if the exit flag is turned on, no more messages will come...
	//return ok;
	
	return true; // temp return value
}


*/

#endif //_VISSIMCOM
