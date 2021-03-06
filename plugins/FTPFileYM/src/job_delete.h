/*
FTP File YM plugin
Copyright (C) 2007-2010 Jan Holub

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include "dbentry.h"
#include "manager.h"

class DeleteJob
{
private:
	static const int MAX_RUNNING_JOBS = 2;
		
	DBEntry *m_entry;
	Manager::TreeItem *m_treeItem;
	ServerList::FTP *m_ftp;
	char m_buff[256];

	static mir_cs mutexJobCount;
	static int iRunningJobCount;

	char *getDelFileString();
	char *getDelUrlString();

	static void __cdecl waitingThread(DeleteJob *job);
	void run();

public:
	static Event jobDone;

	DeleteJob(DBEntry *entry, Manager::TreeItem *item);
	~DeleteJob();

	void start();
};
