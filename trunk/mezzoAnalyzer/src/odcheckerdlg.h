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
 * this is the definition of the dialog to 
 * check the OD information in the Mezzo network
 *
 * note: code here assume route list of an odpair 
 * will not be changed during the operation 
 *
 * Xiaoliang Ma
 * Last modification: 2007-08-05
 *
 */

#ifndef  ODCHECKERDLG_H
#define  ODCHECKERDLG_H

#include <QDialog>
#include <QTableWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QModelIndex>
#include <QComboBox>
#include <QColor>
#include <QString>
//#include <Qt3Support>
#include <vector>


// use the generated ui form header  
#include "../ui_odcheckdlg.h"
// use the mainwindow and network definitions 
#include "../../mezzo_lib/src/network.h"
// include custom item delegate
#include "odtabledelegate.h"

class ODCheckerDlg : public QDialog, public Ui::ODCheckerDlg
{
	Q_OBJECT
 public:
	ODCheckerDlg(QWidget* parent=0);
	~ODCheckerDlg();
	void setNetwork(Network* mezzonet);
	bool getNetworkState(){return networkset_;}
	void setNetworkState(bool networkset){networkset_=networkset;}
	void loadSelectOD(vector<Node*>& selnodes);

 public slots:
	virtual void reject(); // virutal function of QDialog  

protected:
	bool eventFilter(QObject *obj, QEvent *evt);

 private slots:
	void checkOD(bool check_);
	void loadDestCombwithO(const QString& curtext);
	void loadOrigCombwithD(const QString& curtext);
	void drawRoute(const QString& colortext, const int& index);
	void drawAllRoutes();
	void selectionHandle(const QItemSelection& sel, const QItemSelection& unsel);
	//void keyPressEvent(QKeyEvent* kev);

 signals:
    void paintRequest();

 private:
    void loadInitOD();
	void clearTableView();
	void unselectRoutes();
	QColor txt2Color(const QString& colortext);
	void updateGraph();
	void rmselectedRoutes();
	
	//properties 
	int orgId_;
	int destId_;
	bool networkset_;
	bool allroutesdrawn_;
	
	// selection properties
	int rowCnt_, colCnt_;
	vector<int> rowcounterlist_;

	// references 
	QStandardItemModel* itemmodel_; // model to the tableview
	ODTableViewDelegate* itemdelegate_; // table item control delegate
	Network* mezzonet_;
	ODpair* odsel_;
	vector< std::pair<int, QString> >* paintrouteseq_;  // record of the painted routes

};

#endif

