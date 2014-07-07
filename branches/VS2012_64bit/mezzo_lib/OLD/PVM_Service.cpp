//-*-c++-*------------------------------------------------------------
// NAME: Simulation Widges
// AUTH: Qi Yang
// FILE: PVM_Service.cc
// DATE: Fri Nov  3 22:34:07 1995
//--------------------------------------------------------------------

#ifndef NO_PVM
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

//#include "Exception.h"
#include "PVM_Service.h"

//#include <Tools/ToolKit.h>
#include <string.h>

int PVM_Service::pid_ = 0;

PVM_Service::PVM_Service(char *module, int stag, int rtag,
						 unsigned long int sz)
	: module_(module),			// module label
	  name_(NULL),				// name of executable
	  sndtag_(stag),
	  rcvtag_(rtag), 
	  default_msg_size_(sz), 
	  tid_(-1),
	  msg_size_(0L),
	  state_(0)
{
}

PVM_Service::PVM_Service()
	: module_(NULL),
	  name_(NULL),
	  sndtag_(0),
	  rcvtag_(0),
	  default_msg_size_(DEFAULT_MSG_SIZE),
	  tid_(-1),
	  msg_size_(0L),
	  state_(0)
{
}

int PVM_Service::pid()
{
    if (pid_) return pid_;
    else return (pid_ = pvm_mytid());
}

// Connect or disconnect a logfile

void
PVM_Service::logfile(FILE *fpo)
{
    pvm_catchout(fpo);
}


void
PVM_Service::name(const char *executable, const char *path)
{
    if (name_) delete [] name_;
    if (path) {
    	  strcpy(name_,path);	
        strcat(name_,executable) ;//StrCopy("%s/%s", path, executable) ;
    } else {
        strcpy(name_,executable); //Copy(executable);
    }
    if (!module_) name_ = module_ ;
}


void
PVM_Service::init(char *module, int stag, int rtag, unsigned long int sz)
{
    module_ = module ;
    sndtag_ = stag;
    rcvtag_ = rtag;
    default_msg_size_ = sz;
}


// Connect this service to a task id.  For example, call
//
//     err = connect(pvm_parent());
//
// will connect this service to the parent process.  This function
// can also be called to connect a sibling process when a task id
// is received from the parent process.

int
PVM_Service::connect(int id)
{
    if (!isInitialized()) {
        cerr << endl << "Error:: PVM service not initialized." << endl;
        return -1;
    }

    tid_ = id;

    if (id < 0 ||
        initsend() < 0) {	// clear sending buffer
        cerr << endl << "Error:: <" << module_ << " (" << name_ << ")"
             << "> cannot connect to PVM service." << endl;
    } else {
        cout << module_ << " (" << name_ << ") connected to PVM service ["
             << hex << id << dec << "]." << endl;
    }
   
    return id;
}


int
PVM_Service::spawn(char **argv, int flag, char *where)
{
    if (!isInitialized()) {
        cerr << endl << "Error:: Cannot spawn a uninitialized "
             << "PVM_Service." << endl;
        return -1;
    }
    char *host = "" ;//Copy(ExpandEnvVars(where)) ;
    strcpy(host,where);
    if (pvm_spawn(name_, argv, flag, host, 1, &tid_) == 1) {
        initsend();
        cout << name_ << " started.  PVM service ["
             << hex << tid_ << dec << "]." << endl;
    } else {
        switch (tid_) {
        case PvmBadParam:
        {
            cerr << "Bad parameters <" << flag << ">";
            break;
        }
        case PvmNoHost:
        {
            int nhost, narch;
            struct pvmhostinfo *info;
            if (pvm_config(&nhost, &narch,  &info) >= 0) {
                cerr << "Your virtual machine contains hosts:" << endl;
                for (int i = 0; i < nhost; i ++) {
                    cerr << " " << info[i].hi_name << endl;
                }
            } else {
                cerr << "PVM is not functioning properly." << endl;
            }
            if (flag != PvmTaskDefault) {
                cerr << "Host \"" << host
                     << "\" is not in the virtual machine." << endl;
            } else {
                cerr << "No host in the virtual machine." << endl;
            }
            break;
        }
        case PvmNoFile:
        {
            cerr << "Cannot find executable." << endl;
            break;
        }
        case PvmNoMem:
        {
            cerr << "Not enough memory." << endl;
            break;
        }
        case PvmSysErr:
        {
            cerr << "PVM daemon not responding." << endl;
            break;
        }
        case PvmOutOfRes:
        {
            cerr << "Out of resources." << endl;
            break;
        }
        }
        cerr << "Module " << module_
             << " (" << name_
             << ") cannot get started on "
             << host << "." << endl;
    }
    free(host);
    return tid_;
}


// Send the packed message if immediately is TRUE of message size has
// exceeded the default_msg_size.

void PVM_Service::flush(int mode)
{
if (mode == SEND_IMMEDIATELY || // preemptive
mode == SEND_END && msg_size_ > 0 ||	// no more to pack
msg_size_ >= default_msg_size_) { // no space to pack
	*this << MSG_END_FLAG;	// flag indicating the end
	if (pvm_send(tid_, sndtag_) < 0 ||
		initsend() < 0) {
        error("sending data", FATAL_ERROR);
	}
	msg_size_ = 0;
}
}


// Receive data from the server. If fatal error, the process will quit
// and this function will never return. Otherwise, it returns id of
// the active buffer or 0 if mode is DO_NOT_WAIT_MSG and there is no
// message arrived.

int
PVM_Service::receive(int mode)
{
    int bufid;
    if (mode == WAIT_MSG) bufid = pvm_recv(tid_, rcvtag_);
    else bufid = pvm_nrecv(tid_, rcvtag_);
    if (bufid < 0) {
        error("receiving data", FATAL_ERROR);
    }
    return bufid;
}

int
PVM_Service::receive(double sec)
{
    const double epsilon = 0.01;
    int bufid;
    if (sec > epsilon) {
        struct timeval tmout;

        tmout.tv_sec = (long)sec;
        double microsec = (sec - (double)tmout.tv_sec) * 1000000.0;
        tmout.tv_usec = (long)microsec;

        // wait the given sec if no data

        bufid = pvm_trecv(tid_, rcvtag_, &tmout);

    } else if (sec < -epsilon) {

        // blocking receiving

        bufid = pvm_trecv(tid_, rcvtag_, NULL);

    } else {

	bufid = pvm_nrecv(tid_, rcvtag_);
  }

  if (bufid < 0) {
	error("receiving data", FATAL_ERROR);
  }
  return bufid;
}


// Preappend 'msg' to the error message of last PVM call and place
// them to the file /tmp/pvml.<uid> on the host of master pvmd.

int
PVM_Service::error(char *msg, int fatal)
{
  const int MSG_LENGTH = 255;
  char buffer[MSG_LENGTH + 1];
  sprintf(buffer, "Error:: %s fail in %s. ", module_, msg);
  int err = pvm_perror(buffer);
  if (fatal) {
//	theException->exit(1);
  }
  return err;
}


// Send a terminate signal to this process 

int
PVM_Service::exit()
{
  if (pid() <= 0) return 0;
  pid_ = 0;
  pvm_catchout(0);
  return pvm_exit();
}

//--------------------------------------------------------------------
// Extracting operators for unpacking received data from the server.
// These operations assume that receive() has been called and data is
// available and in the correct order.
//--------------------------------------------------------------------

PVM_Service& PVM_Service::operator >>(char *b)
{
  if (pvm_upkstr(b) < 0)
	error("extracting a string");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(unsigned char *b)
{
  *this >> (char*)b;
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(unsigned char &c)
{
  if (pvm_upkbyte((char *)&c, sizeof(unsigned char), 1) < 0)
	error("extracting an unsigned char");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(char &c)
{
  if (pvm_upkbyte(&c, sizeof(char), 1) < 0)
	error("extracting a char");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(unsigned short &i)
{
  if (pvm_upkushort(&i, 1, 1) < 0)
	error("extracting a unsigned short");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(short int &i)
{
  if (pvm_upkshort(&i, 1, 1) < 0)
	error("extracting a short");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(unsigned int &i)
{
  if (pvm_upkuint(&i, 1, 1) < 0)
	error("extracting a unsigned int");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(int &i)
{
  if (pvm_upkint(&i, 1, 1) < 0)
	error("extracting an int");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(unsigned long &l)
{
  if (pvm_upkulong(&l, 1, 1) < 0)
	error("extracting a unsigned long");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(long &l)
{
  if (pvm_upklong(&l, 1, 1) < 0)
	error("extracting a long");
  return (*this);
}

PVM_Service&
PVM_Service::operator >>(float &f)
{
  if (pvm_upkfloat(&f, 1, 1) < 0)
	error("extracting a float");
  return (*this);
}

PVM_Service& 
PVM_Service::operator >>(double &d)
{
  if (pvm_upkdouble(&d, 1, 1) < 0)
	error("extracting a double");
  return (*this);
}


//--------------------------------------------------------------------
// Inserting operators for packing data into sending buffer.
//--------------------------------------------------------------------

PVM_Service&
PVM_Service::operator <<(const char *b)
{
  if (pvm_pkstr((char*)b) < 0)
	error("appending a string");
  msg_size_ += strlen(b);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(const unsigned char *b)
{
  char *s = (char*) b;
  *this << s;
  msg_size_ += strlen(s);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(unsigned char c)
{
  if (pvm_pkbyte((char *)&c, sizeof(unsigned char), 1) < 0)
	error("appending an unsigned char");
  msg_size_ += sizeof(unsigned char);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(char c)
{
  if (pvm_pkbyte(&c, sizeof(char), 1) < 0)
	error("appending a char");
  msg_size_ += sizeof(char);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(unsigned short i)
{
  if (pvm_pkushort(&i, 1, 1) < 0)
	error("appending a unsigned short");
  msg_size_ += sizeof(unsigned short);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(short int i)
{
  if (pvm_pkshort(&i, 1, 1) < 0)
	error("appending a short");
  msg_size_ += sizeof(short int);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(unsigned int i)
{
  if (pvm_pkuint(&i, 1, 1) < 0)
	error("appending a unsigned int");
  msg_size_ += sizeof(unsigned int);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(int i)
{
  if (pvm_pkint(&i, 1, 1) < 0)
	error("appending an int");
  msg_size_ += sizeof(int);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(unsigned long l)
{
  if (pvm_pkulong(&l, 1, 1) < 0)
	error("appending a unsigned long");
  msg_size_ += sizeof(unsigned long);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(long l)
{
  if (pvm_pklong(&l, 1, 1) < 0)
	error("appending a long");
  msg_size_ += sizeof(long);
  return (*this);
}

PVM_Service&
PVM_Service::operator <<(float f)
{
  if (pvm_pkfloat(&f, 1, 1) < 0)
	error("appending a float");
  msg_size_ += sizeof(float);
  return (*this);
}

PVM_Service& 
PVM_Service::operator <<(double d)
{
  if (pvm_pkdouble(&d, 1, 1) < 0)
	error("appending a double");
  msg_size_ += sizeof(double);
  return (*this);
}
#endif // NO_PVM
