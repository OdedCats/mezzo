#ifndef PASSENGER_ROUTE
#define PASSENGER_ROUTE
#include "busline.h"
class Busline;

class Pass_route
{
	public:
	Pass_route ();
	~Pass_route ();

	vector <Busline*> route_lines;
	void add_lines (Busline* line) {route_lines.push_back(line);}

protected:
};

#endif