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

#include <stdio.h>
#include <string.h>

#include <ClientBase.h>
#include <Exception.h>

#include <xqilla/xqilla-dom3.hpp>
#include <xercesc/dom/DOM.hpp>

#include <map>
#include <string>

using namespace std;
using namespace xercesc;

static void usage()
{
	fprintf(stderr,"Usage : evqueue_api [--connect <cnx string>] --group --action [--<parameter name> <parameter value>]*\n");
	exit(-1);
}

int main(int argc, char  **argv)
{
	// Parse cmdline parameters
	const char *connection_str = "tcp://localhost:5000";
	
	int cur;
	for(cur=1;cur<argc;cur++)
	{
		if(strcmp(argv[cur],"--connect")==0)
		{
			if(cur+1>=argc)
				usage();
			
			connection_str = argv[cur+1];
			cur++;
		}
		else
			break;
	}
	
	// Read group and action
	if(cur+1>=argc)
		usage();
	
	if(strncmp(argv[cur],"--",2)!=0)
		usage();
	const char *group = argv[cur]+2;
	
	if(strncmp(argv[cur+1],"--",2)!=0)
		usage();
	const char *action = argv[cur+1]+2;
	
	cur+=2;
	
	// Read parameters
	map<string,string> parameters;
	for(;cur<argc;cur+=2)
	{
		if(strncmp(argv[cur],"--",2)!=0)
			usage();
		
		string name = argv[cur];
		if(cur+1>=argc)
			return -1;
		
		string value = argv[cur+1];
		
		parameters[name] = value;
	}
	
	// Build API Query XML
	XQillaPlatformUtils::initialize();
	
	DOMImplementation *xqillaImplementation = DOMImplementationRegistry::getDOMImplementation(X("XPath2 3.0"));
	DOMDocument *xmldoc = xqillaImplementation->createDocument();
	
	DOMElement *root_node = xmldoc->createElement(X(group));
	xmldoc->appendChild(root_node);
	
	root_node->setAttribute(X("action"),X(action));
	for(auto it=parameters.begin();it!=parameters.end();it++)
	{
		string name = it->first;
		name = name.substr(2);
		replace( name.begin(), name.end(), '-', '_');
		root_node->setAttribute(X(name.c_str()),X(it->second.c_str()));
	}
		
	DOMLSSerializer *serializer = xqillaImplementation->createLSSerializer();
	XMLCh *query_xml = serializer->writeToString(root_node);
	char *query_xml_c = XMLString::transcode(query_xml);
	
	// Send launch command to evQueue
	
	int exit_status = 0;
	
	try
	{
		ClientBase client(connection_str,"","");
		client.Exec(query_xml_c,true);
		
		DOMDocument *xmldoc = client.GetResponseDOM();
		
		DOMLSSerializer *serializer = xqillaImplementation->createLSSerializer();
		XMLCh *response_xml = serializer->writeToString(xmldoc->getDocumentElement());
		char *response_xml_c = XMLString::transcode(response_xml);
		
		printf("%s\n",response_xml_c);
		
		XMLString::release(&response_xml);
		XMLString::release(&response_xml_c);
		serializer->release();
	}
	catch(Exception &e)
	{
		printf("Exception in client : %s\n",e.error.c_str());
		exit_status = -1;
	}
	
	XMLString::release(&query_xml);
	XMLString::release(&query_xml_c);
	serializer->release();
	xmldoc->release();
	
	XQillaPlatformUtils::terminate();
	
	return exit_status;
}