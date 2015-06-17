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

#include <FileManager.h>
#include <Exception.h>
#include <Logger.h>
#include <base64.h>

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

using namespace std;

bool FileManager::CheckFileName(const string &file_name)
{
	if(file_name[0]=='.')
		return false;
	
	for(int i=0;i<file_name.length();i++)
		if(!isalnum(file_name[i]) && file_name[i]!='_' && file_name[i]!='-' && file_name[i]!='.')
			return false;
	
	return true;
}

void FileManager::PutFile(const string &directory,const string &filename,const string &data,int filetype)
{
	if(!CheckFileName(filename))
		throw Exception("Notification","Invalid file name");
	
	string path = directory+"/"+filename;
	
	int fd;
	if(filetype==FILETYPE_CONF)
		fd = open(path.c_str(),O_CREAT|O_RDWR,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
	else if(filetype==FILETYPE_BINARY)
		fd = open(path.c_str(),O_CREAT|O_EXCL|O_RDWR,S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH);
	
	if(fd==-1)
	{
		if(errno==EEXIST)
		{
			Logger::Log(LOG_ERR,"File already exist : %s",path.c_str());
			throw Exception("Notification","File already exist");
		}
		
		Logger::Log(LOG_ERR,"Unable to create file : %s",path.c_str());
		throw Exception("Notification","Unable to create file");
	}
	
	FILE *f = fdopen(fd,"w");
	bool re = base64_decode_file(f,data.c_str());
	fclose(f);
	
	if(!re)
	{
		unlink(path.c_str());
		throw Exception("Notification","Invalid file data");
	}
}

void FileManager::RemoveFile(const string &directory,const string &filename)
{
	if(!CheckFileName(filename))
		throw Exception("Notification","Invalid file name");
	
	string path = directory+"/"+filename;
	
	int re = unlink(path.c_str());
	if(re!=0)
	{
		Logger::Log(LOG_ERR,"Unable to remove file : %s",path.c_str());
		throw Exception("Notification","Unable to remove file");
	}
}