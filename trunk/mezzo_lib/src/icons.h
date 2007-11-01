/**
 * modification:
 *   add a handler to the linkicon
 *
 * Xiaoliang Ma 
 * last change: 2007-10-30 
 */
 #ifndef ICONS_HH
#define ICONS_HH

//#undef _NO_GUI
#ifndef _NO_GUI
//Added by qt3to4:
#include <QPixmap>
#include <qt3support> // new in QT4 to port from QT3
#include <qpixmap>
#include <qpen>
#include <qpainter>
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
	  Icon( int startx_, int starty_) : startx(startx_), starty(starty_) 
			{selected = false; selected_color = theParameters->selectedcolor;}
	  virtual ~Icon(){};
	  virtual void draw(QPixmap * pm,QMatrix * wm){};
	  virtual const bool within_boundary(const double x, const double y, const int rad);
	  void settext(const string st) {text=QString(st.c_str());}
	  const int get_x()  { return startx;}
	  const int get_y()  { return starty;}
	  void set_selected (const bool sel) {selected = sel;} // sets if object is selected or not
	  const bool get_selected () {return selected;} // gets the selected status of object
	  void set_selected_color (const QColor selcolor) {selected_color = selcolor;}
	  const QColor get_selected_color () {return selected_color;}
  protected:
      QString text;
      int startx, starty; // the icon's position
	  bool selected;
	  QColor selected_color; // local selected color, default is taken from Parameters object 
};

class LinkIcon : public Icon
{
  public:
  	LinkIcon(int x, int y, int tox, int toy );
	virtual ~LinkIcon(){};
	void set_pointers(double * q, double * r);
	void setHandler(bool handle){handler_on_=handle;}
	bool getHandler(){return handler_on_;}
	int getLinkicon_leng(){return linkicon_leng_;}
	virtual const bool within_boundary(const double x, const double y, const int rad);
  	virtual void draw(QPixmap * pm,QMatrix * wm);
  protected:
  	int stopx, stopy;
	int shiftx, shifty;
	int handlex, handley;
    int x2,x3,y2,y3; // points for the arrowhead
  	double * queuepercentage;
  	double * runningpercentage;
	bool handler_on_;
	int linkicon_leng_;
};

class VirtualLinkIcon: public LinkIcon
{
 public:
	VirtualLinkIcon(int x, int y, int tox, int toy) : LinkIcon(x,y,tox,toy) {}
	virtual ~VirtualLinkIcon(){};
	virtual void draw(QPixmap * pm,QMatrix * wm);
 private:
};

class NodeIcon : public Icon
{
  public:
  	NodeIcon(int x, int y) ;       
	virtual ~NodeIcon(){};
    virtual void draw(QPixmap * pm,QMatrix * wm);
  private:
  	int width, height;
      
};

#endif // _NO_GUI

#endif
