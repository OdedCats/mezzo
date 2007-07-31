#include "parametersdialog_qt4.h"

ParametersDialog::ParametersDialog(QWidget *parent): QDialog(parent)
{
	setupUi(this);
}

// Auto connect slots

void ParametersDialog::on_LinkIds_toggled(bool value)
{
	theParameters->draw_link_ids=value;
}

void ParametersDialog::on_ShowBgImage_toggled( bool value)
{
    theParameters->show_background = value;
}

void ParametersDialog::on_LinkColorButton_clicked()
{
	theParameters->linkcolor=QColorDialog::getColor();
}

void ParametersDialog::on_NodeColorButton_clicked()
{
    theParameters->nodecolor=QColorDialog::getColor();
}

void ParametersDialog::on_QueueColorButton_clicked()
{
     theParameters->queuecolor=QColorDialog::getColor();
}

void ParametersDialog::on_BgColorButton_clicked()
{
     theParameters->backgroundcolor=QColorDialog::getColor();
}

void ParametersDialog::on_LinkThickness_valueChanged( int value)
{
  theParameters->link_thickness = value;
}

void ParametersDialog::on_NodeThickness_valueChanged( int value)
{
    theParameters->node_thickness = value;
}
void ParametersDialog::on_QueueThickness_valueChanged( int value )
{
  theParameters->queue_thickness = value;
}

void ParametersDialog::on_NodeRadius_valueChanged( int  value)
{
  theParameters->node_radius = value;
}