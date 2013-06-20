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
#include <QtGui>

#include "transactionthread.h"
#include "dev_storage.h"
#include "dev_db.h"
#include "exec_prog.h"
#include "qtmib_socket.h"
#include "qtmib_discover.h"

TransactionThread::TransactionThread(): ending_(0), sock_(0) {
	start();
}


TransactionThread::~TransactionThread() {
	ending_ = 1;
	close(sock_);   
	wait();
}


void TransactionThread::addTransaction(DevStorage *dev) {
	QMutexLocker locker(&mutex);
	queue_add_.append(dev);
}

static QString extract_string(QString line) {
	int index = line.indexOf(" = STRING: \"");
	if (index != -1) {
		index += 12;
		QString left = line.mid(index);
		index = left.indexOf("\"");
		if (index != -1)
			left.truncate(index); 
		return left;
	}
	
	return QString(" ");
}

void TransactionThread::checkDevice(DevStorage *dev, TransactionThread *th) {
	// test exit falg	
	if (th->ending_)
		return;
		
	if (dev->timeout_ == 0 || dev->timeout_ == 20) {
		int port = atoi(dev->port_.toStdString().c_str());
		int type = 1;
		if (dev->version_ == "v1")
			type = 0;
			
		tx_packet(th->sock_, dev->range_start_,
			port, type, dev->community_.toStdString().c_str());
	}
	
	else if (dev->timeout_ > 40) {
		QString cmd = "snmpbulkget -m \"\" ";
		if (dev->version_ == "v1")
			cmd = "snmpget -m \"\" ";
		cmd += "-" + dev->version_ + " ";
		cmd += "-c " + dev->community_ + " ";
		cmd += "-t 1 ";
		cmd += "-r 0 ";
		cmd += dev->ip_ + ":" + dev->port_ + " ";
		if (dev->version_ == "v1")
			cmd += ".1.3.6.1.2.1.1.5.0 2>&1";
		else
			cmd += ".1.3.6.1.2.1.1 2>&1";
	
		// execute command
		char *rv = exec_prog(cmd.toStdString().c_str());
		bool found = false;
		if (rv) {
			QString out = "add\t" + dev->ip_ + "\t";
			if (dev->version_ == "v1")
				 out = "add\t" + dev->ip_ + "\t \t";
			QString input = rv;
			QStringList lines = input.split( "\n", QString::SkipEmptyParts );
			foreach (QString line, lines) {
				if (line.startsWith("iso.3.6.1.2.1.1.4.0")) {
					out += extract_string(line) + "\t";
					found = true;
				}
				else if (line.startsWith("iso.3.6.1.2.1.1.5.0")) {
					out += extract_string(line) + "\t";
					found = true;
				}
				else if (line.startsWith("iso.3.6.1.2.1.1.6.0")) {
					out += extract_string(line) + "\t";
					found = true;
				}
					
			}
			if (found) {
				emit th->displayResult(out);
			}
			free(rv);
		}
		dev->remove_ = 1;
	}

//	if (dbg)		
//		printf("%s %d\n", dev->ip_.toStdString().c_str(), dev->timeout_);
	if (dev->timeout_ == 40)
		dev->remove_ = 1;
	dev->timeout_++;
}





void TransactionThread::run() {
	sock_ = rx_open(29456);

	forever {
		msleep(200);

		// test exit falg	
		if (ending_)
			break;

		mutex.lock();
		int addcnt = queue_add_.count();
		if (addcnt == 0)
;//			printf("sleep\n");
		else {
			for (int i = 0; i < addcnt; i++) {
				DevStorage *dev = queue_add_.at(0);
//				printf("sleep; add ip %s\n", dev->ip_.toStdString().c_str());
				
				uint32_t range_start = dev->range_start_;
				uint32_t range_end = dev->range_start_;
				for (int j = 0; j < 5; j++) {
					if (dev->range_start_ > dev->range_end_)
						break;

					DevStorage *newdev = new DevStorage(dev);
					// add dev to device database
					DevDb::add(newdev);
					dev->range_start_++;
					range_end++;
				}
				
				char msg[100];
				if (dev->range_start_ > dev->range_end_) {
					// remove dev from input queue
					queue_add_.removeFirst();
					sprintf(msg, "Finishing...");
				}
				
				else 
					sprintf(msg, "Checking range %d.%d.%d.%d to %d.%d.%d.%d",
						RCP_PRINT_IP(range_start), RCP_PRINT_IP(range_end));

				emit transactionDone(QString(msg));
			}
		}
		mutex.unlock();
		
		// check the socket
		uint32_t ip;
		while ((ip = rx_packet(sock_)) != 0) {
			// find the device and mark it for snmpbulkget
			DevStorage *dev = DevDb::find(ip);
			if (dev)
				dev->timeout_ = 41;
		}
		
		// retrieve data
		DevDb::walk(TransactionThread::checkDevice, this);
		
		// test exit falg	
		if (ending_)
			break;
	}
	
	close(sock_);
}


