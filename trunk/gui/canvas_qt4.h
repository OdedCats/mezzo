/** 
 * changes: 
 * 1)add the mezzo analyzer dialog
 * 2)solve the zooming problems 
 * last modified: Xiaoliang Ma 
 * 2007-09-26
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
	void on_savescreenshot_activated();
	void on_loadbackground_activated();
	void on_breakoff_activated ();
	void on_run_activated();	
	void on_parametersdialog_activated();
	void on_inspectdialog_activated();
	void on_simspeed_valueChanged( int  value);
	void on_zoomfactor_valueChanged( int value);
	void on_panfactor_valueChanged( int value );
	void on_saveresults_activated();
		
	// other slots	
	void keyPressEvent( QKeyEvent *e );
	void mousePressEvent(QMouseEvent *event );
	void mouseMoveEvent(QMouseEvent *event );  
	
	void loop();
	void paintEvent(QPaintEvent *event );
	void copyPixmap();

private:
//FUNCTIONS
	void displaytime(double time);
	
	
//VARS
	int start_x ; // the x coordinate of the upper right corner of the canvas
    int start_y ; // the y coordinate of the upper right corner of the canvas
	QPoint canvas_center; // the center of the canvas
	int panelx, panely;
    int dx,dy;
    bool exited;

	// xiaoliang work variables on zooming
	QWMatrix wm;			  // general world matrix for painting	
    QWMatrix mod2stdViewMat_; //define transition from basic model to standard view
	QWMatrix viewMat_;		  //define transition from standard view to current view
	QSize viewSize_;
	QPixmap pm1, pm2; //shared
	double scalefactor;
    double scale;
    int panfactor;
	int panpixels;

	double runtime, currtime;
    QTimer* timer;
    Network theNetwork;
	Parameters* theParameters;
    QStringList files;
    QString filename;
    bool breaknow;
    QStatusBar* statusbar;
    bool initialised;
	
	// sub dialogs
	ParametersDialog* pmdlg;
	ODCheckerDlg* od_analyser_;

};


#endif //MAINFORM