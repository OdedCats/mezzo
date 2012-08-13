#include "pvm.h"
#ifdef _PVM

#include <cstring>
#include <algorithm>
#include <fstream>

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

PVM::PVM(char* name, int sendtag, int receivetag):PVM_Service(name,sendtag,receivetag,1024)

{
	sendcount=0;
	exit=false;
	connected=false;
	tid_=parent();
	if (tid_>0)
	{
	  connect (tid_);
	  connected=true;
     name_="MEZZO";
      //FILE * fpo = fopen("/home/wilco/test/MezzoTrace.out","w")      ;
      //this->logfile(fpo);
	}
	else
		cout << " Mezzo cannot find PVM parent " << endl;
}

int PVM::send (double time)
{
	initsend (); // prepare for sending
	int sigcount=0; // init nr of signatures sent
	// Send the vehicle signatures
	(*this) << sendcount++;         // put the index of this message
	(*this) << boundaryouts->size();	// put the number of nodes
	for (vector <BoundaryOut*>::iterator iter=(*boundaryouts).begin();iter<(*boundaryouts).end();iter++)
	{
	 	sigcount+=(*iter)->send_message(this); // let each boundary out node send its signatures
	}
	// send the speeds and  distances of the BoundaryIn tail-vehicles
	(*this) << boundaryins->size();
	for (vector <BoundaryIn*>::iterator iter=(*boundaryins).begin();iter<(*boundaryins).end();iter++)
	{
	     sigcount+=(*iter)->send_message(this,time);
	}
	
	flush (SEND_IMMEDIATELY);
	return sigcount;
}


int PVM::receive_now ()
{
	 int rcv=this->receive(WAIT_MSG);
	//int rcv=this->receive(0.1);
	int rcvcount=0;
//	int nrnodes=0;
	int nodeid, nrsigs, code;
  double speed;
	bool ok=false;
	if (rcv)
	{
		(*this) >> rcvcount;
		// Check the rcvcount with previous messages.
		(*this) >> nrsigs;
		switch (rcvcount)
		{
			case -1 :
					exit=true;
					// and I should send a similar thing back....
					break;
			case -2 :
				for (int i=0; i<nrsigs;i++) // for all signals of boundary blockage / unblockage
				{
					(*this) >> nodeid; // get the nodeid
					
					// do what is needed for blocking virtual links from Boundary Out node
					vector <BoundaryOut*>::iterator optr=find_if(boundaryouts->begin(),boundaryouts->end(),compare <BoundaryOut> (nodeid));
				 	if (optr < (*boundaryouts).end() )
				 	{
						(*this) >> code;
                     (*this) >> speed;
						(*optr)->block(code,speed);   // if code > 0 then code= upstream density
						cout << "Mezzo::(un)blocking node " << nodeid <<" code " << code  << " speed " << speed << endl;
                      cout.flush();
					 }
					else
						cout << "cannot find boundaryout node " << nodeid<< endl;	
				 } // and do another receive, since this one is "extra"
				 this->receive_now();
				 //eventlist->add_event(time,this);
				return rcv; // and exit otherwise the service will book itself once too many.
				break;
		
			default	:
		
				for (int i=0; i<nrsigs;i++)
				{
				 	Signature* sptr=new Signature(this);
				 	vector <BoundaryIn*>::iterator iptr=find_if(boundaryins->begin(),boundaryins->end(),compare <BoundaryIn> (sptr->tmpdestination));
				 	if (iptr < (*boundaryins).end() )
				 	{
						ok=(*iptr)->newvehicle(sptr);
						if (!ok)
							cout << " could not create vehicle from signature number " << i << endl;
					}
					else
						cout << "cannot find boundaryin node " << sptr->tmpdestination << endl;	
				 	delete sptr;	
				}
   		}
	}
	else
		cout << " No message received! " << endl;
	return rcv;		
}

bool PVM::execute(Eventlist* eventlist, double time)
{
	//cout << " Mezzo receive at time " << time << endl;
// SEND
	bool ok=false;
	int sigcount=send(time);
	int rec=receive_now();	
	double new_time=time+theParameters->mime_comm_step;
	if (!exit)
		eventlist->add_event(new_time,this);	// if the exit flag is turned on, no more messages will come...
	return ok;
}





bool PVM::spawnfriend (char* name)
{
	initsend();
	name_=name;
	tid_=spawn(NULL);
	
	return connect(tid_);
}	

#endif //_NO_PVM
