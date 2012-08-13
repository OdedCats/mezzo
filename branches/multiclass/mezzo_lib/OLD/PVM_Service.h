//!!!!!!!!!!!!!!!!!!!!DONT USE!!!!!!!!!!!USE THE ONE IN SIMLAB INSTEAD!!!!!!!!!!!!!!!
//
//-*-c++-*------------------------------------------------------------
// NAME: A communication wrapper for PVM.
// AUTH: Qi Yang
// FILE: PVM_Service.h
// DATE: Fri Nov  3 22:21:29 1995
//--------------------------------------------------------------------

#ifndef PVM_SERVICE_HEADER
#define PVM_SERVICE_HEADER

#ifndef NO_PVM
using namespace std;
#include <iostream>
#include <stdio.h>
#include <pvm3.h>

#include "MessageTags.h"

const unsigned long int DEFAULT_MSG_SIZE = 4096;

//--------------------------------------------------------------------
// CLASS NAME: PVM_Service
// This class is a wrapper used for various PVM service in the
// simulation.  If an application needs to talk with other modules,
// it should create a PVM_Service and 
//--------------------------------------------------------------------

class PVM_Service
{
protected:

  static int pid_;	// task id of the calling process

  char* module_;	// module name
  char* name_;		// executable name
  int sndtag_;		// message tag for sending
  int rcvtag_;		// message tag for receiving
  

  unsigned long int default_msg_size_;	// num of msg before auto send
  int tid_;			// task id
  unsigned long int msg_size_;		// message count in sending buffer
  
  int state_;		// current state

public:

  // Returns the task id of the calling process

  static int pid();

  // Returns the task id of the parent process

  static int parent() { return pvm_parent(); }

  // Kill current PVM process

  static int exit();

  // Connect or disconnect a logfile

  static void logfile(FILE *fpo);
      
  static int taskHostFlag() { return PvmTaskHost; }      
  static int taskDebugFlag() { return PvmTaskDebug; }

  char* name() { return name_; }
  void name(const char *executable, const char* path = NULL);

  // This constuctor creates an empty PVM service. The service is
  // not ready until connect(id) is sucessfully called.

  PVM_Service(char *module, int stag, int rtag,
			  unsigned long int sz = DEFAULT_MSG_SIZE);

  PVM_Service();

  virtual ~PVM_Service() { }

  inline unsigned int state(unsigned int mask = 0xFFFF) {
	return (state_ & mask);
  }
  inline void unsetState(int s = 0) {
	state_ &= ~s;
  }
  inline void setState(int s) {
	state_ |= s;
  }

  virtual void init(char *module, int stag, int rtag,
					unsigned long int sz = DEFAULT_MSG_SIZE);

  inline int isInitialized() {
	return (module_ != NULL);
  }
  inline short int isConnected() {
	return (tid_ > 0);
  }

  inline int tid() { return tid_; }
  inline int sndtag() { return sndtag_; }
  inline int rcvtag() { return rcvtag_; }
  inline unsigned long int msg_size() { return msg_size_; }

  // Assign this service a processor id.  This function is used to
  // connect current process to another process. For example, call
  // err = connect(pvm_parent()) will connect this service to the
  // parent processor.

  virtual int connect(int id);

  // Start a child process

  virtual int spawn(char **argv, int flag = PvmTaskDefault,
					char *where = "");

  // Clear current sending buffer and prepare for packing new
  // messages.  By default no encoding scheme is used.

  virtual int initsend(int encoding = PvmDataRaw) {
	return pvm_initsend(encoding);
  }

  // Send the packed message if mode is SEND_IMMEDIATELY or
  // message size has exceeded the default_msg_size.

  virtual void flush(int mode);

  // Receive data from the server. If fatal error, the process
  // will quit and this function will never return. Otherwise, it
  // returns id of the active buffer or 0 if mode is
  // DO_NOT_WAIT_MSG and there is no message arrived.

  virtual int receive(int mode = DO_NOT_WAIT_MSG);

  virtual int receive(double wait);

  // Preappend 'msg' to the error message of last PVM call and
  // place them to the file /tmp/pvml.<uid> on the host of
  // master pvmd.  If fatal is TRUE, this process will quit
  // and the function never returns.

  virtual int error(char *msg = "\0", int type = FATAL_ERROR);

  // Extracting operators for unpacking received data.  These
  // operations assume that pvm_recv() has been called and data is
  // available and in the correct order.

  PVM_Service& operator >>(char *);
  PVM_Service& operator >>(unsigned char *c);
  PVM_Service& operator >>(unsigned char &c);
  PVM_Service& operator >>(char &c);
  PVM_Service& operator >>(short int &);
  PVM_Service& operator >>(int &);
  PVM_Service& operator >>(long &);
  PVM_Service& operator >>(unsigned short &);
  PVM_Service& operator >>(unsigned int &);
  PVM_Service& operator >>(unsigned long &);
  PVM_Service& operator >>(float &);
  PVM_Service& operator >>(double &);

  // Inserting operators for packing data into sending buffer.

  PVM_Service& operator <<(const char *);
  PVM_Service& operator <<(const unsigned char *c);
  PVM_Service& operator <<(unsigned char c);
  PVM_Service& operator <<(char c);
  PVM_Service& operator <<(short int);
  PVM_Service& operator <<(int);
  PVM_Service& operator <<(long);
  PVM_Service& operator <<(unsigned short);
  PVM_Service& operator <<(unsigned int);
  PVM_Service& operator <<(unsigned long);
  PVM_Service& operator <<(float);
  PVM_Service& operator <<(double);
};

#endif // NO_PVM
#endif
