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
 * this is the definition of the node dialog
 *
 * Xiaoliang Ma
 * Last modification: 2007-11-20
 *
 */

#ifndef  NODEDLG_H
#define  NODEDLG_H

#include <QDialog>
// use the generated ui form header  
#include "../ui_nodedlg.h"
// node definition
#include "../../mezzo_lib/src/node.h"

class NodeDlg : public QDialog, public Ui::NodeDlg
{
	Q_OBJECT
public:
	NodeDlg(Node* curnode, QWidget* parent=0);
	~NodeDlg(){}
	
private:		 
	Node* curnode_;
};

#endif

