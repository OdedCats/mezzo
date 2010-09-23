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

/*
 * definition of the batch run dialog
 *
 * Wilco
 * Last modification: 2008-09-30
 *
 * This dialogue sets up the batch run functions, for automatic iterations
 * for instance to create stable travel times (input = output travel times, i.e.
 * drivers' expectations are in line with the experienced travel times. Also called
 * "dynamic equilibrium"
 */

#ifndef  BATCHRUNDLG_H
#define  BATCHRUNDLG_H

#include <QDialog>
// use the generated ui form header  
#include "../ui_batchrundlg.h"
// network definition
#include "../mezzo_lib/src/network.h"
#include "../mezzo_lib/src/linktimes.h"

class BatchrunDlg : public QDialog, public Ui::Batchrun
{
	Q_OBJECT
public:
	BatchrunDlg(QWidget* parent=0);
	~BatchrunDlg(){}
	void setNetwork(Network* net);
	void show();
	void autorun()
	{
		on_runButton_clicked();
		on_saveButton_clicked();
	}
	
	
	//SLOTS
	private slots:
	void on_runButton_clicked();
	void on_stopButton_clicked();
	void on_saveButton_clicked();
	void on_max_iterations_val_valueChanged(int i);
	

 signals:
    void paintRequest();
	void activateAnalyzeOutput();
	
private:
//private methods
	void run_iterations();
	const bool checkConvergence(const int i, const double rmsn_ltt_, const double relgap_rf_);
//VARS
	QWidget* parent_;
	int max_iter;
	double max_relgap;
	Network* theNetwork;
	LinkTimeInfo* linktimes;
	bool stop_pressed;
};

#endif

