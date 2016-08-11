/*
 * This file is part of evQueue
 * 
 * evQueue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * evQueue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with evQueue. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Thibault Kummer <bob@coldsource.net>
 */

#include <tools.h>
#include <tools_ipc.h>
#include <global.h>
#include <Logger.h>
#include <WorkflowScheduler.h>
#include <Tasks.h>
#include <RetrySchedules.h>
#include <Workflows.h>
#include <Notifications.h>
#include <NotificationTypes.h>
#include <Retrier.h>
#include <QueuePool.h>
#include <Configuration.h>
#include <SocketQuerySAX2Handler.h>
#include <QueryResponse.h>
#include <Statistics.h>
#include <WorkflowInstances.h>
#include <Exception.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

using namespace std;

int tools_queue_destroy()
{
	int msgqid = ipc_openq(Configuration::GetInstance()->Get("core.ipc.qid").c_str());
	if(msgqid==-1)
		return -1;
	
	int re = msgctl(msgqid,IPC_RMID,0);
	if(re!=0)
	{
		if(errno==EPERM)
			fprintf(stderr,"Permission refused while trying to remove message queue\n");
		else
			fprintf(stderr,"Unknown error trying to remove message queue : %d\n",errno);
		
		return -1;
	}
	
	printf("Message queue successfully removed\n");
	
	return 0;
}

int tools_queue_stats()
{
int msgqid = ipc_openq(Configuration::GetInstance()->Get("core.ipc.qid").c_str());
	if(msgqid==-1)
		return -1;
	
	struct msqid_ds ipcq_stats;
	int re = msgctl(msgqid,IPC_STAT,&ipcq_stats);
	if(re!=0)
	{
		fprintf(stderr,"Unknown error trying to get message queue statistics: %d\n",errno);
		return -1;
	}
	
	printf("Queue size : %ld\n",ipcq_stats.msg_qbytes);
	printf("Pending messages : %ld\n",ipcq_stats.msg_qnum);
}

void tools_print_usage()
{
	fprintf(stderr,"Usage :\n");
	fprintf(stderr,"  Launch evqueue      : evqueue (--daemon) --config <path to config file>\n");
	fprintf(stderr,"  Clean IPC queue     : evqueue --config <path to config file> --ipcq-remove\n");
	fprintf(stderr,"  Get IPC queue stats : evqueue --config <path to config file> --ipcq-stats\n");
	fprintf(stderr,"  Show version        : evqueue --version\n");
}

void tools_config_reload(const std::string &module)
{
	Logger::Log(LOG_NOTICE,"Got SIGHUP, reloading configuration");
	
	if(module=="all" || module=="scheduler")
	{
		WorkflowScheduler *scheduler = WorkflowScheduler::GetInstance();
		scheduler->Reload();
	}
	
	if(module=="all" || module=="tasks")
	{
		Tasks *tasks = Tasks::GetInstance();
		tasks->Reload();
	}
	
	if(module=="all" || module=="retry_schedules")
	{
		RetrySchedules *retry_schedules = RetrySchedules::GetInstance();
		retry_schedules->Reload();
	}
	
	if(module=="all" || module=="workflows")
	{
		Workflows *workflows = Workflows::GetInstance();
		workflows->Reload();
	}
	
	if(module=="all" || module=="notifications")
	{
		NotificationTypes::GetInstance()->Reload();
		Notifications::GetInstance()->Reload();
	}
	
	if(module=="all" || module=="queuepool")
	{
		QueuePool *qp = QueuePool::GetInstance();
		qp->Reload();
	}
}

void tools_sync_tasks(void)
{
	Tasks *tasks = Tasks::GetInstance();
	tasks->SyncBinaries();
}

void tools_sync_notifications(void)
{
	NotificationTypes::GetInstance()->SyncBinaries();
}

void tools_flush_retrier(void)
{
	Logger::Log(LOG_NOTICE,"Flushing retrier");
	Retrier *retrier = Retrier::GetInstance();
	retrier->Flush();
}

int tools_send_exit_msg(int type,int tid,char retcode)
{
	int msgqid = ipc_openq(Configuration::GetInstance()->Get("core.ipc.qid").c_str());
	if(msgqid==-1)
		return -1;
	
	st_msgbuf msgbuf;
	msgbuf.type = type;
	memset(&msgbuf.mtext,0,sizeof(st_msgbuf::mtext));
	msgbuf.mtext.pid = getpid();
	msgbuf.mtext.tid = tid;
	msgbuf.mtext.retcode = retcode;
	return msgsnd(msgqid,&msgbuf,sizeof(st_msgbuf::mtext),0);
}

bool tools_handle_query(SocketQuerySAX2Handler *saxh, QueryResponse *response)
{
	const string action = saxh->GetRootAttribute("action");
	
	if(saxh->GetQueryGroup()=="control")
	{
		if(action=="reload")
		{
			string module = saxh->GetRootAttribute("module","");
			if(module.length()==0)
				tools_config_reload();
			else
				tools_config_reload(module);
			
			return true;
		}
		else if(action=="retry")
		{
			tools_flush_retrier();
			return true;
		}
		else if(action=="synctasks")
		{
			tools_sync_tasks();
			return true;
		}
		else if(action=="syncnotifications")
		{
			tools_sync_notifications();
			return true;
		}
	}
	else if(saxh->GetQueryGroup()=="status")
	{
		if(action=="query")
		{
			string type = saxh->GetRootAttribute("type");
			
			Statistics *stats = Statistics::GetInstance();
			
			if(type=="workflows")
			{
				stats->IncStatisticsQueries();
				
				WorkflowInstances *workflow_instances = WorkflowInstances::GetInstance();
				workflow_instances->SendStatus(response);
				
				return true;
			}
			else if(type=="scheduler")
			{
				stats->IncStatisticsQueries();
				
				WorkflowScheduler *scheduler = WorkflowScheduler::GetInstance();
				scheduler->SendStatus(response);
				
				return true;
			}
			else if(type=="configuration")
			{
				stats->IncStatisticsQueries();
				
				Configuration *config = Configuration::GetInstance();
				config->SendConfiguration(response);
				
				return true;
			}
		}
	}
	
	return false;
}
