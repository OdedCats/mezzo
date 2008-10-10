#include <QtGui>
#include "batchrundlg.h"

BatchrunDlg::BatchrunDlg( QWidget* )
{
	theNetwork=NULL;
	setupUi(this);
	//progress_gb->setEnabled(false);
	cur_iter->setNum(0);
	rmsn_ltt->setNum(0);
	rmsn_odtt->setNum(0);
	totalPb->setValue(0);
	currIterPb->setValue(0);

	max_iter = 1000; // default value
	max_rmsn=1.0;
	
//TODO: there is a linktime_alpha in the parameters file and a time_alpha in the Master file! where should it be?
}

void BatchrunDlg::setNetwork(Network* net)
{
	theNetwork=net;
	alpha->setText( QString::number(theNetwork->get_parameters()->linktime_alpha,'f',3));
}

void BatchrunDlg::on_max_iterations_val_valueChanged(int i)
{	
	total_iter->setNum(i);
}

void BatchrunDlg::show()
{		
	alpha->setText( QString::number(theNetwork->get_time_alpha(),'f',3));
	cur_iter->setNum(0);
	rmsn_ltt->setNum(0);
	totalPb->setValue(0);
	currIterPb->setValue(0);
	QWidget::show();
}

void BatchrunDlg::on_runButton_clicked()
{
	// check what type of iteration is to be done

	if (iterate_traveltimes_rb->isChecked())
	{
		
		if (max_iterations_cb->isChecked())
			max_iter=max_iterations_val->value();

		if (max_rmsn_cb->isChecked())
			max_rmsn = max_rmsn_val->text().toDouble();
		//progress_gb->setEnabled(true);
		run_iterations();
	}	
}

const bool BatchrunDlg::checkConvergence(const int i, const double rmsn_ltt_, const double rmsn_odtt_)
{
	if (i >= max_iter)
		return true;
	if (max_rmsn_cb->isChecked())
	{
		if ( (rmsn_link_tt->isChecked()) && (rmsn_ltt_ <= max_rmsn))
				return true;
		if ( (rmsn_od_tt->isChecked()) && (rmsn_odtt_ <= max_rmsn))
				return true;	
	}
	
	return false;
}
void BatchrunDlg::run_iterations()
{
	//theNetwork->get_parameters()->linktime_alpha = alpha->text().toDouble();
	theNetwork->set_time_alpha(alpha->text().toDouble());
	int i=1;
	total_iter->setNum(max_iter);
	double runtime=theNetwork->get_runtime();
	double curtime=0.0;
	double rmsn_ltt_=1.0;
	double rmsn_odtt_=1.0;
	cur_iter->setNum(i);
	update();

// iterations
	for (i; !checkConvergence(i,rmsn_ltt_,rmsn_odtt_);i++)
	{
		// update display widgets with correct values
		cur_iter->setNum(i);
		rmsn_ltt->setText (QString::number(rmsn_ltt_,'f',5));
		rmsn_odtt->setText(QString::number(rmsn_odtt_,'f',5));
		int progress = static_cast<int>(100*(i-1)/max_iter);
		totalPb->setValue(progress);
		repaint();

		if (i>1)
		{
			theNetwork->copy_linktimes_out_in();
			theNetwork->reset();
		}
		curtime=0.0;
		while (curtime < runtime)
		{
			curtime=theNetwork->step(1.0);
			int progress = static_cast<int>(100*curtime/runtime);
			currIterPb->setValue(progress);
			update();
			repaint();
		}
		theNetwork->end_of_simulation(runtime);
		rmsn_ltt_= theNetwork->calc_rmsn_input_output_linktimes();
		if (i>1)
			rmsn_odtt_=theNetwork->calc_rmsn_input_output_odtimes();
		// calculate the OD travel times rmsn as well
	}	
//	rmsn_ltt_= theNetwork->calc_rmsn_input_output_linktimes();
//	rmsn_odtt_=theNetwork->calc_rmsn_input_output_odtimes();
	rmsn_ltt->setText (QString::number(rmsn_ltt_,'f',5));
	rmsn_odtt->setText(QString::number(rmsn_odtt_,'f',5));
	currIterPb->setValue(100);
	totalPb->setValue(100);
	repaint();
}

	