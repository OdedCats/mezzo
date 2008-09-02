/* Turning derives from  Action so the vehicle processing can be booked in the
	eventlist. It needs to implement the function execute() that is called at the
	appropriate time by the eventlist.

*/
#ifndef TURNING_HH
#define TURNING_HH
#include "link.h"
#include "server.h"
#include "eventlist.h"
#include "node.h"

//#define _DEBUG_TURNING

class Link;
class Node;

class Turning;



class TurnAction : public Action
{
 	public:
 		TurnAction(Turning* turning_);
 		virtual bool execute(Eventlist* eventlist, double time);
 	private:
 		Turning* turning;
		double new_time;

} ;


class Turning
{
public:
	Turning(int id_, Node* node_, Server* server_, Link* inlink_, Link* outlink_, int size_);
	~Turning();
	void reset ();
	void activate (Eventlist* eventlist,double time); // green light
	void stop() { active = false;} // red light
	bool is_active() {return active;}
	bool is_blocked() { return blocked;}
	void block() {blocked=true;}
	void unblock() {blocked=false;}
	bool process_veh(double time);
	double next_time(double time);
	double link_next_time(double time);
	bool init(Eventlist* eventlist, double time);
	bool execute(Eventlist* eventlist, double time);
	double nexttime;
	const int get_id() {return id;}
	void write(ostream& out);
private:
	//TurnAction* turnaction;    // performs the action of transferring vehicles at the right times
	vector <TurnAction*> turnactions; // multiple turnactions per turning. One for each lane...
	int id;
	Node* node;
	Server* server;
	Link* inlink;
	Link* outlink;
	int size;  // size of the turning pocket (=nr cars that can be treated before other directions might block:also called lookback)
	vector <Turning*> conflicting;
	double delay;
	bool blocked;   // true if blocked
	// help vars
	bool ok; // true if processing went fine
	bool out_full; // true if outlink full
	bool active; // true if turning is active (has green light in signalised crossing), false if red light.

};



#endif
