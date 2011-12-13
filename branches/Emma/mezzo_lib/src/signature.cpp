#include "signature.h"
// Signature files



 #ifdef _PVM 

Signature::Signature (int id_, int speed_ , double timestamp_, double entrytime_, double starttime_, int meters_, int origin_, int destination_, int type_, double length_, int path_, int tmppath_,int tmpor_, int tmpdest_) :
id(id_), speed(speed_), timestamp(timestamp_), entrytime(entrytime_), starttime(starttime_), meters(meters_), origin(origin_), destination (destination_),
type(type_), length(length_), path (path_), tmppath(tmppath_), tmporigin(tmpor_), tmpdestination (tmpdest_)
{}

Signature::Signature(PVM_Service * com)
{
Signature();
 read (com);
}

bool Signature::read (PVM_Service* com)
{
 	(*com) >>  id;
 	(*com) >> speed;
 	(*com) >> timestamp;
 	(*com) >> entrytime;
 	(*com) >> starttime;
 	(*com) >> meters;
 	(*com) >> origin;
 	(*com) >> destination;
 	(*com) >> type;
 	(*com) >> length;
 	(*com) >> path;
  (*com) >> tmppath;
 	(*com) >> tmporigin;
 	(*com) >> tmpdestination;
 	return true;
}

bool Signature:: send(PVM_Service* com)
{
 	(*com) <<  id;
 	(*com) << speed;
 	(*com) << timestamp;
 	(*com) << entrytime;
 	(*com) << starttime;
 	(*com) << meters;
 	(*com) << origin;
 	(*com) << destination;
 	(*com) << type;
 	(*com) << length;
 	(*com) << path;
  (*com) << tmppath;
 	(*com) << tmporigin;
 	(*com) << tmpdestination;
 	return true;
}

void Signature::display(ostream& out)
{
 (out) <<  id << '\t';
 	(out) << speed << '\t';
 	(out) << timestamp << '\t';
 	(out) << entrytime << '\t';
 	(out) << starttime << '\t';
 	(out) << meters << '\t';
 	(out) << origin << '\t';
 	(out) << destination << '\t' ;
 	(out) << type << '\t';
 	(out) << length << '\t';
 	(out) << path << '\t';
  (out) << tmppath << '\t';
 	(out) << tmporigin << '\t';
 	(out) << tmpdestination << '\t';
  (out) << endl;
}
 #endif // _PVM

#ifdef _VISSIMCOM //VISSIMCOM IMPLEMENTATION

Signature::Signature (int id_, int speed_ , double timestamp_, double entrytime_, double starttime_, int meters_, int origin_, int destination_, int type_, double length_, int path_, int tmppath_,int tmpor_, int tmpdest_) :
id(id_), speed(speed_), timestamp(timestamp_), entrytime(entrytime_), starttime(starttime_), meters(meters_), origin(origin_), destination (destination_),
type(type_), length(length_), path (path_), tmppath(tmppath_), tmporigin(tmpor_), tmpdestination (tmpdest_)
{}



void Signature::display(ostream& out)
{
 (out) <<  id << '\t';
 	(out) << speed << '\t';
 	(out) << timestamp << '\t';
 	(out) << entrytime << '\t';
 	(out) << starttime << '\t';
 	(out) << meters << '\t';
 	(out) << origin << '\t';
 	(out) << destination << '\t' ;
 	(out) << type << '\t';
 	(out) << length << '\t';
 	(out) << path << '\t';
  (out) << tmppath << '\t';
 	(out) << tmporigin << '\t';
 	(out) << tmpdestination << '\t';
  (out) << endl;
}

#endif // _VISSIMCOM