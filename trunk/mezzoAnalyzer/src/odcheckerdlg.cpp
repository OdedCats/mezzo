#include <QtGui>
#include <algorithm>
#include "odcheckerdlg.h"
#include "assist.h"
using namespace std;

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
   itemmodel_=new QStandardItemModel(0,4);
   itemmodel_->setHeaderData(0, Qt::Horizontal, tr("Route"));
   itemmodel_->setHeaderData(1, Qt::Horizontal, tr("Proportion"));
   itemmodel_->setHeaderData(2, Qt::Horizontal, tr("Travel time"));
   itemmodel_->setHeaderData(3, Qt::Horizontal, tr("Distance"));

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
	bool findroutes=false;
	// start to search the route with 
	// the inputed OD pair
	if(check_)
	{
		infolabel->setText(QString("Search the route with the OD..."));
		orgId_=(origcomb->currentText()).toInt();	
		destId_=(destcomb->currentText()).toInt();

		vector <ODpair*>& odpairs=mezzonet_->get_odpairs();
		vector<ODpair*>::const_iterator odlocation;
		odlocation=find_if(odpairs.begin(), odpairs.end(), 
							assist::compareod(odval(orgId_,destId_)));
		if(odlocation==odpairs.end()) 
		{
			QMessageBox::warning(this, "Errors", "no route is found!", 
			QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
			infolabel->setText(QString("Please choose an OD pair"));
		}
		else
		{
			findroutes=true;
			vector<Route*>& allroutes=(*odlocation)->get_allroutes();
			
			// add the information as a row in the table
			for(unsigned i=0; i<allroutes.size(); i++)
			{
				QList<QStandardItem*> *onerowptr= new QList<QStandardItem*>();
			
				// add the route ID item
				QString routeid=QString::number(allroutes[i]->get_id());
				QStandardItem* cell1=new QStandardItem(routeid);
				onerowptr->append(cell1);

				// add the item of travel time at the current time
				double currenttime=mezzonet_->get_currenttime();
				QString routecost=QString::number(allroutes[i]->cost(currenttime));
				QStandardItem* cell2=new QStandardItem(routecost);
				onerowptr->append(cell2);
			
				// add the item of route utility at the current time
				QString routeutil=QString::number(allroutes[i]->utility(currenttime));
				QStandardItem* cell3=new QStandardItem(routeutil);
				onerowptr->append(cell3);

				// add the item of distances
				QStandardItem* cell4=new QStandardItem(QString("100"));
				onerowptr->append(cell4);

				// add the row to the tableview model
				itemmodel_->appendRow(*onerowptr);
			}
			
			// set the information label
			infolabel->setText(QString("Routes corresponding to the OD are listed!"));

		} // end if routes are founded
	}
	else
	{
		infolabel->setText(QString("Please choose an OD pair"));
	}
	ODTableView->setVisible(check_&&findroutes);
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
	for(unsigned i=0; i<origs.size(); i++){
		origcomb->addItem(QString::number(origs[i]->get_id()));
	}
	for(unsigned i=0; i<dests.size(); i++){
		destcomb->addItem(QString::number(dests[i]->get_id()));
	}
}

