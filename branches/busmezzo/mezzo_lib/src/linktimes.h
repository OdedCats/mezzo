/***********************************************************************************************************
LinkTime contains the time-variant travel time for a link.

LinkTimeInfo contains the LinkTime for all links in the network. It is used by the shortest path algorithm.

***********************************************************************************************************/
 #ifndef LINKTIMES
 #define LINKTIMES

 #include <vector>
#include <map>

 using namespace std;

 class LinkTime
 {
  public:
  LinkTime(): id(0), nrperiods(0), periodlength (0.0) {totaltime =0.0;}
  LinkTime(int id_, int nrperiods_, double periodlength_) : id(id_), nrperiods(nrperiods_), periodlength(periodlength_)   
				{	totaltime = nrperiods*periodlength;
					times.resize(nrperiods);
				}
  int id;   // link id
  const int get_id() {return id;}
  int nrperiods;   // number of time periods
  double periodlength; //periodlength
  vector <double> times;  // the vector of times

  const double cost(const double time) ;            // this function returns the correct travel time (cost) based on the entry time
  double totaltime;

 } ;

 class LinkTimeInfo
 {
 	public:
 	const double cost (const int i, const double time=0.0);

 	//vector <LinkTime*> times;
	map <int, LinkTime*> times;
 };
	 	

#endif
