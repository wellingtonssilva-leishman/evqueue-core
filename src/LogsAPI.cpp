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

#include <LogsAPI.h>
#include <Exception.h>
#include <DB.h>
#include <SocketQuerySAX2Handler.h>
#include <QueryResponse.h>
#include <Workflows.h>
#include <Workflow.h>
#include <WorkflowSchedules.h>
#include <WorkflowSchedule.h>
#include <RetrySchedules.h>
#include <RetrySchedule.h>
#include <Tags.h>
#include <Tag.h>
#include <QueuePool.h>

using namespace std;

bool LogsAPI::HandleQuery(const User &user, SocketQuerySAX2Handler *saxh, QueryResponse *response)
{
	string action = saxh->GetRootAttribute("action");
	
	if(action=="list")
	{
		unsigned int limit = saxh->GetRootAttributeInt("limit",100);
		unsigned int offset = saxh->GetRootAttributeInt("offset",0);
		
		DB db;
		
		db.QueryPrintf("SELECT node_name,user_login,log_api_object_id,log_api_object_type,log_api_group,log_api_action,log_api_timestamp FROM t_log_api ORDER BY log_api_timestamp DESC,log_api_id DESC LIMIT %i,%i",&offset,&limit);
		while(db.FetchRow())
		{
			DOMElement node = (DOMElement)response->AppendXML("<log />");
			node.setAttribute("node",db.GetField(0));
			node.setAttribute("user",db.GetField(1));
			
			node.setAttribute("object_id",db.GetField(2));
			node.setAttribute("object_type",db.GetField(3));
			
			string object_name = "unknown";
			try
			{
				if(db.GetField(3)=="Workflow")
					object_name = Workflows::GetInstance()->Get(db.GetFieldInt(2)).GetName();
				else if(db.GetField(3)=="WorkflowSchedule")
					object_name = WorkflowSchedules::GetInstance()->Get(db.GetFieldInt(2)).GetWorkflowName();
				else if(db.GetField(3)=="RetrySchedule")
					object_name = RetrySchedules::GetInstance()->Get(db.GetFieldInt(2)).GetName();
				else if(db.GetField(3)=="Tag")
					object_name = Tags::GetInstance()->Get(db.GetFieldInt(2)).GetLabel();
				else if(db.GetField(3)=="Queue")
					object_name = QueuePool::GetQueueName(db.GetFieldInt(2));
			}
			catch(Exception &e) {}
			node.setAttribute("object_name",object_name);
			
			node.setAttribute("group",db.GetField(4));
			node.setAttribute("action",db.GetField(5));
			node.setAttribute("timestamp",db.GetField(6));
		}
		
		return true;
	}
	
	return false;
}
