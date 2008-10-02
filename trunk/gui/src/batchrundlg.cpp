#include <QtGui>
#include "batchrundlg.h"

BatchrunDlg::BatchrunDlg( QWidget* parent)
{
	theNetwork=NULL;
	setupUi(this);
	//progress_gb->setEnabled(false);
	cur_iter->setNum(0);
	rmsn->setNum(0);
	totalPb->setValue(0);
	currIterPb->setValue(0);
	//alpha->setText( QString::number(theNetwork->get_parameters()->linktime_alpha,'f',3));
	alpha->setText( QString::number(theNetwork->get_parameters()->linktime_alpha,'f',3));
//TODO: there is a linktime_alpha in the parameters file and a time_alpha in the Master file! where should it be?
}

void BatchrunDlg::setNetwork(Network* net)
{
	theNetwork=net;
}

void BatchrunDlg::on_max_iterations_val_valueChanged(int i)
{	
	total_iter->setNum(i);
}



void BatchrunDlg::on_runButton_clicked()
{
	// check what type of iteration is to be done

	if (iterate_traveltimes_rb->isChecked())
	{
		int max_iter=max_iterations_val->value();
		double max_rmsn=0.1;
		//progress_gb->setEnabled(true);
		run_iterations(max_iter,max_rmsn);
	}

	
}

void BatchrunDlg::run_iterations(int nr_iterations, double max_rmsn)
{
	//theNetwork->get_parameters()->linktime_alpha = alpha->text().toDouble();
	theNetwork->set_time_alpha(alpha->text().toDouble());
	int i=1;
	total_iter->setNum(nr_iterations);
	double runtime=theNetwork->get_runtime();
	double curtime=0.0;
	double rmsn_=1.0;
	cur_iter->setNum(i);
	update();
// First pass, from read input travel times
	while (curtime < runtime)
	{
		curtime=theNetwork->step(1.0);
		int progress = static_cast<int>(100*curtime/runtime);
		currIterPb->setValue(progress);
		update();
	}
	theNetwork->end_of_simulation(runtime);
	rmsn_ = theNetwork->calc_rmsn_input_output_linktimes();
	rmsn->setText (QString::number(rmsn_,'f',4));
	if (nr_iterations ==1)
		return;
	i++;
// Subsequent iterations
	for (i; i<=nr_iterations;i++)
	{
		cur_iter->setNum(i);
		rmsn_ = theNetwork->calc_rmsn_input_output_linktimes();
		rmsn->setText (QString::number(rmsn_,'f',4));
		int progress = static_cast<int>(100*(i-1)/nr_iterations);
		totalPb->setValue(progress);
		repaint();
		theNetwork->copy_linktimes_out_in();
		theNetwork->reset();
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
	}	
	rmsn_ = theNetwork->calc_rmsn_input_output_linktimes();
	rmsn->setText (QString::number(rmsn_,'f',4));
	currIterPb->setValue(100);
	totalPb->setValue(100);
	repaint();
}

	