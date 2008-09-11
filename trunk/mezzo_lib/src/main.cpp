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

#include "network.h"


int main ( int argc, char **argv)
{
  if (argc < 2)
  {
	cout << "at least one argument needed (*.mezzo filename) " << endl;
	return -1;
  }
  Network theNetwork;
  if (argc > 2)
  {
      theNetwork.seed(atoi (argv[2]));
	  //Network theNetwork = Network(atoi(argv[2]));
	  theNetwork.readmaster(argv[1]);
	  double runtime=theNetwork.executemaster();
	  theNetwork.step(runtime);
	  theNetwork.writeall();
  }
  else
  {
	  
	  theNetwork.readmaster(argv[1]);
	  double runtime=theNetwork.executemaster();
	  theNetwork.step(runtime);
	  theNetwork.writeall();
  }
  return 0;
}


