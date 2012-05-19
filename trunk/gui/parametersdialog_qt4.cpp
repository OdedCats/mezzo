#include "parametersdialog_qt4.h"

ParametersDialog::ParametersDialog(QWidget *parent): QDialog(parent)
{
	setupUi(this);
	QObject::connect(zoomfactor, SIGNAL(valueChanged(int)), 
					       this, SIGNAL(activateZoomFactor(int)));
    QObject::connect(panfactor, SIGNAL(valueChanged(int)), 
					      this, SIGNAL(activatePanFactor(int)));
    QObject::connect(simspeed, SIGNAL(valueChanged(int)), 
					     this, SIGNAL(activateSimSpeed(int)));
}

void ParametersDialog::set_parameters (Parameters* params) 
{
	theParameters=params;
	refreshrate->setValue(static_cast<int>(theParameters->gui_update_step*1000.0));

	LinkIds->setChecked(theParameters->draw_link_ids);
	LinkThickness->setValue(theParameters->link_thickness);
	NodeThickness->setValue(theParameters->node_thickness);
	NodeRadius->setValue(theParameters->node_radius);
	QueueThickness->setValue(theParameters->queue_thickness);
	ShowBgImage->setChecked(theParameters->show_background);
}

// Auto connect slots

void ParametersDialog::on_LinkIds_toggled(bool value)
{
	theParameters->draw_link_ids=value;
	
	emit paintRequest();
}

void ParametersDialog::on_ShowBgImage_toggled( bool value)
{
    theParameters->show_background = value;
	emit paintRequest();
}

void ParametersDialog::on_LinkColorButton_clicked()
{
	theParameters->linkcolor=QColorDialog::getColor();
	emit paintRequest();
}

void ParametersDialog::on_NodeColorButton_clicked()
{
    theParameters->nodecolor=QColorDialog::getColor();
	emit paintRequest();
}

void ParametersDialog::on_QueueColorButton_clicked()
{
     theParameters->queuecolor=QColorDialog::getColor();
	 emit paintRequest();
}

void ParametersDialog::on_BgColorButton_clicked()
{
     theParameters->backgroundcolor=QColorDialog::getColor();
	 emit paintRequest();
}

void ParametersDialog::on_LinkThickness_valueChanged( int value)
{
  theParameters->link_thickness = value;
  emit paintRequest();
}

void ParametersDialog::on_NodeThickness_valueChanged( int value)
{
    theParameters->node_thickness = value;
	emit paintRequest();
}
void ParametersDialog::on_QueueThickness_valueChanged( int value )
{
  theParameters->queue_thickness = value;
  emit paintRequest();
}

void ParametersDialog::on_NodeRadius_valueChanged( int  value)
{
  theParameters->node_radius = value;
 // emit calc_shift();
  emit paintRequest();
}