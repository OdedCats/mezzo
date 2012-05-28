#include <QtGui>
#include "find.h"

FindDialog::FindDialog(QWidget* parent):parent_(parent)
{
	theNetwork=NULL;
	setupUi(this);
	selected_links.clear();
	selected_nodes.clear();
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
	// unselect();
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
			alllinks [id]->set_selected(true);
			selected_links.push_back(alllinks [id]);
		}
	}
	else // find node
	{
		int id = findId->text().toInt();
		map <int, Node*> allnodes=theNetwork->get_nodes();
		if (!allnodes.count(id)) // link id does not exist
		{
			response->setText("Cannot find that node!");
			
		}
		else // node id exists
		{
			allnodes [id]->get_icon()->set_selected(true);
			selected_nodes.push_back(allnodes [id]);
		}

	}

	// redraw
	findId->clear();
	emit paintRequest();
}

void FindDialog::on_clearButton_clicked()
{
	unselect();
	findId->clear();
	emit paintRequest();
}

void FindDialog::unselect()
{
	if (selected_links.size()>0)
	{
		for (list<Link*>::iterator l_iter=selected_links.begin(); l_iter != selected_links.end(); ++l_iter)
		{
			(*l_iter)->set_selected(false);
		}
		selected_links.clear();
	}
	if (selected_nodes.size()>0)
	{
		for (list<Node*>::iterator n_iter=selected_nodes.begin(); n_iter != selected_nodes.end(); ++n_iter)
		{
			(*n_iter)->get_icon()->set_selected(false);
		}
		selected_nodes.clear();		
	}
	response->clear();
}

void FindDialog::on_cancelButton_clicked()
{
	// clean up and return
	unselect();
	
	emit paintRequest();
	close();
}

void FindDialog::closeEvent(QCloseEvent *event)
 {
     unselect();
     event->accept();
   
 }
	
