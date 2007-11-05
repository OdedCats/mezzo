/*
* implementation of the graphic interface
*
* last modification: Xiaoliang Ma 2007-10-31
*	
* solve the zooming problem by reimplementation through matrix operations
*	(It is necessary to create a document to explain the principle later since
*    the implementation is quite tricky and the code needs to be further clean 
*    up as while!)
*
* change the GUI: move the paramter setting to the parameter dialog
* coding the object selection functions
*
*/

#include <qmessagebox.h>
#include "canvas_qt4.h"

MainForm::MainForm(QWidget *parent)
: Q3MainWindow(parent)
{
	setupUi(this);
	
	// INITIALIZATION
	panelx=Canvas->width();
	panely=Canvas->height();
	start_x=Canvas->x();
	start_y=Canvas->y();
	yadjust_=54;
	
	scalefactor=1.50;
	panfactor=20;
	panpixels=20;
	scale=1.0;
	dx=0;
	dy=0;

	// states
	initialised=false;
	lmouse_pressed_=false;
	keyN_pressed_=false;
	keyL_pressed_=false;
	nodes_sel_=vector<Node*>();
	links_sel_=vector<Link*>();

	// implementation of view zooming and panning
	//xiaoliang
	mod2stdViewMat_=QWMatrix(1,0,0,1,0,0);
	viewMat_=QWMatrix(1,0,0,1,0,0);
	viewSize_=QSize(panelx,panely);
	pm1=QPixmap(viewSize_);
	pm2=pm1;
	//xiaoliang
	
	timer = new QTimer( this );
	breaknow=false;
	connect( timer, SIGNAL(timeout()), this, SLOT(loop()) );
	runtime=1.0; 
	currtime=0.0;
	
	//canvas_center = QPoint(start_x + (panelx /2) , start_y + (panely / 2));
	//wm.scale(scale,scale); 
	//statusbar = this->statusBar();
	//statusbar->showMessage("Load a master file");
	exited = false;
	theParameters=theNetwork.get_parameters();
	
	// need to figure out if this affect the simulation
	//simspeed->setValue(static_cast<int> (  theParameters->sim_speed_factor * 100 ));
	
	// Parameters dialog
	pmdlg = new ParametersDialog (this);
	
	// construction of the MezzoAnalyzer dialog 
	od_analyser_=new ODCheckerDlg();
	QObject::connect(od_analyser_, SIGNAL(paintRequest()), 
					 this, SLOT(copyPixmap()));

	// zoom by window
	zoomrect_=0;
	zoombywin_triggered_=false;
	QObject::connect(zoombywin, SIGNAL(toggled(bool)), this,
					 SLOT(on_zoombywin_triggered(bool)));

	// show link handlers
	QObject::connect(linkhandlemark, SIGNAL(toggled(bool)), this,
					 SLOT(on_showhandle_triggered(bool)));

	// connect the signals from parameter dialog
	QObject::connect(pmdlg, SIGNAL(activateZoomFactor(int)), this,
					 SLOT(on_zoomfactor_valueChanged(int)));
	QObject::connect(zoombywin, SIGNAL(activatePanFactor(int)), this,
					 SLOT(on_panfactor_valueChanged(int)));
	QObject::connect(zoombywin, SIGNAL(activateSimSpeed(int)), this,
					 SLOT(on_simspeed_valueChanged(int)));

	// deactive the actions except the open masterfile
	activateToolbars(false);
    
	// hide the status bar
	statusBar()->hide();
	simprogress_widget->setVisible(false);
}

void MainForm::activateToolbars(bool activated)
{
	savescreenshot->setEnabled(activated);
    run->setEnabled(activated);
    breakoff->setEnabled(activated);
    zoomin->setEnabled(activated);
    zoomout->setEnabled(activated);
	zoombywin->setEnabled(activated);
	linkhandlemark->setEnabled(activated);
    viewSet_ParametersAction->setEnabled(activated);
    parametersdialog->setEnabled(activated);
    loadbackground->setEnabled(activated);
    saveresults->setEnabled(activated);
    inspectdialog->setEnabled(activated);
	if(activated)
		status_label->setText("network loaded");
	else
		status_label->setText("uninitialized");

}

void MainForm::showCanvasinfo()
{
	panelx=Canvas->width();
	panely=Canvas->height();
	start_x=Canvas->x();
	start_y=Canvas->y();
	QString mesg=QString("Canvas X:%1, Y:%2, W:%3, H:%4"
			                 ).arg(start_x).arg(start_y).arg(panelx).arg(panely);
	mouse_label->setText(mesg);
}

void MainForm::updateCanvas()
{
	theNetwork.redraw();
	copyPixmap();
}

// AUTOCONNECTED SLOTS
void MainForm::on_quit_activated()
{
	theNetwork.~Network(); 
	od_analyser_->~ODCheckerDlg();
	close();
}

void MainForm::on_openmasterfile_activated()
{
	QString fn = "";
	//fn = (Q3FileDialog::getOpenFileName(QString::null,"mezzo and MiMe Files (*.mime *.mezzo)", this ) );
	// move to QT4 equivalent:
	fn = (QFileDialog::getOpenFileName(this, "Select a MEZZO master file", QString::null,"Mezzo Files (*.mime *.mezzo)") );
	// Open master file
	if ( !fn.isEmpty() ) 
	{
		// strip the dir from the filename and give to the Network
		int pos = fn.lastIndexOf ('/');
		QString workingdir = fn.left(pos+1);
		theNetwork.set_workingdir (workingdir.latin1());
		// make a STL compatible string
		string name=fn.latin1();
		if (theNetwork.readmaster(name))
			runtime=theNetwork.executemaster(&pm2,&wm);
		else
		{
			cout << "ERROR READING THE MASTER FILE: " << name.c_str() << " Exiting" << endl;
			close();
		}
		
		// initialize the buttons
		initialised=true;
		activateToolbars(initialised);

		// initialize the network graphic
		mod2stdViewMat_=theNetwork.netgraphview_init();
		wm=mod2stdViewMat_;
		updateCanvas();
		//statusbar->message("Initialised");
	}	
}

void MainForm::on_zoomin_activated()
{
	// the view center
	int xviewcenter=viewSize_.width()/2;
	int yviewcenter=viewSize_.height()/2;
	QWMatrix tempMat;
	scale*=scalefactor;

	// transfer matrix
	tempMat.reset();
	tempMat.translate(xviewcenter, yviewcenter);
	tempMat.scale(scalefactor, scalefactor);
	tempMat.translate(-xviewcenter,-yviewcenter);

	// new transfer matrix from standard view to current view 
	viewMat_=viewMat_*tempMat;
	// new general tranfer matrix
	wm=mod2stdViewMat_*viewMat_;
	panfactor=static_cast<int>(0.5+(double)panpixels/scale);
	//update the canvas
	updateCanvas();
}

void MainForm::on_zoomout_activated()
{	
	// the view center
	int xviewcenter=viewSize_.width()/2;
	int yviewcenter=viewSize_.height()/2;
	QWMatrix tempMat;

	scale/=scalefactor;
	tempMat.reset();
	tempMat.translate(xviewcenter, yviewcenter);
	tempMat.scale(1/scalefactor, 1/scalefactor);
	tempMat.translate(-xviewcenter,-yviewcenter);
	viewMat_=viewMat_*tempMat;
	wm=mod2stdViewMat_*viewMat_;
	panfactor=static_cast<int>(0.5+(double)panpixels/scale);

	updateCanvas();
}

void MainForm::on_zoombywin_triggered(bool triggered)
{
	zoombywin_triggered_=triggered;
}

void MainForm::on_showhandle_triggered(bool triggered)
{
	//update all linkicons property on handler 
	if(this->initialised){
		vector<Link*> alllinks=theNetwork.get_links();
		for( unsigned i=0; i<alllinks.size(); i++)
			alllinks[i]->get_icon()->setHandler(triggered);
		updateCanvas();
	}
}

void MainForm::on_savescreenshot_activated()
{
	QString fn = QFileDialog::getSaveFileName(this, "Save Image", QString::null, "PNG Files (*.png)" );
    if (!fn.isEmpty())
       pm1.save(fn,"PNG");
}

void MainForm::on_loadbackground_activated()
{
	QString fn( QFileDialog::getOpenFileName(this, "Open background image",QString::null,"PNG Files (*.png)" ) );
    if (!fn.isEmpty())
    {
		string haha=fn.latin1();
		theNetwork.set_background (haha);
    }
}

void MainForm::on_breakoff_activated()
{
	breaknow=true;
}

void MainForm::on_run_activated()
{
	simprogress_widget->setVisible(true);
	breaknow=false;
	loop();
}

void MainForm::on_zoomfactor_valueChanged( int value)
{
	scalefactor = value / 100.0;
}


void MainForm::on_panfactor_valueChanged( int value )
{
   panpixels = value;
}

void MainForm::on_parametersdialog_activated()
{
	pmdlg->set_parameters(theNetwork.get_parameters());
    pmdlg->show();
	pmdlg->raise();
	pmdlg->activateWindow();
} 

/**
* if the mezzoAnalyzer is triggered
*/
void MainForm::on_inspectdialog_activated()
{
    // if initialized (network is created)
	if (this->initialised){

		// set the network if necessary
		if(!od_analyser_->getNetworkState()){
			od_analyser_->setNetwork(&theNetwork);
			od_analyser_->setNetworkState(true); 
		}
		
		// start mezzo analyzer dialog
		od_analyser_->loadSelectOD(nodes_sel_);
		od_analyser_->show();
		od_analyser_->activateWindow();
	
	}else{ 
		// warn to load the network 
		QMessageBox::warning(this, "Network not loaded", "Please load a network first!", 
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
	}

}


void MainForm::on_saveresults_activated()
{
	 theNetwork.writeall();
}

////////////////////////////////////////////////
// NORMAL PRIVATE METHODS
////////////////////////////////////////////////

void MainForm::loop()
{
	copyPixmap();
	int msecs (pmdlg->refreshrate->value());
	int updatefac (pmdlg->updatefactor->value());
	if (!breaknow)
	timer->start( msecs, TRUE ); // ... mseconds single-shot timer 
	currtime=theNetwork.step(((updatefac/100)*msecs/1000.0));
	progressbar->setProgress(static_cast<int>(100.0*currtime/runtime));
	//LCDNumber1->display(static_cast<int>(currtime));
	displaytime(currtime);
	if (currtime>=runtime)
	{
		breaknow=false;
	}
}

void MainForm::displaytime(double time)
{
	QString s_secs, s_mins, s_hrs;
    int secs = static_cast<int> (time + theParameters->zerotime);
    int hours = static_cast<int> (secs/3600);
    int mins = static_cast<int> ((secs-(hours*3600))/60);
    secs = static_cast<int> (secs - (hours*3600) - (mins*60));
    if (secs < 10)
		s_secs = QString("0%1").arg(secs);
    else 
		s_secs= QString("%1").arg(secs);
    if (mins < 10)
		s_mins = QString("0%1").arg(mins);
    else 
		s_mins= QString("%1").arg(mins);
    if (hours < 10)
		s_hrs = QString("0%1").arg(hours);
    else 
	s_hrs= QString("%1").arg(hours);
    QString string = s_hrs + ":" + s_mins + ":" + s_secs ;
    LCDNumber->display(string);
}

void MainForm::copyPixmap()
{	
	pm1=pm2;
	Canvas->setPixmap(pm1);
	Canvas->repaint();  
}

void MainForm::paintEvent(QPaintEvent *  event )
{}

void MainForm::seed(int sd )
{
   theNetwork.seed(sd);
}

void MainForm::on_simspeed_valueChanged( int  value)
{
    theParameters->sim_speed_factor =(value/100);
}

/**
* key press event handling
*/
void MainForm::keyPressEvent( QKeyEvent *e )
{
	if (!initialised) return;
		 
	panfactor=static_cast<int>(0.5+(double)panpixels / wm.m11());
	switch (e->key() ) 
	{
		case (Qt::Key_Plus):	// zoom in
			on_zoomin_activated();
			break;
		case (Qt::Key_Minus):	// zoom out
			on_zoomout_activated();	 
			break;
		case (Qt::Key_Up):		// pan up
			dy=panfactor;
			wm.translate(0,dy);
			viewMat_=mod2stdViewMat_.invert()*wm;
			updateCanvas();
			break;   
		case (Qt::Key_Down):	// pan down
			dy=-panfactor;
			wm.translate(0,dy);	
			viewMat_=mod2stdViewMat_.invert()*wm;
			updateCanvas();
			break;       
		case (Qt::Key_Left):	// pan left
			dx=panfactor;
			wm.translate(dx,0);
			viewMat_=mod2stdViewMat_.invert()*wm;
			updateCanvas();
			break; 
		case (Qt::Key_Right):	// pan right
			dx=-panfactor;
			wm.translate(dx,0);
			viewMat_=mod2stdViewMat_.invert()*wm;
			updateCanvas();
			break;
		case (Qt::Key_C):		// return to initial view with a central image
			wm=mod2stdViewMat_;
			viewMat_.reset();
			scale=1;
			panfactor=panpixels;
			updateCanvas();
			break;
		case(Qt::Key_N):
			keyN_pressed_=true;
			break;
		case(Qt::Key_L):
			keyL_pressed_=true;
			break;
	} // end of switch
}

/*
* key release event handling
*/
void MainForm::keyReleaseEvent(QKeyEvent *kev)
{
	if (!initialised) return;
	switch (kev->key()){
		case(Qt::Key_N):
			keyN_pressed_=false;
			break;
		case(Qt::Key_L):
			keyL_pressed_=false;
			break;
	}
}

/**
* mouse press event handling
*/
void MainForm::mousePressEvent ( QMouseEvent * event )
{
	if (!initialised) return;

	// left mouse button pressed
	if (event->button() == Qt::LeftButton) 
	{
		// mouse position relative to the up-left corner of the Canvas
		int	x_current = event->x() - start_x; 
		int	y_current = event->y() - start_y - yadjust_; 
		lmouse_pressed_=true;
		
		if (keyN_pressed_) //node selecting mode
		{
			QPoint pos_view = QPoint(x_current,y_current);
			QString mesg=QString("Mouse_X %1, Mouse_Y %2").arg(x_current).arg(y_current);
			mouse_label->setText(mesg);

			QMatrix inv = wm.inverted();
			QPoint pos_model = inv.map(pos_view);
			selectNodes(pos_model);
		}
		else if(keyL_pressed_) //link selecting mode
		{
			QPoint pos_view = QPoint (x_current,y_current);
			QString mesg=QString("Mouse_X %1, Mouse_Y %2").arg(x_current).arg(y_current);
			mouse_label->setText(mesg);

			QMatrix inv = wm.inverted();
			QPoint pos_model = inv.map(pos_view);
			selectLinks(pos_model);
		}
		else // none selection mode
		{   
			// when zoombywin button triggered
			if (zoombywin_triggered_)
				zoomrect_=new QRect(x_current, y_current, 0, 0);
			
			// clear the selected nodes and links 
			unselectNodes();
			unselectLinks();

			// show the current mouse position 
			QPoint pos_view = QPoint (x_current,y_current);
			QString mesg=QString("Mouse: X %1, Y %2").arg(x_current).arg(y_current);
			mouse_label->setText(mesg);
		}
		// redraw network so that selcted or unselected 
		// nodes or links are redrawn 
		updateCanvas();
    }
	else if(event->button()==Qt::RightButton)
	{
		
	}
}

/**
* mouse move event handling
*/
void MainForm::mouseMoveEvent(QMouseEvent* mev)
{
	if (!lmouse_pressed_||keyN_pressed_||keyL_pressed_) return;

	if(zoombywin_triggered_)
	{
		// do nothing if no zoom window is drawn
		if(!zoomrect_) return;
		
		// mouse position relative to the up-left corner of the Canvas
		int	x_current = mev->x() - start_x; 
		int	y_current = mev->y() - start_y - yadjust_;
			
		// neglect the rectangle extending to the negative direction
		if (x_current<=zoomrect_->left()||y_current<=zoomrect_->top())
		{
			// remove the drawn rectangle by copy the stored
			// pixelmap
			copyPixmap();
			return;
		}
		else // draw a rectangle
		{
			zoomrect_->setRight(x_current);
			zoomrect_->setBottom(y_current);

			// draw the rect on pm1 but keep pm2 untouched 
			// so that need not to draw network when updating 
			drawZoomRect();
		}
	}
}

/**
* mouse release event
*/
void MainForm::mouseReleaseEvent(QMouseEvent* mev) 
{
	if (mev->button() == Qt::LeftButton)
	{
		if(zoombywin_triggered_&&zoomrect_)
			zoomRectArea();
	}
	else 
	{
	}
	
	if(zoomrect_)
	{
		delete zoomrect_;
		// it is very important to make sure 
		// that null pointer value is 0
		zoomrect_=0; 
	}
	lmouse_pressed_=false;
}

/** 
* select nodes by mouse pointer
*/
void MainForm::selectNodes(QPoint pos)
{
	// remove links selected
	unselectLinks();

	vector<Node*> allnodes=theNetwork.get_nodes();
	int rad=theParameters->node_radius/mod2stdViewMat_.m11();

	for( unsigned i=0; i<allnodes.size(); i++){
		if (allnodes[i]->get_icon()->within_boundary(pos.x(),pos.y(),rad))
		{
			nodes_sel_.push_back(allnodes[i]);
			break;
		}
	}
	QString mesg;
	for( unsigned i=0; i<nodes_sel_.size();i++){
		nodes_sel_[i]->get_icon()->set_selected(true);
		nodes_sel_[i]->get_icon()->set_selected_color(Qt::blue);
		if (i==0)
			mesg=QString("Selected node: %1").arg(nodes_sel_[0]->get_id());
		else
			mesg+=QString(",%1").arg(nodes_sel_[i]->get_id());		
	}	
	mouse_label->setText(mesg);
}

/**
* selct links by mouse pointer
*/
void MainForm::selectLinks(QPoint pos)
{
	// remove nodes selected
	unselectNodes();

	vector<Link*> alllinks=theNetwork.get_links();
	int rad=5;

	for( unsigned i=0; i<alllinks.size(); i++){
		if (alllinks[i]->get_icon()->within_boundary(pos.x(),pos.y(),rad))
		{
			links_sel_.push_back(alllinks[i]);
			break;
		}
	}

	QString mesg;
	for( unsigned i=0; i<links_sel_.size();i++){
		links_sel_[i]->set_selected(true);
		links_sel_[i]->set_selected_color(Qt::blue);
		if (i==0)
			mesg=QString("Selected link: %1").arg(links_sel_[0]->get_id());
		else
			mesg+=QString(",%1").arg(links_sel_[i]->get_id());		
	}	
	mouse_label->setText(mesg);
}

/**
* unselect nodes
*/
void MainForm::unselectNodes()
{
	for( unsigned i=0; i<nodes_sel_.size(); i++){
		nodes_sel_[i]->get_icon()->set_selected(false);
	}
	nodes_sel_.clear();		
}

/**
* unselect links
*/
void MainForm::unselectLinks()
{
	for( unsigned i=0; i<links_sel_.size(); i++){
		links_sel_[i]->set_selected(false);
	} 
	links_sel_.clear();
}

/*
* draw the rectangle for zooming
*/
void MainForm::drawZoomRect()
{
	// compute the rectangle to the current view
	int rect_w = zoomrect_->width();
	int rect_h = zoomrect_->height();
	int view_w = viewSize_.width();
	int view_h = viewSize_.height();

	// do nothing when rectangle is a point 
	if (rect_w==0||rect_h==0) return;

	double scale_x = view_w/rect_w;
    double scale_y = view_h/rect_h;
	if (scale_x >= scale_y){	
		double newscale=scale_x;	
		int newheight=view_h/newscale;
		zoomrect_->setHeight(newheight);
   }
   else{
		double newscale=scale_y;	
		int newwidth=view_w/newscale;
		zoomrect_->setWidth(newwidth);
   }

	// copy pixmap
	pm1=pm2;
	
	//start painter
	QPainter paint(&pm1); 
	paint.setRenderHint(QPainter::Antialiasing); // smooth lines
	// set the pen
	QPen pen1(Qt::black, 1);
	pen1.setStyle(Qt::DashLine);
	paint.setPen(pen1);
	paint.drawRect(*zoomrect_); 
	paint.end();
	// update the canvas with pm1 and leave pm2 as a copy 
	// of the network
	Canvas->setPixmap(pm1);
	Canvas->repaint();
}

/** 
* zoom the window area selected 
*/
void MainForm::zoomRectArea()
{
	// do nothing when rectangle is a point 
	if (zoomrect_->width()==0||zoomrect_->height()==0) return;

	// compute the scale
	double rect2view_factor=viewSize_.width()/zoomrect_->width();
	scale*=rect2view_factor;

	// transfer matrix
	QWMatrix tempMat;
	tempMat.reset();
	tempMat.scale(rect2view_factor, rect2view_factor);
	tempMat.translate(-zoomrect_->left(),-zoomrect_->top());
	
	// new transfer matrix from standard view to current view 
	viewMat_=viewMat_*tempMat;
	// new general tranfer matrix
	wm=mod2stdViewMat_*viewMat_;
	panfactor=static_cast<int>(0.5+(double)panpixels/scale);
	//update the canvas
	updateCanvas();	
}