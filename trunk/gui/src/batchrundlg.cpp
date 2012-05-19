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

	//max_iter = 10; // default value
	max_relgap=1.0;
	
//TODO: there is a linktime_alpha in the parameters file and a time_alpha in the Master file! 
}

void BatchrunDlg::setNetwork(Network* net)
{
	theNetwork=net;
	if (theNetwork->get_calc_paths())
		routesearch_rb->setChecked(true);
	alpha->setText( QString::number(theParameters->linktime_alpha,'f',3));
}

void BatchrunDlg::on_max_iterations_val_valueChanged(int i)
{	
	total_iter->setNum(i);
	theParameters->max_iter=i;
}

void BatchrunDlg::on_max_route_iterations_val_valueChanged(int i)
{
	theParameters->max_route_iter=i;
}

void BatchrunDlg::on_stopButton_clicked()
{
	stop_pressed = true;
}

void BatchrunDlg::on_disturbances_cb_clicked(bool checked)
{
	theParameters->use_linktime_disturbances = checked;

}

void BatchrunDlg::on_random_draws_val_valueChanged (int i)
{
	theParameters->routesearch_random_draws = i;
}

void BatchrunDlg::on_disturbance_val_textEdited (const QString & text)
{
	theParameters->linktime_disturbance=text.toDouble();
}

void BatchrunDlg::on_scale_cb_clicked(bool checked)
{
	theParameters->scale_demand=checked;
}

void BatchrunDlg::on_scale_val_textEdited (const QString & text)
{
	theParameters->scale_demand_factor=text.toDouble();
}

void BatchrunDlg::on_prune_cb_clicked(bool checked)
{
	theParameters->delete_bad_routes=checked;
}

void BatchrunDlg::on_max_rel_cost_val_textEdited (const QString & text)
{
	theParameters->max_rel_route_cost=text.toDouble();
}

void BatchrunDlg::on_renum_cb_clicked(bool checked)
{
	theParameters->renum_routes=checked;
}





void BatchrunDlg::show()
{	// Dynamic User Equilibrium tab	
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
	// Route Search Options tab
	max_route_iterations_val->setValue(theParameters->max_route_iter);
	disturbances_cb->setChecked(theParameters->use_linktime_disturbances);
	random_draws_val->setValue(theParameters->routesearch_random_draws);
	disturbance_val->setText(QString("%1").arg(theParameters->linktime_disturbance));
	scale_cb->setChecked(theParameters->scale_demand);
	scale_val->setText(QString("%1").arg(theParameters->scale_demand_factor));
	prune_cb->setChecked(theParameters->delete_bad_routes);
	max_rel_cost_val->setText(QString("%1").arg(theParameters->max_rel_route_cost));
	renum_cb->setChecked(theParameters->renum_routes);

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
	//if (sdue_rb->isChecked())
	//{
	
	// Set the convergence criteria and iterations
	if (!(max_iterations_cb->isChecked()))
	{
		//max_iter=max_iterations_val->value();
		theParameters->max_iter = 1000; // 
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
	theNetwork->open_convergence_file(theNetwork->get_workingdir() + "convergence.dat"); // open the convergence file
	// RUN the actual SDUE or Routesearch loops
	if (routesearch_rb->isChecked())
		run_route_iterations();
	else
		run_iterations();
	//}	
	theNetwork->close_convergence_file();
	activateAnalyzeOutput();
	//theNetwork->recenter_image();
	repaint();
	
}

const bool BatchrunDlg::checkConvergence(const int i, const double relgap_ltt_, const double relgap_rf_)
{
	// if more than max iterations, return true
	if (i > theParameters->max_iter)
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
	total_iter->setNum(theParameters->max_iter);
	double runtime=theNetwork->get_runtime();
	double curtime=0.0;
	double relgap_ltt_=1.0;
	double relgap_rf_=1.0;
	cur_iter->setNum(i);
	update();
	//theNetwork->open_convergence_file(theNetwork->get_workingdir() + "convergence.dat");
// iterations
	for (i; !checkConvergence(i,relgap_ltt_,relgap_rf_) && !stop_pressed;i++)
	{
		// update display widgets with correct values
		cur_iter->setNum(i);
		relgap_ltt->setText (QString::number(relgap_ltt_,'f',5));
		relgap_rf->setText(QString::number(relgap_rf_,'f',5));
		int progress = static_cast<int>(100*(i-1)/theParameters->max_iter);
		totalPb->setValue(progress);
		repaint();

		if (i>1)
		{
			theNetwork->copy_linktimes_out_in();
			theNetwork->reset();
			emit center_image();
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
	//theNetwork->close_convergence_file();
	//activateAnalyzeOutput();
}

void BatchrunDlg::run_route_iterations()

{
	bool breaksim = false;
	for (int i = 0; i < theParameters->max_route_iter; i++)
	{
		cur_route_iter->setNum(i+1);
		unsigned int old_nr_routes= theNetwork->get_nr_routes();
		theNetwork->get_convergence_stream() << "RouteSearch Iteration: " << i+1 << " Nr routes: " << old_nr_routes << endl;
		run_iterations();
		theNetwork->get_convergence_stream()  << endl;
		if (breaksim)
			break;
		
		if (i < (theParameters->max_route_iter-1)) // except for last iteration, then we keep the results.
		{
			if (!theParameters->shortest_paths_initialised)
				theNetwork->init_shortest_path();
			theNetwork->shortest_paths_all();
			if (theParameters->renum_routes)
				theNetwork->renum_routes();
			theNetwork->add_od_routes();
			theNetwork->reset();
			emit center_image();
		}
		unsigned int new_nr_routes= theNetwork->get_nr_routes();
		if (old_nr_routes==new_nr_routes)
		{
			//eout << "INFO: Network::run_route_iterations: no new routes found in iteration " << i << " exiting. " << endl;
			breaksim=true;
		}
	 }
	 theNetwork->writepathfile(""); // write back the routes to the standard route file
	
	
}

	