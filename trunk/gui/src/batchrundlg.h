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

/*!
 * definition of the SDUE (Stochastic Dynamic Equilibrium) batch run dialog
 * This dialogue sets up the SDUE and Route Search batch run functions
 * Depending on the selected function the model iterates to a SDUE, or runs
 * an outer loop for new route searches, while an inner loop re-converges to SDUE
 * between route searches.
 * Route searches are repeated with random noise on the link travel times, and for
 * each update period (defined by the Parameters->update_interval_routes)
 */

#ifndef  BATCHRUNDLG_H
#define  BATCHRUNDLG_H

#include <QDialog>
// use the generated ui form header  
#include "../ui_batchrundlg.h"
// network definition
#include "../../mezzo_lib/src/network.h"
#include "../../mezzo_lib/src/linktimes.h"

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
	void on_runButton_clicked(); //!< these on...clicked() slots will auto connect to the relevant actions
	void on_stopButton_clicked();
	void on_saveButton_clicked();
	void on_max_iterations_val_valueChanged(int i);

	// Route Search Options tab
	void on_max_route_iterations_val_valueChanged(int i);
	void on_disturbances_cb_clicked(bool checked);
	void on_random_draws_val_valueChanged (int i);
	void on_disturbance_val_textEdited (const QString & text);
	void on_scale_cb_clicked(bool checked);
	void on_scale_val_textEdited (const QString & text);
	void on_prune_cb_clicked(bool checked);
	void on_max_rel_cost_val_textEdited (const QString & text);
	void on_renum_cb_clicked(bool checked);


 signals:
    void paintRequest();
	void center_image();
	void activateAnalyzeOutput();
	
private:
//private methods
	void run_iterations(); //!< A much more elaborate version of the one in Network, providing repainting of the network, progress etc.
	void run_route_iterations(); //!< A much more elaborate version of the one in Network, providing repainting of the network, progress etc.
	const bool checkConvergence(const int i, const double rmsn_ltt_, const double relgap_rf_);
//VARS
	QWidget* parent_;
	int max_iter;
	double max_relgap;
	Network* theNetwork;
	LinkCostInfo* linktimes;
	bool stop_pressed;
};

#endif

