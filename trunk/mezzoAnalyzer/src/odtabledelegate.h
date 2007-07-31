/**
* definition of the custom delegate for the 
* tableview of routes of a given OD pair
*
* Xiaoliang Ma
* last update: 2007-07-30
*
*/

#ifndef ODTABLEDELEGATE_H
#define ODTABLEDELEGATE_H

#include <QObject>
#include <QItemDelegate>
#include <QModelIndex>
#include <QWidget>
#include <QSize>
#include <QComboBox>

class ODTableViewDelegate : public QItemDelegate
{
     Q_OBJECT

 public:
     
	 ODTableViewDelegate(int viewcol, QObject *parent = 0);
	 ~ODTableViewDelegate(){}

     QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                           const QModelIndex &index) const;

     void setEditorData(QWidget *editor, const QModelIndex &index) const;
     void setModelData(QWidget *editor, QAbstractItemModel *model,
                       const QModelIndex &index) const;

     void updateEditorGeometry(QWidget *editor,
         const QStyleOptionViewItem &option, const QModelIndex &index) const;

 private:

	 // the column index for view color of routes
	 int viewcol_;
};


#endif
