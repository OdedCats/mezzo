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

/*! Mainform is the main GUI class
 * It contains the main network graph display, and calls the Network->step() repeatedly to simulate and redraw
 *
 */


#ifndef MAINFORM
#define MAINFORM

#include <Qfiledialog>
#include <QMatrix>
#include "ui_canvas_qt4.h"
#include "parametersdialog_qt4.h"
#include "../mezzo_lib/src/parameters.h"
#include "../mezzoAnalyzer/src/odcheckerdlg.h"
#include "../mezzo_lib/src/network.h"
#include "src/batchrundlg.h"
#include "src/outputview.h"
#include "src/positionbackground.h"
#include "src/find.h"

class MainForm : public QMainWindow, private Ui::MainForm
{
	Q_OBJECT

public:
	MainForm(QWidget *parent = 0); //!< inits the main form
	virtual ~MainForm(); //!< destroys and cleans up by calling cleanup()
	
	void seed(int sd ); //!< sets the seed
	void set_filename(const QString fn_) {	
			fn = QDir (fn_).path();
			process_masterfile();}
	void auto_run() // when run from commandline, autorun and save results, then closes the application.
	{
		on_batch_run_activated();
		brdlg->autorun();
		brdlg->close();
		on_quit_activated();
	}
	void process_masterfile();
	
	Network* getMezzoNet(){return (theNetwork);}
	void set_started_from_commandline(const bool value) { started_from_commandline=value;}
	const bool get_started_from_commandline() {return started_from_commandline;}
signals:
	void closing();
private slots: 
	// Using the Auto-Connect feature with the on_<signal>_<event>() syntax
	void on_closenetwork_activated(); //!< Closes the current network and resets to the initial state.
	void on_quit_activated(); //!< Quits the program
	void on_stop_activated(); //!< Stops and resets the simulation
	void on_openmasterfile_activated();  //!< Opens a 'Open master file' dialog
	void on_zoomin_activated();  //!< Zooms in on the network
	void on_zoomout_activated();  //!< Zooms out on the network
	void on_zoombywin_triggered(bool);  //!< Zooms in on the indicated rectangle
	void showhandle_triggered(bool);  //!< Shows the Link handle icons, enables selecting them
	void on_inselectmode_triggered(bool);  //!< Triggers selection mode
	void on_savescreenshot_activated();  //!< Saves screenshot of network
	void on_loadbackground_activated();  //!< Opens a Load background dialog
	void on_breakoff_activated ();  //!< Pauses the simulation
	void on_run_activated();	 //!< Starts the simulation
	void on_batch_run_activated(); //!< Shows the Batch Run dialog
	void on_parametersdialog_activated();  //!< Shows parameters dialog
	void on_inspectdialog_activated();  //!< Shows Route inspect dialog
	void on_finddialog_activated();  //!< Shows Route inspect dialog
	void simspeed_valueChanged(int value);  //!< changes the simspeed
	void zoomfactor_valueChanged(int value);  //!< changes the zoom step for zooming in/out
	void panfactor_valueChanged(int value );  //!< changes the pan step for panning the network
	void on_saveresults_activated();  //!< Saves the results of the simulation 
	void on_actionAnalyzeOutput_toggled(); //!< Turns on output analysis dialogue
	void on_horizontalSlider_valueChanged(); // !< changes output view period
	void on_actionPositionBackground_activated();//!< opens Position Background Dialogue
	void on_actionSlow_toggled(bool); //!< if checked the simulation is slowed down
		
	// other slots	
	void keyPressEvent(QKeyEvent* e);  //!< handle for key presses
	void keyReleaseEvent(QKeyEvent* kev);  //!< handle for key releases
	void mousePressEvent(QMouseEvent* event);  //!< handle for mous presses
	void mouseDoubleClickEvent(QMouseEvent* mev);  //!< handle for double clicks
	void mouseMoveEvent(QMouseEvent* mev);  //!< handle for mouse moves
	void mouseReleaseEvent(QMouseEvent* mev);   //!< handle for mouse release
	
	void loop();  //!< main simulation - display loop in which the Network->step() is called repeatedly and the GUI is refreshed in between
	void paintEvent(QPaintEvent *event );
	void activate_AnalyzeOutput() ;

	void updateCanvas(); //!< redraws the network then calls copyPixmap()
	 
	void copyPixmap();  //!< redraws the network image on the GUI canvas

	void center_image(); //!< centers the image.


private:
//FUNCTIONS
	void activateToolbars(bool activated);
	void displaytime(double time);
	void showCanvasinfo();
	
	void selectNodes(QPoint pos);
	void unselectNodes();
	void selectLinks(QPoint pos);
	void unselectLinks();
	void drawZoomRect();
	void zoomRectArea();
	void resizeEvent(QResizeEvent*);

//VARS
	int start_x ; //!< the x coordinate of the upper right corner of the canvas
    int start_y ; //!< the y coordinate of the upper right corner of the canvas
	int yadjust_;  //!< the height of menu and tool bars
	QPoint canvas_center; //!< the center of the canvas
	int panelx, panely;
    int dx,dy;
    bool exited;
	bool started_from_commandline; // true if run from commandline

	// xiaoliang work variables on zooming
	QMatrix wm;			  //!< general world matrix from model to current view	
    QMatrix mod2stdViewMat_; //!< define transition from basic model to standard view
	QMatrix viewMat_;		  //!< define transition from standard view to current view
	QSize viewSize_;
	QSize canvasOffset; //!< offset in X and Y of the Canvas to the Mainform
	QPixmap pm1, pm2; //!< shared pixmaps on which the network is drawn off-screen
	QRect* zoomrect_;
	double scalefactor;
    double scale;
    int panfactor;
	int panpixels;

	// states
	bool initialised;  //!< True if the simulation has been initialised
	bool zoombywin_triggered_; 
	bool inselection_;

	//key and mouse states
	bool lmouse_pressed_;
	bool keyN_pressed_;   //!< node selection
	bool keyL_pressed_;   //!< link selection

	// network 
	Network* theNetwork;  //!< The main Network object, in which is the entire simulation engine
	vector<Node*> nodes_sel_;  //!< nodes currently selected
	vector<Link*> links_sel_;  //!< links currently selected

	// simulation
	double runtime, currtime;
    QTimer* timer;
	Parameters* theParameters;  //!< The parameters object, which contains the global parameters for the simulation and GUI
    QStringList files;
    QString fn;
    bool breaknow;
    //QStatusBar* statusbar;
	
	// child dialogs
	ParametersDialog* pmdlg;
	ODCheckerDlg* od_analyser_;
	BatchrunDlg* brdlg;
	OutputView* outputview;
	PositionBackground* posbackground;
	FindDialog* finddialog;

};


#endif //MAINFORM