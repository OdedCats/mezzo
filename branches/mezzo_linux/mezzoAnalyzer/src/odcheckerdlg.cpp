#include <QtGui>
#include "odcheckerdlg.h"

/**
 *  constructor of OD check dialog
 * */
ODCheckerDlg::ODCheckerDlg(QWidget* parent):QDialog(parent)
{
   setupUi(this);
   mezzonet_=0;
   orgId_=-1;
   destId_=-1;
   networkset_=false;
   
   QObject::connect(CheckButton, SIGNAL(toggled(bool)), 
		   	        this, SLOT(checkOD(bool)));
   
   // create the model for the tableview
   itemmodel_=new QStandardItemModel(0,3);
   itemmodel_->setHeaderData(0, Qt::Horizontal, tr("Origin"));
   itemmodel_->setHeaderData(1, Qt::Horizontal, tr("Destination"));
   itemmodel_->setHeaderData(2, Qt::Horizontal, tr("Route"));
   ODTableView->setModel(itemmodel_);
   ODTableView->hide();
   
   // lay out the size of the dialog
   layout()->setSizeConstraint(QLayout::SetFixedSize);
   
}

/**
 * destructor of OD check dialog 
 */
ODCheckerDlg::~ODCheckerDlg()
{
	// only release the properties of this dialog
	delete itemmodel_;
}

// implement the virtual public slot "reject" function
void ODCheckerDlg::reject()
{	
	//trigger the checkbutton
	CheckButton->setChecked(false);
	// call the virtual function of the base class
	QDialog::reject();
}


/**
 * A slot function connected with "Check" button
 * to check the traffic on routes with the pair 
 * of OD selected
 * */
void ODCheckerDlg::checkOD(bool check_)
{
	// start to search the route with 
	// the inputed OD pair
	if(check_){
		infolabel->setText(QString("Search the route with the OD..."));
		vector <ODpair*>& odpairs=mezzonet_->get_odpairs();
		
		
		//infolabel->setText(QString("Routes corresponding to the OD are listed!"));
	}
	else{
		infolabel->setText(QString("Please choose an OD pair"));
	}
	ODTableView->setVisible(check_);
}

/**
 * Set the mezzo network and so the OD range to the comboxes
 */
void ODCheckerDlg::setNetwork(Network* mezzonet)
{
	networkset_=true;
	mezzonet_=mezzonet;
	origcomb->clear();
	destcomb->clear();

	// get the list of origins and destinations
	vector<Origin*>& origs=mezzonet_->get_origins();
	vector<Destination*>& dests=mezzonet_->get_destinations();
	
	// attach string lists to the combo box for OD
	for(int i=0; i<origs.size(); i++){
		origcomb->addItem(QString::number(origs[i]->get_id()));
	}
	for(int i=0; i<dests.size(); i++){
		destcomb->addItem(QString::number(dests[i]->get_id()));
	}
}

