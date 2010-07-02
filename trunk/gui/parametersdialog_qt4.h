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

#ifndef PARAMETERSDIALOG
#define PARAMETERSDIALOG

#include "ui_parametersdialog_qt4.h"
#include "../mezzo_lib/src/parameters.h"
//#include <Qt3Support>

class ParametersDialog : public QDialog, public Ui::ParametersDialog
{
	Q_OBJECT

public:
	ParametersDialog(QWidget *parent = 0); // inits the dialog
	void set_parameters (Parameters* params);

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

signals:
	void activateZoomFactor(int) const;
	void activatePanFactor(int) const;
	void activateSimSpeed(int) const;

private:
	
	// VARS
	Parameters* theParameters;
};
	
#endif // PARAMETERSDIALOG