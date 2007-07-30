#ifndef MEZZOASSIST_H
#define MEZZOASSIST_H

#include "../mezzo_lib/src/od.h" 

namespace assist{
	
	struct compareod
	{
		compareod(odval val_):val(val_) {}
		bool operator () (ODpair* odpair){return (odpair->odids()==val);}
	    odval val;
	};
};

#endif

