/*
 * this is the definition of the dialog to 
 * check the OD information in the Mezzo network
 */
#ifndef  ODCHECKERDLG_H
#define  ODCHECKERDLG_H

#include <QDialog>
#include <QTableWidget>
#include <QTableView>
#include <QStandardItemModel>
#include <QStandardItem>

#include <QString>
#include <Qt3Support>
#include <vector>

// use the generated ui form header  
#include "../mezzoAnalyzer/ui_odcheckdlg.h"

// use the mainwindow and network definitions 
#include "../mezzo_lib/src/network.h"

//class Network;

class ODCheckerDlg : public QDialog, public Ui::ODCheckerDlg
{
   Q_OBJECT
 public:
   ODCheckerDlg(QWidget* parent);
   ~ODCheckerDlg();
   void setNetwork(Network* mezzonet);
   bool getNetworkState(){return networkset_;}
   void setNetworkState(bool networkset){networkset_=networkset;}

 public slots:
   virtual void reject(); // virutal function of QDialog

 private slots:
   void checkOD(bool check_);
   
 private:

	 //properties 
	 QStandardItemModel* itemmodel_; // model to the tableview 
	 int orgId_;
	 int destId_;

	 //internal state
	 bool networkset_;

	 // references 
	 Network* mezzonet_;
	  
};

#endif

