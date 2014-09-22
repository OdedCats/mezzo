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
#include "day2day.h"
//#include <windows.h>

int main ( int argc, char **argv)
{
  //SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
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
  Random::create(1);
  if (seed != 0)
  {
		theRandomizers[0]->seed(seed);
  }
   // NEW started using threads for future parallel runs. 
   // However, global vars need to be moved local to run more than one thread at a time to avoid data conflicts.
  NetworkThread* net1 = new NetworkThread(argv[1],1,seed);
  net1->init(); // reads the input files
  bool steady_state=false;
  int max_days = 5;

  enum m {wt, ivt};
  Day2day* d2d = new Day2day(1);
  float crit[2];
  crit[wt] = 1000.0f;
  crit[ivt] = 1000.0f;
  float theta = theParameters->break_criterium;

  if (replications <=1)
  {
	for (int day = 1; (crit[wt] >= theta || crit[ivt] >= theta) && day <= 20; day++) //day2day
	{
		cout << "Day: " << day << endl;

		crit[wt] = 0.0f;
		crit[ivt] = 0.0f;

		d2d->update_day(day);
	  
		remove("o_passenger_waiting_experience.dat");
		remove("o_passenger_onboard_experience.dat");
		remove("o_passenger_alighting.dat");
		remove("o_passenger_boarding.dat");
		remove("o_passenger_connection.dat");
		remove("o_selected_paths.dat");

		net1->start(QThread::HighestPriority);
		net1->wait();
		net1->saveresults();
		net1->reset();

		if (theParameters->pass_day_to_day_indicator == true)
		{
			d2d->process_wt_replication();
			crit[wt] = d2d->process_wt_output();
		}

		if (theParameters->in_vehicle_d2d_indicator == true)
		{
			d2d->process_ivt_replication();
			crit[ivt] = d2d->process_ivt_output();
		}

		cout << "Convergence: " << crit[wt] << " " << crit[ivt] << endl;
	}
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


