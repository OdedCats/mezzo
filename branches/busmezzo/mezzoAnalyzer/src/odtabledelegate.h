/*
	Mezzo Mesoscopic Traffic Simulation 
	Copyright (C) 2008  Wilco Burghout

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

/**
* definition of the custom delegate for the 
* tableview of routes of a given OD pair
* as a child of QItemDelegate
*
* Xiaoliang Ma
* last update: 2007-07-30
*
*/

#ifndef ODTABLEDELEGATE_H
#define ODTABLEDELEGATE_H

#include <QObject>
#include <QString>
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

 signals:
	 void activateAColor(const QString&, const int&) const;

 private:

	 // the column index for view color of routes
	 int viewcol_;
};


#endif
