//#undef _NO_GUI
#ifndef _NO_GUI

#include "icons.h"
#include "parameters.h"
#include <math.h>
#include <qimage.h>
#include <QPixmap>
#include <iostream>

/////////////////////////////////////////
// Drawing functions
/////////////////////////////////////////

Drawing::Drawing() {bg_set=false;bpm=NULL;}

Drawing::~Drawing()
{
  icons.empty();
}

void Drawing::add_icon(Icon* icon)
{
 	icons.push_back(icon);
 	//cout <<"nr of icons in drawing "<< icons.size() << endl;
}

void Drawing::draw(QPixmap* pm,QMatrix * wm)
{
	if (!basematrix_set)
	{
		bwm=*wm;
		basematrix_set=true;
	}
	if (bpm && bg_set && theParameters->show_background)
	{
		*pm = *bpm;	
	}
	else	
	{
		pm->resize(800,600); // standard width & height
		pm->fill(theParameters->backgroundcolor); // fill with white background
	}
	for (list<Icon*>::iterator iter=icons.begin(); iter != icons.end(); iter++)
		(*iter)->draw(pm,wm);
}


 vector <int> Drawing::get_boundaries()

 {
    int min_x=0, min_y=0, max_x=0, max_y=0;
    
    vector <int> result;
    for (list <Icon*>::iterator iter=icons.begin(); iter != icons.end(); iter++)
    {
       register int x = (*iter)->get_x();
       if (x < min_x)
         min_x = x;
       if (x > max_x)
         max_x=x;
       register int y = (*iter)->get_y();    
       if (y < min_y)
         min_y=y;
       if (y > max_y)
         max_y=y;
    }
    result.push_back(min_x);
    result.push_back(min_y);
    result.push_back(max_x);
    result.push_back(max_y);
    
    return result;
 }         

////////////////////////////////////////
// Icon functions
////////////////////////////////////////
bool Icon::within_boundary(double x, double y, int rad)
{
	if (x<=startx+rad && x>=startx-rad)
		if(y<=starty+rad && y>=starty-rad)
			return true;
	return false;
}


////////////////////////////////////////
// LinkIcon functions
////////////////////////////////////////

LinkIcon::LinkIcon(int x, int y, int tox, int toy ): Icon (x, y), stopx(tox), stopy(toy)
{
	// link icon handler

	int vx=stopx-startx;
	int vy=stopy-starty;
	linkicon_leng_=sqrt(double(vx*vx+vy*vy));

	double q=((theParameters->queue_thickness/2.0)+1.0);
	// calculating the shift to make the links excentric 
	// (start & end at the side of nodes)
	if (vx==0)
	{
		shifty=static_cast<int>(0.0);
		if (vy>0)
			shiftx=static_cast<int>(-q);
		else
			shiftx=static_cast<int>(q);
				
	}
	else if (vy==0)
	{
		shiftx=static_cast<int>(0.0);
		if (vx<0)
			shifty=static_cast<int>(-q);
		else
			shifty=static_cast<int>(q);
	}
	else
	{
		double p=vy;
		p=p/vx;
		double move=(sqrt( (q*q)/(1+(p*p)) ) );
		int tempy=abs(static_cast<int>  (move));
		int tempx=abs(static_cast<int> (move*(p)));
		//shift is the orhogonal vector to the link, used to shift the block
		// take care of the sign for righthandtraffic
		if (vx<0)
			shifty=-tempy;
		else
			shifty=tempy;
		if (vy>0)
			shiftx=-tempx;
		else
			shiftx=tempx;
	}

	handler_on_=true;
	handlex=(2*startx+stopx)/3+shiftx;
	handley=(2*starty+stopy)/3+shifty;

	// Setting the points for the arrowhead
	x2=static_cast <int> ( 0.96 *vx+startx);
	x3= static_cast <int> (0.96 *vx+startx+2*shiftx);
	y2=static_cast <int> (0.96 *vy+starty);
	y3= static_cast <int> (0.96 *vy+starty+2*shifty);   
  
}

void LinkIcon::set_pointers(double * q, double * r)
{
	 queuepercentage=q;
	 runningpercentage=r;
}

void LinkIcon::draw(QPixmap * pm,QMatrix * wm)   // draw the stuff on pixmap

{
	// init the painter
	QPainter paint(pm); // automatic paint.begin()
	paint.setRenderHint(QPainter::Antialiasing);
    
	// set the world matrix that controls zoom and translation of  the drawn image
	paint.setWorldMatrix(*wm);
	QPen pen1;
	if (!selected)
		pen1 =QPen( theParameters->linkcolor , theParameters->link_thickness); // pen for the link base
	else
		pen1 =QPen( selected_color , theParameters->selected_thickness); // pen for the link base
	
	QPen pen2 ( theParameters->queuecolor , theParameters->queue_thickness); // pen for queue
#ifdef FIXED_RUNNING
	int r,g,b;
	g=0;
	b=0;
	/*** TEMPORARY TO COMPLY WITH SThlms Stad
	double perc_density=(*runningpercentage) / (1.0-(*queuepercentage));
	if (perc_density > 0.50)
	{
        r=255;
    	b=255 - static_cast<int>(((perc_density-0.50)/0.50)*255);
	}
	else
	{
      r=static_cast<int>(((perc_density)/0.50)*255);
      b=255;
	}
	*/
	double perc_density=(*runningpercentage) / (1.0-(*queuepercentage));
	if (perc_density > 80)
	   b=255;
	r=255;
	int width=static_cast<int>((theParameters->link_thickness+theParameters->queue_thickness)*perc_density) ;
	QPen pen3 (QColor(r,g,b),width); // pen for variable (density)
#else
	QPen pen3 ( Qt::blue , theParameters->queue_thickness);
#endif // FIXED_RUNNING

	// draw the center line
	paint.setPen(pen1);
	paint.drawLine( startx+shiftx,starty+shifty, stopx+shiftx,stopy+shifty ); 
	
	// draw an arrow to show the direction of the link
	//  paint.drawLine( stopx+shiftx,stopy+shifty, x2,y2 );
	//  paint.drawLine( stopx+shiftx,stopy+shifty, x3,y3 );
	//  paint.drawLine( x3,y3, x2,y2 );
	
	// drawing the handler 
	if(handler_on_)
	{
		QPen pen_h(theParameters->linkcolor, 1.5*(theParameters->link_thickness));
		paint.setPen(pen_h);
		paint.drawLine(handlex-shiftx/2,handley-shifty/2,
					   handlex+shiftx/2,handley+shifty/2);
	}

	// draw the queue part
   	int x_1=static_cast <int> ((1.0-(*queuepercentage))*(stopx-startx)+startx);   // x and y for queue-end
   	int y_1=static_cast <int> ((1.0-(*queuepercentage))*(stopy-starty)+starty);
    paint.setPen(pen2);
	paint.drawLine( x_1+shiftx,y_1+shifty, stopx+shiftx,stopy+shifty ); // draw the queue

	//draw the running part
	paint.setPen(pen3);
#ifdef FIXED_RUNNING
	if (width>1)
	{	
		//	int sx=static_cast <int> (0.5+shiftx*perc_density);
		//	int sy=static_cast <int> (0.5+shifty*perc_density);
		int sx=shiftx;
		int sy=shifty;
		paint.drawLine(startx+sx,starty+sy, x_1+sx,y_1+sy ); // draw the running segment
	}
#else
	int x_2=static_cast <int> ((1.0-((*queuepercentage)+(*runningpercentage)))*(stopx-startx)+startx);   // x and y for running-part-end
	int y_2=static_cast <int> ((1.0-((*queuepercentage)+(*runningpercentage)))*(stopy-starty)+starty);
	paint.drawLine(x_2+shiftx,y_2+shifty, x_1+shiftx,y_1+shifty ); // draw the running segment
#endif //FIXED_RUNNING
	if (theParameters->draw_link_ids)
	{
		paint.setFont(QFont ("Helvetica", theParameters->text_size));
		QPen pen4 ( Qt::red , 1);
		paint.setPen(pen4);
		paint.drawText (((startx+stopx)/2)+shiftx*theParameters->text_size - 2*theParameters->text_size,((starty+stopy)/2)+shifty*theParameters->text_size, text);
	}
	paint.end();	
}

bool LinkIcon::within_boundary(double x, double y, int rad)
{
	rad=linkicon_leng_/12;
	if (x<=handlex+rad && x>=handlex-rad)
		if(y<=handley+rad && y>=handley-rad)
			return true;
	return false;
}

//////////////////////////////////////////////
// VirtualLinkIcon functions
//////////////////////////////////////////////

void VirtualLinkIcon::draw(QPixmap * pm,QMatrix * wm)   // draw the stuff on pixmap
{

   QPainter paint(pm); // automatic paint.begin()
    paint.setRenderHint(QPainter::Antialiasing); // make nice smooth lines
    paint.setWorldMatrix(*wm);
   QPen pen1 ( Qt::green , 1);

   paint.setPen(pen1);
   paint.drawLine( startx+shiftx,starty+shifty, stopx+shiftx,stopy+shifty ); // draw the center line
   /*
   QPen pen4 ( Qt::red , 1);
   paint.setPen(pen4);
   paint.drawText (((startx+stopx)/2)+5,((starty+stopy)/2)+5, text);
   */
   paint.end();	
}

///////////////////////////////////////////////
// NodeIcon functions
///////////////////////////////////////////////

NodeIcon::NodeIcon( int x, int y): Icon(x,y)
{
 	//startx=x;
 	//starty=y;
 	width=2*theParameters->node_radius;
 	height=2*theParameters->node_radius;
	text="";
}

void NodeIcon::draw(QPixmap *pm,QMatrix *wm)
{
	width=2*theParameters->node_radius;
	height=2*theParameters->node_radius;
	QPainter paint(pm); // automatic paint.begin()
	paint.setRenderHint(QPainter::Antialiasing); // smooth lines
	paint.setWorldMatrix(*wm);

	QPen pen1;
	if (!selected)
		pen1 =QPen(theParameters->nodecolor, theParameters->node_thickness);
	else
		pen1 =QPen(selected_color, 4*(theParameters->node_thickness)); 

	paint.setPen(pen1);
	paint.drawEllipse (startx,starty, width,height ); // draw a line
	paint.end();	
	// draw the stuff on pixmap
}

#endif //_NO_GUI
