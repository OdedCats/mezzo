#ifndef PASSENGER_ROUTE
#define PASSENGER_ROUTE


class Pass_route
{
	Pass_route ();
	~Pass_route ();
public:
	vector <Busline*> route_lines;
	void add_lines (Busline* line) {route_lines.push_back(line);}

protected:
}

#endif