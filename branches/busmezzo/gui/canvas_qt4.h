/** 
 * change: add the mezzo analyzer dialog
 * last modified: Xiaoliang Ma 
 * 2007-07-13
 */

#ifndef MAINFORM
#define MAINFORM

#include <Qt3Support>
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
	
	void loop();
	void paintEvent(QPaintEvent *event );
	
private:
//FUNCTIONS
	void copyPixmap();
	
	void displaytime(double time);
	
	
//VARS
	
    int panpixels;
    bool exited;
    double scalefactor;
    int panelx, panely;
    int dx,dy;
    double scale;
    double runtime, currtime;
    QTimer* timer;
    QPixmap pm1, pm2;
    Network theNetwork;
	Parameters* theParameters;
    QStringList files;
    QString filename;
    QWMatrix wm;
    int panfactor;
    bool breaknow;
    QStatusBar* statusbar;
    bool initialised;
	
	ParametersDialog* pmdlg;
	ODCheckerDlg* od_analyser_;

};


#endif //MAINFORM