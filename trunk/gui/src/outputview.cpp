#include <QtGui>
#include "outputview.h"

OutputView::OutputView(QWidget* )
{
	theNetwork=NULL;
	setupUi(this);
	// populate the ThicknessMOE and ColourMOE comboboxes
	ThicknessMOE->insertItem(0,"None");
	ThicknessMOE->insertItem(1,"Outflow");
	ThicknessMOE->insertItem(2,"Inflow");
	ThicknessMOE->insertItem(3,"Speed");
	ThicknessMOE->insertItem(4,"Density");
	ThicknessMOE->insertItem(5,"Queue");
	ColourMOE->insertItem(0,"None");
	ColourMOE->insertItem(1,"Outflow");
	ColourMOE->insertItem(2,"Inflow");
	ColourMOE->insertItem(3,"Speed");
	ColourMOE->insertItem(4,"Density");
	ColourMOE->insertItem(5,"Queue");
}

void OutputView::show()
{
	QWidget::show();
	if (theNetwork)
	{
		ThicknessMOE->setCurrentIndex(1);
		ColourMOE->setCurrentIndex(3); // speed
		theParameters->inverse_colour_scale = true; // so high speeds are green and low speeds red
		inverseColourScale->setChecked(theParameters->inverse_colour_scale);
		maxThickness->setValue(theParameters->thickness_width);
	}
}

void OutputView::on_ThicknessMOE_currentIndexChanged(int index)
{
	if (theNetwork != NULL)
	{
		unsigned int val;
		if ((index < 0) || (index > 5))
			val = 0;
		else
			val = index;
		theNetwork->set_output_moe_thickness(val);

		// set boundaries
		thickness_min->setText(QString::number(theParameters->min_thickness_value,'f',1));
		thickness_max->setText(QString::number(theParameters->max_thickness_value,'f',1));

		// draw legend
		draw_thickness_legend();
		set_thickness_unit(index);
	}
}

void OutputView::on_ColourMOE_currentIndexChanged(int index)
{
	if (theNetwork!=NULL)
	{
		unsigned int val;
		if ((index < 0) || (index > 5))
			val = 0;
		else
			val = index;
		// set MOE
		if (val == 3) // for speed 
		{
			theParameters->inverse_colour_scale = true;
			inverseColourScale->setChecked(true);
		}
		else
		{
			theParameters->inverse_colour_scale = false;
			inverseColourScale->setChecked(false);			
		}
		theNetwork->set_output_moe_colour(val);
		// set boundaries
		colour_min->setText(QString::number(theParameters->min_colour_value,'f',1));
		colour_max->setText(QString::number(theParameters->max_colour_value,'f',1));
		// draw legend
		draw_colour_legend();
		set_colour_unit(index);
		
	}
}

void OutputView::draw_colour_legend()
{

	QPixmap col (colour_legend->size());
	QPainter painter (&col);
	QLinearGradient linearGrad(QPointF(0, 0), QPointF(160, 0));
	if (theParameters->inverse_colour_scale)
	{
		 linearGrad.setColorAt(1, Qt::green);
		 linearGrad.setColorAt(0.5,Qt::yellow);
		 linearGrad.setColorAt(0, Qt::red);
	}
	else
	{
		 linearGrad.setColorAt(0, Qt::green);
		 linearGrad.setColorAt(0.5,Qt::yellow);
		 linearGrad.setColorAt(1, Qt::red);
	}
	QBrush brush(linearGrad);
	painter.fillRect(0,0,160,20,brush);
	painter.end();
	colour_legend->setPixmap(col);
	this->repaint();
}

void OutputView::draw_thickness_legend()
{
	thickness_legend->resize(thickness_legend->width(),theParameters->thickness_width);
	int steps = 5;
	QSize size = thickness_legend->size();
	QPixmap thick (size);
	thick.fill();
	QPainter painter (&thick);
	QPen pen(Qt::red);
	
	double w=size.width()/steps;
	double x1(0.0) ,x2 (0.0);
	double y = theParameters->thickness_width / 2;
	for (int i=1;i<steps+1;i++)
	{
		x2=x1+w;
		double h=(theParameters->thickness_width*i)/steps;		
		pen.setWidthF(h); // height of line
		painter.setPen(pen);
		painter.drawLine(x1,y,x2,y);		
		x1=x2;
	}	
	painter.end();
	thickness_legend->setPixmap(thick);
	this->repaint();
}

void OutputView::set_thickness_unit(int val)
{
	QString text;
	switch (val)
	{ 
	case 0:
		text="";
		break;
	case 1:
		text="veh (count)";
		break;
	case 2:
		text="veh (count)";
		break;
	case 3:
		text="m/s";
		break;
	case 4:
		text="veh/km/lane";
		break;
	case 5:
		text="veh";
		break;
	}
	thickness_unit->setText(text);
}
void OutputView::set_colour_unit(int val)
{
	QString text;
	switch (val)
	{ 
	case 0:
		text="";
		break;
	case 1:
		text="veh/h/lane";
		break;
	case 2:
		text="veh/h/lane";
		break;
	case 3:
		text="m/s";
		break;
	case 4:
		text="veh/km/lane";
		break;
	case 5:
		text="veh";
		break;
	}
	colour_unit->setText(text);
}

void OutputView::on_inverseColourScale_toggled()
{
	theParameters->inverse_colour_scale=inverseColourScale->isChecked();
	draw_colour_legend();
}

void OutputView::on_maxThickness_valueChanged(int i)
{
	theParameters->thickness_width = i;
	thickness_legend->resize(thickness_legend->width(),theParameters->thickness_width);
}

void OutputView::on_showLinkNames_toggled(bool checked)
{
	theParameters->show_link_names=checked;
}

void OutputView::on_showLinkIds_toggled (bool checked)
{
	theParameters->show_link_ids=checked;
}