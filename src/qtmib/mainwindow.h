/*
 * Copyright (C) 2013 RCP100 Team (rcpteam@yahoo.com)
 *
 * This file is part of qtmib project
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>
#include <QTextEdit>

class PrefDialog;

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QComboBox;
class QLabel;
class QLineEdit;
class QProgressBar;
class QCheckBox;
class QTreeView;
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
Q_OBJECT

	public:
	MainWindow(QWidget *parent = 0);

public slots:
	void updateActions();

private slots:
	void about();
	void preferences();
	void handleAction();
	void handleClear();

private:
	void createMenu();
	QAbstractItemModel *modelFromFile(const QString& fileName);

	QTreeView *treeView;
	QAbstractItemModel *mymodel;
	QTextEdit *result;
	QTextEdit *oidView;
	QTextEdit *connectionView;
	QLineEdit *actionIp;
	QLineEdit *actionOid;
	QComboBox *action;
	PrefDialog *pref;
};
#endif