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
 * It contains the main network graph display, and calls the Network->step() repeatedly to simulate
 *
 *
 *
 *  latest changes: 
 * 1)add the mezzo analyzer dialog
 * 2)solve the zooming problems by the qmatrix approach 
 * 3)GUI development
 * last modified: Xiaoliang Ma 
 * 2007-10-22
 */


#ifndef MAINFORM
#define MAINFORM

#include <Qt3Support>
#include <Qfiledialog>
#include <QWMatrix>
#include "ui_canvas_qt4.h"
#include "parametersdialog_qt4.h"
#include "../mezzo_lib/src/parameters.h"
#include "../mezzoAnalyzer/src/odcheckerdlg.h"
#include "../mezzo_lib/src/network.h"

class MainForm : public Q3MainWindow, private Ui::MainForm
{
	Q_OBJECT

public:
	MainForm(QWidget *parent = 0); //!< inits the main form
	void seed(int sd ); //!< sets the seed

	// access ports
	Network* getMezzoNet(){return (theNetwork);}

private slots: 
	// Using the Auto-Connect feature with the on_<signal>_<event>() syntax
	void on_quit_activated(); //!< Quits the program
	void on_stop_activated(); //!< Stops and resets the simulation
	void on_openmasterfile_activated();  //!< Opens a 'Open master file' dialog
	void on_zoomin_activated();  //!< Zooms in on the network
	void on_zoomout_activated();  //!< Zooms out on the network
	void on_zoombywin_triggered(bool);  //!< Zooms in on the indicated rectangle
	void on_showhandle_triggered(bool);  //!< Shows the Link handle icons, enables selecting them
	void on_inselectmode_triggered(bool);  //!< Triggers selection mode
	void on_savescreenshot_activated();  //!< Saves screenshot of network
	void on_loadbackground_activated();  //!< Opens a Load background dialog
	void on_breakoff_activated ();  //!< Pauses the simulation
	void on_run_activated();	 //!< Starts the simulation
	void on_parametersdialog_activated();  //!< Shows parameters dialog
	void on_inspectdialog_activated();  //!< Shows Route inspect dialog
	void on_simspeed_valueChanged(int value);  //!< changes the simspeed
	void on_zoomfactor_valueChanged(int value);  //!< changes the zoom step for zooming in/out
	void on_panfactor_valueChanged(int value );  //!< changes the pan step for panning the network
	void on_saveresults_activated();  //!< Saves the results of the simulation 
		
	// other slots	
	void keyPressEvent(QKeyEvent* e);  //!< handler for key presses
	void keyReleaseEvent(QKeyEvent* kev);  //!< handler for key releases
	void mousePressEvent(QMouseEvent* event);  //!< handler for mous presses
	void mouseDoubleClickEvent(QMouseEvent* mev);  //!< handler for double clicks
	void mouseMoveEvent(QMouseEvent* mev);  //!< handler for mouse moves
	void mouseReleaseEvent(QMouseEvent* mev);   //!< handler for mouse release
	
	void loop();  //!< main simulation - display loop in which the Network->step() is called repeatedly and the GUI is refreshed in between
	void paintEvent(QPaintEvent *event );
	void copyPixmap();  //!< redraws the network on the GUI

private:
//FUNCTIONS
	void activateToolbars(bool activated);
	void displaytime(double time);
	void showCanvasinfo();
	void updateCanvas();
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

	// xiaoliang work variables on zooming
	QWMatrix wm;			  //!< general world matrix from model to current view	
    QWMatrix mod2stdViewMat_; //!< define transition from basic model to standard view
	QWMatrix viewMat_;		  //!< define transition from standard view to current view
	QSize viewSize_;
	QSize canvasOffset; // off set in X and Y of the Canvas to the Mainform
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
    QString filename;
    bool breaknow;
    //QStatusBar* statusbar;
	
	// sub dialogs
	ParametersDialog* pmdlg;
	ODCheckerDlg* od_analyser_;

};


#endif //MAINFORM