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
  long int seed = 0;
  unsigned int replications = 1;
  if (argc < 2)
  {
	cout << "at least one argument needed (*.mezzo filename) " << endl;
	cout << " Usage: mezzo_s   <filename.mezzo>   <random_seed>" << endl; 
	return -1;
  }
  if (argc > 2)
	  seed=atoi(argv[2]);
	  /*
	replications=atoi(argv[2]);
   if (argc > 3) 
	  seed=atoi(argv[3]); */
   // NEW started using threads for future parallel runs. 
   // However, global vars (notably theParameters and vid) need to be moved local to run more than one thread at a time to avoid data conflicts.
  NetworkThread* net1 = new NetworkThread(argv[1],1,seed);
  cout << "seed is " << seed << endl;
  net1->init(); // reads the input files
  
  /* Normal case, but now override, replications are disabled for now.
  if (replications <=1)
  {
	  net1->start(QThread::HighestPriority);
	  net1->wait();
	  net1->saveresults();
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
 */

	net1->start(QThread::HighestPriority);
	  net1->wait();
	  net1->saveresults();
	
  
  delete net1;
  

  
  return 0;
}


