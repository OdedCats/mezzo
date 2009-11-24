#include <QtGui>
#include "positionbackground.h"

PositionBackground::PositionBackground(QWidget* parent):parent_(parent)
{
	theNetwork=NULL;
	setupUi(this);

}

void PositionBackground::set_network(Network* net_) 
{
	theNetwork=net_;
	theParameters = theNetwork->get_parameters();
	xpos->setValue(theParameters->background_x);
	ypos->setValue(theParameters->background_y);
	scale->setValue(static_cast<int>(theParameters->background_scale*100));

}


void PositionBackground::on_xpos_valueChanged(int val)
{
	theParameters->background_x = val;
	theNetwork->redraw();
	parent_->repaint();
}
	
void PositionBackground::on_ypos_valueChanged(int val)
{
	theParameters->background_y = val;
	theNetwork->redraw();
	parent_->repaint();

}
	
void PositionBackground::on_scale_valueChanged(int val)
{
	theParameters->background_scale = val/100;
	theNetwork->redraw();
	parent_->repaint();

}