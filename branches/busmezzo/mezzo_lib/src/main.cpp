
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


