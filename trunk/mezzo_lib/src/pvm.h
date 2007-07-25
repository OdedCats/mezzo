/* Communication class that talks to its equivalent in MITSIM. On start it connects to its parent, which is mitsim.
	It is derived from action, since it will book itself in the event list each update step (say 0.1 sec), and when called
	it sends all the vehicles from the boundary outnodes and receives vehicles and puts them into the boundaryin nodes.

*/

#ifndef PVM_
#define PVM_

#ifdef _PVM
#define _MIME
//#include <iostream>
#include "/home/wilco/view/simlab/IO/PVM_Service.h"
#include "eventlist.h"
#include "node.h"
#include "parameters.h"
#include "signature.h"

//#include "gettime.h"

// sendtag== receivetag== nodeID

class BoundaryOut;
class BoundaryIn;

class PVM : public PVM_Service, Action
{
	public:
		PVM(char* name, int sendtag, int receivetag);
		~PVM() {pvm_exit();} // be nice and announce our exit...
		bool execute(Eventlist* eventlist, double time);   // to do what has to be done and book itself regularly in the eventlist
		void register_boundaryouts( vector <BoundaryOut*> * boundaryouts_) {boundaryouts=boundaryouts_;}
		void register_boundaryins( vector <BoundaryIn*> * boundaryins_) {boundaryins=boundaryins_;}
		bool spawnfriend (char* name); // alternative to makefriends. Spawns the receiving process.
		int send (double time);
		int receive_now ();
		bool is_connected() {return connected;}
	private:
		vector <BoundaryOut*>  * boundaryouts;
		vector <BoundaryIn*> * boundaryins;
		vector <Signature*> signatures;
		int sendcount;
		bool exit;
		bool connected;
};

#endif //_PVM

#endif //PVM_
