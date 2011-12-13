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

#ifndef OUTPUTVIEW_
#define OUTPUTVIEW_

#include <QDialog>
// use the generated ui form header  
#include "../ui_outputview.h"
// network definition
#include "../../mezzo_lib/src/network.h" // also includes the parameters


class OutputView : public QDialog, public Ui::OutputView
{
	Q_OBJECT
public:
	OutputView(QWidget* parent=0);
	~OutputView(){}
	void setNetwork(Network* net) {theNetwork=net; theParameters=theNetwork->get_parameters();}
	void show();

	void setColourRange(double min, double max);
	void setThicknessRange(double min, double max);
	void draw_colour_legend();
	void draw_thickness_legend();
	void set_thickness_unit(int val);
	void set_colour_unit(int val);
	
	
	//SLOTS
	private slots:
		void on_ThicknessMOE_currentIndexChanged(int index);
		void on_ColourMOE_currentIndexChanged(int index);
		void on_inverseColourScale_toggled();
		void on_maxThickness_valueChanged(int i);
		void on_cutoff_valueChanged(int i);
		void on_textsize_valueChanged(int i);
		void on_showLinkNames_toggled(bool checked);
		void on_showLinkIds_toggled (bool checked);
		void on_showDataValues_toggled (bool checked);
private:
	QWidget* parent_;
	Network* theNetwork;
	Parameters* theParameters;  //!< The parameters object, which contains the global parameters for the simulation and GUI	
};




#endif // OUTPUTVIEW_
