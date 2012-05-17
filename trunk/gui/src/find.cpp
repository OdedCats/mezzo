#include <QtGui>
#include "find.h"

FindDialog::FindDialog(QWidget* parent):parent_(parent)
{
	theNetwork=NULL;
	setupUi(this);
	selected_link=NULL;
	selected_node=NULL;
}

void FindDialog::set_network(Network* net_) 
{
	theNetwork=net_;
	theParameters = theNetwork->get_parameters();
}

void FindDialog::show()
{
	findId->clear();
	QWidget::show();
	unselect();
}
void FindDialog::on_findButton_clicked()
{
	// Find the link/node and set as selected
	unselect();
	if (linkRadio->isChecked()) // find link
	{
		int id = findId->text().toInt();
		map <int, Link*> alllinks=theNetwork->get_links();
		if (!alllinks.count(id)) // link id does not exist
		{
			response->setText("Cannot find that link!");

			findId->clear();
		}
		else // link id exists
		{
			selected_link= alllinks [id];
			selected_link->set_selected(true);
		}
	}
	else // find node
	{
		int id = findId->text().toInt();
		map <int, Node*> allnodes=theNetwork->get_nodes();
		if (!allnodes.count(id)) // link id does not exist
		{
			response->setText("Cannot find that node!");
			findId->clear();
		}
		else // node id exists
		{
			selected_node= allnodes [id];
			selected_node->get_icon()->set_selected(true);
		}

	}

	// redraw
	theNetwork->redraw();
	parent_->repaint();
	qApp->processEvents();
	emit paintRequest();
}

void FindDialog::unselect()
{
	if (selected_link)
	{
		selected_link->set_selected(false);
		selected_link=NULL;
	}
	if (selected_node)
	{
		selected_node->get_icon()->set_selected(false);
		selected_node=NULL;
	}
	response->clear();
}

void FindDialog::on_cancelButton_clicked()
{
	// clean up and return
	unselect();
	theNetwork->redraw();
	parent_->repaint();
	emit paintRequest();
	close();
}

void FindDialog::closeEvent(QCloseEvent *event)
 {
     unselect();
     event->accept();
   
 }
	
