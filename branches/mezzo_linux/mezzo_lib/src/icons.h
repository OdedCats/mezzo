
 #ifndef ICONS_HH
#define ICONS_HH

//#undef _NO_GUI
#ifndef _NO_GUI
//Added by qt3to4:
#include <QPixmap>
#include <Qt3Support> // new in QT4 to port from QT3
#include <QPen>
#include <QPainter>
#include <list>
#include "parameters.h"

#define FIXED_RUNNING

using namespace std;

class Icon;

class Drawing
{
  public:
  	Drawing();
  	virtual ~Drawing();
  	void set_background (const char* name) {if (bpm) delete bpm; bpm=new QPixmap(name);bg_set=true;basematrix_set=false;}
  	void add_icon (Icon* icon);
  	virtual void draw(QPixmap * pm,QMatrix * wm);
   vector <int> get_boundaries();        // returns [min_x,min_y, max_x, max_y]
  private:
  	list <Icon*> icons;
    bool bg_set;
	bool basematrix_set;
	QMatrix bwm;
  	QPixmap* bpm;
};


class Icon
{
  public:
      Icon() {selected = false;}
      Icon( int startx_, int starty_) : startx(startx_), starty(starty_) {selected = false;}
      virtual ~Icon();
  		virtual void draw(QPixmap * pm,QMatrix * wm);
		void settext(const string st) {text=QString(st.c_str());}
       int get_x()  { return startx;}
     int get_y()  { return starty;}
	 void set_selected (const bool sel) {selected = sel;}
	 bool get_selected () {return selected;}
  protected:
      QString text;
      int startx, starty; // the icon's position
	  bool selected;
};

class LinkIcon : public Icon
{
  public:
  		LinkIcon(int x, int y, int tox, int toy );
      virtual ~LinkIcon();
  		void set_pointers(double * q, double * r);
  		virtual void draw(QPixmap * pm,QMatrix * wm);
  protected:
  	int  stopx, stopy, shiftx, shifty;
      int x2,x3,y2,y3; // points for the arrowhead
  	double * queuepercentage;
  	double * runningpercentage;
};

class VirtualLinkIcon: public LinkIcon
{
 public:
	VirtualLinkIcon(int x, int y, int tox, int toy) : LinkIcon(x,y,tox,toy) {}
  virtual ~VirtualLinkIcon();
   virtual void draw(QPixmap * pm,QMatrix * wm);
 private:
};

class NodeIcon : public Icon
{
  public:
  		NodeIcon(int x, int y) ;       
      virtual ~NodeIcon();
    virtual void draw(QPixmap * pm,QMatrix * wm);
    
  private:
  		int width, height;
      
};

#endif // _NO_GUI

#endif
