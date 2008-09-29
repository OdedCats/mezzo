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

	