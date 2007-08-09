/**
* implementation of the custom delegate for the 
* tableview of routes of a given OD pair
*
* Xiaoliang Ma
* last update: 2007-08-02
*
*/

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>
#include "odtabledelegate.h"

ODTableViewDelegate::ODTableViewDelegate(int viewcol, QObject *parent):QItemDelegate(parent)
{
	this->viewcol_=viewcol;
}

QWidget* ODTableViewDelegate::createEditor(QWidget *parent, 
										const QStyleOptionViewItem &option,
                                        const QModelIndex &index) const
{
	if(index.column()==viewcol_){
		QComboBox* comboboxeditor = new QComboBox(parent);
		comboboxeditor->addItem("none"); 
		comboboxeditor->addItem("black");
		comboboxeditor->addItem("blue");
		comboboxeditor->addItem("green");
		comboboxeditor->addItem("red");
		comboboxeditor->addItem("magenta");

		// add an event handler to the item delegate
		comboboxeditor->installEventFilter(const_cast<ODTableViewDelegate*>(this));
		return comboboxeditor;
	}
	else{
		return QItemDelegate::createEditor(parent, option, index);
	}
}

void ODTableViewDelegate::setEditorData(QWidget *editor, const QModelIndex &index) const
{
	//const QAbstractItemModel* itemmodel=index.model();
	//QString itemText=itemmodel->(index, Qt::DisplayRole).toString();
	
	if(index.column()==viewcol_){
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		//comboBox->setCurrentIndex(0);
		//comboBox->show();
	}
	else{
		QItemDelegate::setEditorData(editor, index);
	}
}

void ODTableViewDelegate::setModelData(QWidget *editor, 
								   QAbstractItemModel *model,
								   const QModelIndex &index) const
{
	if(index.column()==viewcol_){
		QComboBox *comboBox = static_cast<QComboBox*>(editor);
		QString colortext= comboBox->currentText();
		model->setData(index, colortext);
		if (colortext!=""){
			int rowcount=index.row();
			emit activateAColor(colortext,rowcount);
		}
	}else{
		QItemDelegate::setModelData(editor,model,index);
	}
}

void ODTableViewDelegate::updateEditorGeometry(QWidget *editor, 
										   const QStyleOptionViewItem &option, 
						                   const QModelIndex &index) const
{
	if(index.column()==viewcol_){
		editor->setGeometry(option.rect);
	}
	else{
		QItemDelegate::updateEditorGeometry(editor, option, index);
	}
}



