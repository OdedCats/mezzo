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
#include <Qt3Support>
#include <vector>


// use the generated ui form header  
#include "../mezzoAnalyzer/ui_odcheckdlg.h"
// use the mainwindow and network definitions 
#include "../mezzo_lib/src/network.h"
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
	void selectionHandle(const QModelIndex & sel, const QModelIndex & unsel);

 signals:
    void paintRequest();

 private:
    void loadInitOD();
	void clearTableView();
	void unselectRoutes();
	QColor txt2Color(const QString& colortext);

	//properties 
	int orgId_;
	int destId_;
	bool networkset_;

	// references 
	QStandardItemModel* itemmodel_; // model to the tableview
	ODTableViewDelegate* itemdelegate_; // table item control delegate
	Network* mezzonet_;
	ODpair* odsel_;
	vector<std::pair<int, QString>>* paintrouteseq_;  // record of the painted routes

};

#endif

