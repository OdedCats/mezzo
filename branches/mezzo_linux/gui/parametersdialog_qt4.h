#ifndef PARAMETERSDIALOG
#define PARAMETERSDIALOG

#include "ui_parametersdialog_qt4.h"
#include "../mezzo_lib/src/parameters.h"
#include <Qt3Support>

class ParametersDialog : public QDialog, private Ui::ParametersDialog
{
	Q_OBJECT

public:
	ParametersDialog(QWidget *parent = 0); // inits the dialog
	void set_parameters (Parameters* params) {theParameters = params; }

private slots: 
	// Using the Auto-Connect feature with the on_<signal>_<event>() syntax
	void on_LinkIds_toggled(bool value);
	void on_LinkColorButton_clicked ();
	void on_NodeColorButton_clicked();
	void on_QueueColorButton_clicked();
	void on_BgColorButton_clicked();
	void on_LinkThickness_valueChanged(int value);
	void on_NodeThickness_valueChanged( int value);
	void on_QueueThickness_valueChanged(int value);
	void on_NodeRadius_valueChanged(int  value);
	void on_ShowBgImage_toggled( bool value);

private:
	
	// VARS
	Parameters* theParameters;
};
	
#endif // PARAMETERSDIALOG