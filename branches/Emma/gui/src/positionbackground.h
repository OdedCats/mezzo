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

/*! OutputView determines which MOEs are viewed in the map; 
the user selects the MOEs for link thickness and link colour display.




*/

#ifndef POSITIONBACKGROUND_
#define POSITIONBACKGROUND_

#include <QDialog>
// use the generated ui form header  
#include "../ui_positionbackground.h"
// network definition
#include "../../mezzo_lib/src/network.h" // also includes the parameters

class PositionBackground : public QDialog, public Ui::PositionBackground
{
	Q_OBJECT
public:
	PositionBackground (QWidget* parent=0);
	~PositionBackground() {}
	void set_network(Network* net_) ;
private slots:
	void on_xpos_valueChanged(int val);
	void on_ypos_valueChanged(int val);
	void on_scale_valueChanged(int val);

private:
	Network* theNetwork;
	Parameters* theParameters;
	QWidget* parent_;
};




#endif // POSITIONBACKGROUND_

