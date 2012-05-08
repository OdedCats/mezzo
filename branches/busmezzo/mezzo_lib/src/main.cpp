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
#include "parameters.h"

class Day;

int main ( int argc, char **argv)
{
  long int seed = 0;
  unsigned int replications = 1;
  if (argc < 2)
  {
	cout << "at least one argument needed (*.mezzo filename) " << endl;
	cout << " Usage: mezzo_s   <filename.mezzo>    <nr_replications>   <random_seed>" << endl; 
	return -1;
  }
  if (argc > 2)
	replications=atoi(argv[2]);
   if (argc > 3)
	  seed=atoi(argv[3]);
   // NEW started using threads for future parallel runs. 
   // However, global vars need to be moved local to run more than one thread at a time to avoid data conflicts.
  NetworkThread* net1 = new NetworkThread(argv[1],1,seed);
  net1->init(); // reads the input files
  bool steady_state=false;
  int max_days = 5;
  if (replications <=1)
  {
	  //while (steady_state==false)
	  //{
			//Day* today = new Day();
			//theParameters->calendar.push_back(today);
			net1->start(QThread::HighestPriority);
			net1->wait();
			net1->saveresults();
			/*
			did++;
			if(theParameters->calendar.size() < max_days)  //steady-state check
			{
				net1->end_of_day();
			}
			else
			{ 
				steady_state=true;
				break;
			}
			*/
		//	steady_state = true;
		//}
  }
  else
  {
	  for (unsigned int rep=1; rep <=replications; rep++)
	  {
		  net1->start(QThread::HighestPriority);
		  net1->wait();
		  net1->saveresults(rep);
		  net1->reset();
	  }
  }
  delete net1;
  

  //OLD:
  /*
  Network* theNetwork= new Network();
  if (argc > 2)
  {
      theNetwork->seed(atoi (argv[2]));
	  //Network theNetwork = Network(atoi(argv[2]));
	  theNetwork->readmaster(argv[1]);
	  double runtime=theNetwork->executemaster();
	  theNetwork->step(runtime);
	  theNetwork->writeall();
  }
  else
  {
	  
	  theNetwork->readmaster(argv[1]);
	  double runtime=theNetwork->executemaster();
	  theNetwork->step(runtime);
	  theNetwork->writeall();
  }
  */
  return 0;
}


