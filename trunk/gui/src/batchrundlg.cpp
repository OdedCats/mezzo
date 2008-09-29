#include <QtGui>
#include "batchrundlg.h"

BatchrunDlg::BatchrunDlg( QWidget* parent)
{
	theNetwork=NULL;
	setupUi(this);

}

void BatchrunDlg::setNetwork(Network* net)
{
	theNetwork=net;
}

void BatchrunDlg::on_run_clicked()
{
	// check what type of iteration is to be done

	if (iterate_traveltimes_rb->isChecked())
	{
		int max_iter=max_iterations_val->value();
		double max_rmsn=0.1;
		run_iterations(max_iter,max_rmsn);
	}

	
}

void BatchrunDlg::run_iterations(int nr_iterations, double max_rmsn)
{
	int i=1;
	double runtime=theNetwork->get_runtime();
	theNetwork->step(runtime);
	if (nr_iterations ==1)
		return;
	i++;
	for (i; i<=nr_iterations;i++)
	{
		int progress = 100*static_cast<int>(i/nr_iterations);
		progressBar->setValue(progress);
		theNetwork->copy_linktimes_out_in();
		theNetwork->reset();
		theNetwork->step(runtime);
	}
	
}

	