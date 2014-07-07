/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2012  Wilco Burghout

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

/*! positions the background image

*/

#ifndef FINDDIALOG_
#define FINDDIALOG_

#include <QDialog>
// use the generated ui form header  
#include "../ui_find.h"
// network definition
#include "../../mezzo_lib/src/network.h" // also includes the parameters

class FindDialog : public QDialog, public Ui::Find
{
	Q_OBJECT
public:
	FindDialog (QWidget* parent=0);
	~FindDialog() {}
	void set_network(Network* net_) ;
	void show();
	void unselect();
private slots:
	void on_findButton_clicked();
	void on_cancelButton_clicked();
	void on_clearButton_clicked();
	void closeEvent(QCloseEvent *event);
	
signals:
    void paintRequest();

private:
	Network* theNetwork;
	Parameters* theParameters;
	QWidget* parent_;
	list <Link*> selected_links;
	list <Node*> selected_nodes;
};




#endif // POSITIONBACKGROUND_

