/*
 * this is the definition of the node dialog
 *
 * Xiaoliang Ma
 * Last modification: 2007-11-20
 *
 */

#ifndef  NODEDLG_H
#define  NODEDLG_H

#include <QDialog>
// use the generated ui form header  
#include "../ui_nodedlg.h"
// node definition
#include "../mezzo_lib/src/node.h"

class NodeDlg : public QDialog, public Ui::NodeDlg
{
	Q_OBJECT
public:
	NodeDlg(Node* curnode, QWidget* parent=0);
	~NodeDlg(){}
	
private:		 
	Node* curnode_;
};

#endif

