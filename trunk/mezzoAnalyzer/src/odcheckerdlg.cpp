#include <QtGui>
#include <algorithm>
#include "odcheckerdlg.h"
#include "assist.h"


/**
 *  constructor of OD check dialog
 */
ODCheckerDlg::ODCheckerDlg(QWidget* parent):QDialog(parent)
{
   setupUi(this);
   mezzonet_=0;
   odsel_=0;
   orgId_=-1;
   destId_=-1;
   networkset_=false;
   allroutesdrawn_=false;
   paintrouteseq_=new vector<std::pair<int,QString>>(0);	

   // create the model for the tableview
   itemmodel_=new QStandardItemModel(0,5);
   itemmodel_->setHeaderData(0, Qt::Horizontal, tr("View"));
   itemmodel_->setHeaderData(1, Qt::Horizontal, tr("Route"));
   itemmodel_->setHeaderData(2, Qt::Horizontal, tr("Proportion (%)"));
   itemmodel_->setHeaderData(3, Qt::Horizontal, tr("Travel time (sec)"));
   itemmodel_->setHeaderData(4, Qt::Horizontal, tr("Distance (m)"));
   
   // set column width
   QHeaderView *headerview=ODTableView->horizontalHeader();
   headerview->resizeSection(0,12);
   headerview->resizeSection(1,12);
   headerview->resizeSection(2,20);
   headerview->resizeSection(3,20);
   headerview->resizeSection(4,20);

   // create new tableview delegate
   itemdelegate_=new ODTableViewDelegate(0, this);
   ODTableView->setItemDelegate(itemdelegate_);
   ODTableView->setModel(itemmodel_);
   ODTableView->installEventFilter(this);

   // connect check button with "checkOD" function
   QObject::connect(CheckButton, SIGNAL(toggled(bool)), 
		   	        this, SLOT(checkOD(bool)));
   // connect combbox with two functions
   QObject::connect(origcomb, SIGNAL(activated(const QString&)),
					this, SLOT(loadDestCombwithO(const QString&)));
   QObject::connect(destcomb, SIGNAL(activated(const QString&)),
	                this, SLOT(loadOrigCombwithD(const QString&)));
   // connect the activated color signal with draw route slots
   QObject::connect(itemdelegate_, SIGNAL(activateAColor(const QString&, const int&)),
					this, SLOT(drawRoute(const QString&, const int&)) );
   // using single click to activate the item editor
   QObject::connect(ODTableView, SIGNAL(clicked(const QModelIndex &)), ODTableView,
								SLOT(edit(const QModelIndex &)));

   QItemSelectionModel *selmodel = ODTableView->selectionModel();
   QObject::connect(selmodel, SIGNAL(selectionChanged(const QItemSelection &, 
		                                              const QItemSelection & )), 
   		   	            this, SLOT(selectionHandle(const QItemSelection &, 
   		   	        		                       const QItemSelection &)) );

   // lay out the size of the dialog
   layout()->setSizeConstraint(QLayout::SetFixedSize);
   //layout()->setSizeConstraint(QLayout::SetDefaultConstraint);  
    ODTableView->hide();
}


/**
 * destructor of OD check dialog 
 */
ODCheckerDlg::~ODCheckerDlg()
{
	// only release the properties of this dialog
	delete paintrouteseq_;
	delete itemmodel_;
}

/**
* event filter for the OD check dlg
*/
bool ODCheckerDlg::eventFilter(QObject *obj, QEvent *evt)
{
	// handling event for table view
	if (obj == ODTableView) {
		if (evt->type() == QEvent::KeyPress) {
		  QKeyEvent* kevt = static_cast<QKeyEvent*>(evt);
		  // if Control + A is pressed
		  if ((kevt->key ()==Qt::Key_A)&&(kevt->modifiers()&Qt::ControlModifier)){
			// clear routes drawn previously
			unselectRoutes();
			drawAllRoutes();
			updateGraph();
		  }
		  return true;
		} 
		else {
            return QDialog::eventFilter(obj, evt);
        }
    } 
	else {
         // pass the event on to the parent class
         return QDialog::eventFilter(obj, evt);
    }
}

// implement the virtual public slot "reject" function
void ODCheckerDlg::reject()
{	
	//trigger the checkbutton
	CheckButton->setChecked(false);
	loadInitOD();
	clearTableView();
	// call the virtual function of the base class
	QDialog::reject();
}

/**
 * Set the mezzo network and so the OD range to the comboxes
 */
void ODCheckerDlg::setNetwork(Network* mezzonet)
{
	networkset_=true;
	mezzonet_=mezzonet;
	loadInitOD();
}

/**
* A slot function when "origin" combobox is activated
* when an item in "origin" combobox is selected,
* load the destination list with a given origin ID
*/
void ODCheckerDlg::loadDestCombwithO(const QString& curtext)
{
	// clear table contents previously shown
	clearTableView();

	if (curtext=="None"){
		loadInitOD();
	}
	else{
		QString& desttext=destcomb->currentText();
		if(desttext=="None"){

			//clear the destination combobox
			destcomb->clear();
			destcomb->addItem("None");
			
			//add dest ID items with the given origin ID 
			orgId_=(origcomb->currentText()).toInt();
			vector <ODpair*>& odpairs=mezzonet_->get_odpairs();
			for(unsigned i=0;i<odpairs.size();i++){
				if(odpairs[i]->get_origin()->get_id()==this->orgId_){
					destcomb->addItem(QString::number(
						odpairs[i]->get_destination()->get_id()));
				}
			}
		}
	}
}

/**
* A slot function when "destination" combobox is activated
* when an item in "destination" combobox is selected,
* load the origin list with a given destination ID
*/
void ODCheckerDlg::loadOrigCombwithD(const QString& curtext)
{
	// clear table contents previously shown
	clearTableView();

	if (curtext=="None"){
		loadInitOD();
	}
	else{
		QString& origtext=origcomb->currentText();
		if(origtext=="None"){

			//clear the destination combobox
			origcomb->clear();
			origcomb->addItem("None");
			
			//add origin ID items with the given dest ID 
			destId_=(destcomb->currentText()).toInt();
			vector <ODpair*>& odpairs=mezzonet_->get_odpairs();
			for(unsigned i=0;i<odpairs.size();i++){
				if(odpairs[i]->get_destination()->get_id()==this->destId_){
					origcomb->addItem(QString::number(
						odpairs[i]->get_origin()->get_id()));
				}
			}
		}
	}
}

/**
 * A slot function connected with "Check" button
 * to check the traffic on routes with the pair 
 * of OD selected
 * */
void ODCheckerDlg::checkOD(bool check_)
{
	bool findroutes=false;
	
	// if either the origin and destination is not set
	// then not checking any OD
	if ((origcomb->currentText()=="None")||(destcomb->currentText()=="None")){
		check_=false;
	}

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
			odsel_=*odlocation;
			vector<Route*>& allroutes=odsel_->get_allroutes();
			
			// compute the summation of utility across all routes
			double utilsum=0;
			double currenttime=mezzonet_->get_currenttime();
			for(unsigned i=0; i<allroutes.size();i++)
				utilsum+=allroutes[i]->utility(currenttime);

			// add the information as a row in the table
			for(unsigned i=0; i<allroutes.size(); i++)
			{
				QList<QStandardItem*> *onerowptr= new QList<QStandardItem*>();
			
				// add the item of view
				QStandardItem* cell1=new QStandardItem(QString("none"));
				onerowptr->append(cell1);

				// add the route ID item
				QString routeid=QString::number(allroutes[i]->get_id());
				QStandardItem* cell2=new QStandardItem(routeid);
				cell2->setEditable(false);
				onerowptr->append(cell2);

				// add the item of utility proportion at the current time
				double prop=allroutes[i]->utility(currenttime)/utilsum;
				QStandardItem* cell3=new QStandardItem(QString::number(prop));
				cell3->setEditable(false);
				onerowptr->append(cell3);
			
				// add the item of travel time at the current time
				QString routecost=QString::number(allroutes[i]->cost(currenttime));
				QStandardItem* cell4=new QStandardItem(routecost);
				cell4->setEditable(false);
				onerowptr->append(cell4);

				// add the item of distances
				int dist=allroutes[i]->computeRouteLength();
				QStandardItem* cell5=new QStandardItem(QString::number(dist));
				cell5->setEditable(false);
				onerowptr->append(cell5);

				// add the row to the tableview model
				itemmodel_->appendRow(*onerowptr);
			}
			
			// set the information label
			infolabel->setText(QString("Routes corresponding to the OD are listed!"));

		} // endif - routes are founded or not
	}
	else // check_==false
	{
		clearTableView();
		loadInitOD();
		infolabel->setText(QString("Please choose an OD pair"));
	}

	// OD Tableview will be shown only if check_ and routes are found
	ODTableView->setVisible(check_&&findroutes);
}

/**
 * handle selection behavior in the table view
 */
void ODCheckerDlg::selectionHandle(	const QItemSelection& sel, 
								    const QItemSelection& unsel)
{
	// initialize a row and column counter for the selection area
	const int rowCnt=itemmodel_->rowCount();
	const int colCnt=itemmodel_->columnCount();
	vector<int> rowcounterlist;
	for(int i=0; i<rowCnt; i++){
		rowcounterlist.push_back(0);
	}

	// there is a bug for using ItemSelection of selected items
	// so I use a more complicated way to determine the case
	QItemSelectionModel *selmodel = ODTableView->selectionModel();
	const QItemSelection selitems= selmodel->selection();
	for (int rowi=0; rowi<rowCnt; rowi++){
		for (int colj=0; colj<colCnt; colj++){
			QModelIndex ind=itemmodel_->index(rowi, colj, QModelIndex());
			if (selitems.contains(ind)){
				(rowcounterlist[rowi])++;
			}
		}
	}

	// determine the truth of selecting all
	bool allselected=true;
	for(unsigned i=0; i<rowcounterlist.size(); i++){
		//QMessageBox::warning(this, "Notice", QString::number(rowcounterlist[i]), 
		//	QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
		if(rowcounterlist[i]!=colCnt){
			allselected=false;
			break;
		}
	}

	// if all selected draw all routes 
	if (allselected){
		if (paintrouteseq_->size()>0)
			unselectRoutes();	
		drawAllRoutes();
		updateGraph();
	}
	else {
		if (allroutesdrawn_){
			unselectRoutes();
			updateGraph();
		}
	}
}

/**
 * draw the indexed route with a color 
 */
void ODCheckerDlg::drawRoute(const QString& colortext, const int& index) 
{
	if (odsel_){

		// get all routes from the od pair
		vector<Route*>& allroutes=odsel_->get_allroutes();
		
		//handling the drawing-route request
		QColor routecolor;
		if(colortext=="none"||colortext=="None"){
			
			// remove the current index from the paint list
			vector<std::pair<int,QString>>::iterator iter;
			for(iter=paintrouteseq_->begin();iter!=paintrouteseq_->end();iter++){
				if((*iter).first==index){
					paintrouteseq_->erase(iter);
					allroutes[index]->set_selected(false);
					break;
				}
			}
			//redraw the left painted routes in the stored sequence
			for(iter=paintrouteseq_->begin();iter!=paintrouteseq_->end();iter++){
				routecolor=txt2Color((*iter).second);
				allroutes[(*iter).first]->set_selected_color(routecolor);
				allroutes[(*iter).first]->set_selected(true);
			}
		}
		else{ // if a color is selected
			// first check if the index is already existed 
			// in the drawing sequence 
			bool routedrawn=false;
			vector<std::pair<int,QString>>::iterator iter;
			for(iter=paintrouteseq_->begin();iter!=paintrouteseq_->end();iter++){
				if((*iter).first==index){
					routedrawn=true;
				}
			}
			if (!routedrawn){
				QString tempstr(colortext);
				paintrouteseq_->push_back(std::pair<int,QString>(index,tempstr));
			}
			routecolor=txt2Color(colortext);
			allroutes[index]->set_selected(true);
			allroutes[index]->set_selected_color(routecolor);
		}
		mezzonet_->redraw();
		emit paintRequest();
	}

}

/***
 * drawing all routes between an odpair
 **/
void ODCheckerDlg::drawAllRoutes()
{	
	vector<Route*>& allroutes=odsel_->get_allroutes();
	QStringList rutcolortexts; 
    rutcolortexts<<"black"<<"blue"<<"green"<<"red"<<"magenta";
	for(unsigned i=0;i<allroutes.size();i++){
		QString tempstr=rutcolortexts[i%5];
		allroutes[i]->set_selected_color(txt2Color(tempstr)); 
		allroutes[i]->set_selected(true);
		paintrouteseq_->push_back(std::pair<int,QString>(i,tempstr));
		QModelIndex ind=itemmodel_->index(i, 0, QModelIndex());
		itemmodel_->setData(ind, QVariant(tempstr));
	}
	allroutesdrawn_=true;
}

/**
 * deselect all the routes that are selected in the sequence
 */
void ODCheckerDlg::unselectRoutes()
{
	int tempN=paintrouteseq_->size();
	if(tempN>0){
		vector<Route*>& allroutes=odsel_->get_allroutes();
		for( unsigned i=0; i<paintrouteseq_->size(); i++){
			QModelIndex ind=itemmodel_->index((*paintrouteseq_)[i].first, 0, QModelIndex());
			// synchronize the text of route color in the table
			//itemmodel_->setData(ind, QVariant((*paintrouteseq_)[i].second));
			itemmodel_->setData(ind,QVariant("none"));
			allroutes[(*paintrouteseq_)[i].first]->set_selected(false);
		}
		paintrouteseq_->clear();
	}
	allroutesdrawn_=false;
}

/**
 * load initial OD list in the comboboxes
 */
void ODCheckerDlg::loadInitOD()
{
	origcomb->clear();
	destcomb->clear();
	origcomb->addItem("None");
	destcomb->addItem("None");

	// get the list of origins and destinations
	vector<Origin*>& origs=mezzonet_->get_origins();
	vector<Destination*>& dests=mezzonet_->get_destinations();
	
	// attach string lists to the combo box for OD
	for(unsigned i=0; i<origs.size(); i++)
		origcomb->addItem(QString::number(origs[i]->get_id()));
	for(unsigned i=0; i<dests.size(); i++)
		destcomb->addItem(QString::number(dests[i]->get_id()));
}

/**
* show selected node information 
*/
void ODCheckerDlg::loadSelectOD(vector<Node*>& selnodes)
{
	int selnodes_size=selnodes.size();
	if (selnodes_size<1) 
		return;
	else if(selnodes_size==1)
	{
		
	}
	else
	{
	}
}

/**
 * clear the table view if table is not empty
 */
void ODCheckerDlg::clearTableView()
{
	int rowcount=itemmodel_->rowCount();
	if (rowcount>0){
		ODTableView->setVisible(false);
		itemmodel_->removeRows(0,rowcount);
		// clear routes drawn previously
		unselectRoutes();
		updateGraph();
	}
	CheckButton->setChecked(false);
}

/**
 * text change to color 
 */
QColor ODCheckerDlg::txt2Color(const QString& colortext)
{
	QColor routecolor;
	if(colortext=="black"){
		routecolor=QColor(0,0,0);
	}
	else if (colortext=="blue"){
		routecolor=QColor(0,0,255);
	}
	else if (colortext=="green"){
		routecolor=QColor(0,255,0);
	}
	else if (colortext=="red"){
		routecolor=QColor(255,0,0);
	}
	else if (colortext=="magenta"){
		routecolor=QColor(255,0,255);
	}
	else{
		// to throw an exception
	}
	return routecolor;
}

/**
 * update view of mezzo network 
 **/
void ODCheckerDlg::updateGraph()
{
	mezzonet_->redraw();
	emit paintRequest();
}
