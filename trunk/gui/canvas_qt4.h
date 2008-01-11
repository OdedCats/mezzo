/** 
 * changes: 
 * 1)add the mezzo analyzer dialog
 * 2)solve the zooming problems
 * 3)GUI changes
 * last modified: Xiaoliang Ma 
 * 2007-10-22
 */


#ifndef MAINFORM
#define MAINFORM

#include <Qt3Support>
//#include <q3filedialog.h> // taken out, replaced by Qfiledialog
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
	MainForm(QWidget *parent = 0); // inits the main form
	void seed(int sd ); // sets the seed

	// access ports
	Network* getMezzoNet(){return (&theNetwork);}

private slots: 
	// Using the Auto-Connect feature with the on_<signal>_<event>() syntax
	void on_quit_activated();
	void on_openmasterfile_activated();
	void on_zoomin_activated();
	void on_zoomout_activated();
	void on_zoombywin_triggered(bool);
	void on_showhandle_triggered(bool);
	void on_inselectmode_triggered(bool);
	void on_savescreenshot_activated();
	void on_loadbackground_activated();
	void on_breakoff_activated ();
	void on_run_activated();	
	void on_parametersdialog_activated();
	void on_inspectdialog_activated();
	void on_simspeed_valueChanged(int value);
	void on_zoomfactor_valueChanged(int value);
	void on_panfactor_valueChanged(int value );
	void on_saveresults_activated();
		
	// other slots	
	void keyPressEvent(QKeyEvent* e);
	void keyReleaseEvent(QKeyEvent* kev);
	void mousePressEvent(QMouseEvent* event);
	void mouseDoubleClickEvent(QMouseEvent* mev); 
	void mouseMoveEvent(QMouseEvent* mev);
	void mouseReleaseEvent(QMouseEvent* mev);  
	
	void loop();
	void paintEvent(QPaintEvent *event );
	void copyPixmap();

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

//VARS
	int start_x ; // the x coordinate of the upper right corner of the canvas
    int start_y ; // the y coordinate of the upper right corner of the canvas
	int yadjust_;  // the height of menu and tool bars
	QPoint canvas_center; // the center of the canvas
	int panelx, panely;
    int dx,dy;
    bool exited;

	// xiaoliang work variables on zooming
	QWMatrix wm;			  // general world matrix from model to current view	
    QWMatrix mod2stdViewMat_; //define transition from basic model to standard view
	QWMatrix viewMat_;		  //define transition from standard view to current view
	QSize viewSize_;
	QPixmap pm1, pm2; //shared
	QRect* zoomrect_;
	double scalefactor;
    double scale;
    int panfactor;
	int panpixels;

	// states
	bool initialised;
	bool zoombywin_triggered_;
	bool inselection_;

	//key and mouse states
	bool lmouse_pressed_;
	bool keyN_pressed_;   //node selection
	bool keyL_pressed_;   //link selection

	// network 
	Network theNetwork;
	vector<Node*> nodes_sel_;
	vector<Link*> links_sel_;

	// simulation
	double runtime, currtime;
    QTimer* timer;
	Parameters* theParameters;
    QStringList files;
    QString filename;
    bool breaknow;
    //QStatusBar* statusbar;
	
	// sub dialogs
	ParametersDialog* pmdlg;
	ODCheckerDlg* od_analyser_;

};


#endif //MAINFORM