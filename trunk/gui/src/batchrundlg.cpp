#include <QtGui>
#include "batchrundlg.h"

BatchrunDlg::BatchrunDlg( QWidget* parent )
{
	parent_=parent;
	theNetwork=NULL;
	setupUi(this);
	//progress_gb->setEnabled(false);
	cur_iter->setNum(0);
	relgap_ltt->setNum(0);
	relgap_rf->setNum(0);
	totalPb->setValue(0);
	currIterPb->setValue(0);

	max_iter = 1000; // default value
	max_relgap=1.0;
	
//TODO: there is a linktime_alpha in the parameters file and a time_alpha in the Master file! 
}

void BatchrunDlg::setNetwork(Network* net)
{
	theNetwork=net;
	alpha->setText( QString::number(theParameters->linktime_alpha,'f',3));
}

void BatchrunDlg::on_max_iterations_val_valueChanged(int i)
{	
	total_iter->setNum(i);
}
void BatchrunDlg::on_stopButton_clicked()
{
	stop_pressed = true;
}

void BatchrunDlg::show()
{		
	alpha->setText( QString::number(theParameters->linktime_alpha,'f',3));
	cur_iter->setNum(0);
	relgap_ltt->setNum(0);
	totalPb->setValue(0);
	currIterPb->setValue(0);
	stop_pressed = false;
	relgap_ltt->setEnabled(false);
	relgap_rf->setEnabled(false);
	max_iterations_val->setValue(theParameters->max_iter);
	max_relgap_val->setText(QString("%1").arg(theParameters->rel_gap_threshold));
	QWidget::show();
}
void BatchrunDlg::on_saveButton_clicked()
{
	if (overwriteHisttimes->isChecked())
		theParameters->overwrite_histtimes=true;
	if (!theNetwork->writeall())
		QMessageBox::information(this, "Error Saving", "Error saving results. See debug_log.txt for information");;
		// QMessageBox::information(this, "Saved", "Results saved.");
}

void BatchrunDlg::on_runButton_clicked()
{
	// check what type of iteration is to be done
	stop_pressed = false;
	if (iterate_traveltimes_rb->isChecked())
	{
		
		if (max_iterations_cb->isChecked())
		{
			max_iter=max_iterations_val->value();
		}

		if (max_relgap_cb->isChecked())
		{
			max_relgap = max_relgap_val->text().toDouble();
			if (relgap_link_tt->isChecked())
			{
				relgap_ltt->setEnabled(true);
			}
			if (relgap_route_flows->isChecked())
			{
				relgap_rf->setEnabled(true);
			}
		}
		//progress_gb->setEnabled(true);
		run_iterations();
	}	
}

const bool BatchrunDlg::checkConvergence(const int i, const double relgap_ltt_, const double relgap_rf_)
{
	// if more than max iterations, return true
	if (i >= max_iter)
		return true;
	
	bool rf=true;
	bool ltt=true;
	if (max_relgap_cb->isChecked())
	{
		// RGAP link traveltimes 
		if ( (relgap_link_tt->isChecked()) )
		{
			if ((relgap_ltt_ <= max_relgap)) 
				ltt= true;
			else 
				ltt= false;
		}
		// RGAP route flows 
		if ( (relgap_route_flows->isChecked()) )
		{
			if ((relgap_rf_ <= max_relgap)) 
				rf= true;
			else 
				rf= false;
		}
		return (rf && ltt); // for now both checked critera need to be true.
	}
	return false;
}
void BatchrunDlg::run_iterations()
{
	
	theParameters->linktime_alpha=(alpha->text().toDouble());
	int i=1;
	total_iter->setNum(max_iter);
	double runtime=theNetwork->get_runtime();
	double curtime=0.0;
	double relgap_ltt_=1.0;
	double relgap_rf_=1.0;
	cur_iter->setNum(i);
	update();
	theNetwork->open_convergence_file(theNetwork->get_workingdir() + "convergence.dat");
// iterations
	for (i; !checkConvergence(i,relgap_ltt_,relgap_rf_) && !stop_pressed;i++)
	{
		// update display widgets with correct values
		cur_iter->setNum(i);
		relgap_ltt->setText (QString::number(relgap_ltt_,'f',5));
		relgap_rf->setText(QString::number(relgap_rf_,'f',5));
		int progress = static_cast<int>(100*(i-1)/max_iter);
		totalPb->setValue(progress);
		repaint();

		if (i>1)
		{
			theNetwork->copy_linktimes_out_in();
			theNetwork->reset();
		}
		curtime=0.0;
		while ((curtime < runtime))
		{
			curtime=theNetwork->step(1.0);
			int progress = static_cast<int>(100*curtime/runtime);
			currIterPb->setValue(progress);
			update();
			qApp->processEvents();
			repaint();
			theNetwork->redraw();
			emit paintRequest();
		}
		theNetwork->end_of_simulation(runtime);
		relgap_ltt_= theNetwork->calc_rel_gap_linktimes();
		if (i>1)
			relgap_rf_= theNetwork->calc_rel_gap_routeflows();
		// write to convergence file
		theNetwork->write_line_convergence(i,relgap_ltt_,relgap_rf_);	
	}	

	relgap_ltt->setText (QString::number(relgap_ltt_,'f',5));
	relgap_rf->setText(QString::number(relgap_rf_,'f',5));
	currIterPb->setValue(100);
	totalPb->setValue(100);
	repaint();
	theNetwork->close_convergence_file();
	activateAnalyzeOutput();

}

	