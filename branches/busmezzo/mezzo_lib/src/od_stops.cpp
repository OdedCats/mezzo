///! odstops.cpp: implementation of the odstops class.
#include "od_stops.h"

ODstops::ODstops ()
{
}

ODstops::ODstops (Busstop* origin_stop_, Busstop* destination_stop_)
{
	origin_stop = origin_stop_;
	destination_stop = destination_stop_;
}

ODstops::~ODstops()
{}