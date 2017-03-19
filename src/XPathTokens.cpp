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

#include <XPathTokens.h>
#include <DOMNode.h>
#include <Exception.h>

using namespace std;

int Token::cast_string_to_int(const string &s)
{
	try
	{
		size_t pos;
		int n = stoi(s,&pos);
		if(pos!=s.length())
			throw Exception("XPath","Could not cast string to int");
		return  n;
	}
	catch(...)
	{
		throw Exception("XPath","Could not cast string to int");
	}
}

double Token::cast_string_to_double(const string &s)
{
	try
	{
		size_t pos;
		int d = stod(s,&pos);
		if(pos!=s.length())
			throw Exception("XPath","Could not cast string to float");
		return d;
	}
	catch(...)
	{
		throw Exception("XPath","Could not cast string to float");
	}
}

int Token::cast_token_to_int(const Token *token)
{
	if(token->GetType()==LIT_FLOAT)
		throw Exception("XPath","Could not cast float to int");
	else if(token->GetType()==LIT_INT)
		return ((TokenInt *)token)->i;
	else if(token->GetType()==LIT_BOOL)
		return ((TokenBool *)token)->b?1:0;
	else if(token->GetType()==LIT_STR)
		return cast_string_to_int(((TokenString *)token)->s);
	else if(token->GetType()==NODELIST && ((TokenNodeList *)token)->nodes.size()==1)
		return cast_string_to_int(((TokenNodeList *)token)->nodes.at(0).getNodeValue());
	throw Exception("XPath","Incompatible type for operand");
}

double Token::cast_token_to_double(const Token *token)
{
	if(token->GetType()==LIT_FLOAT)
		return ((TokenFloat *)token)->d;
	else if(token->GetType()==LIT_INT)
		return ((TokenInt *)token)->i;
	else if(token->GetType()==LIT_BOOL)
		return ((TokenBool *)token)->b?1:0;
	else if(token->GetType()==LIT_STR)
		return cast_string_to_double(((TokenString *)token)->s);
	else if(token->GetType()==NODELIST && ((TokenNodeList *)token)->nodes.size()==1)
		return cast_string_to_double(((TokenNodeList *)token)->nodes.at(0).getNodeValue());
	throw Exception("XPath","Incompatible type for operand");
}

Token::operator int() const
{
	return cast_token_to_int(this);
}

Token::operator double() const
{
	return cast_token_to_double(this);
}

Token::operator string() const
{
	if(GetType()==LIT_STR)
		return ((TokenString *)this)->s;
	else if(GetType()==LIT_INT)
		return to_string(((TokenInt *)this)->i);
	else if(GetType()==LIT_FLOAT)
		return to_string(((TokenFloat *)this)->d);
	else if(GetType()==LIT_BOOL)
		return to_string(((TokenBool *)this)->b);
	else if(GetType()==NODELIST && ((TokenNodeList *)this)->nodes.size()==1)
		return ((TokenNodeList *)this)->nodes.at(0).getNodeValue();
	throw Exception("XPath","Incompatible type for operand");
}

TokenExpr::TokenExpr(TokenExpr &expr)
{
	for(int i=0;i<expr.expr_tokens.size();i++)
		expr_tokens.push_back(expr.expr_tokens.at(i)->clone());
}

TokenExpr::~TokenExpr()
{
	for(int i=0;i<expr_tokens.size();i++)
		delete expr_tokens.at(i);
}

TokenNodeName::TokenNodeName(TokenNodeName &node_name)
{
	name = node_name.name;
	if(filter)
		filter = new TokenExpr(*(node_name.filter));
	else
		filter = 0;
}

TokenNodeName::~TokenNodeName()
{
	if(filter)
		delete filter;
}

TokenAttrName::TokenAttrName(TokenAttrName &attr_name)
{
	name = attr_name.name;
	if(filter)
		filter = new TokenExpr(*(attr_name.filter));
	else
		filter = 0;
}

TokenAttrName::~TokenAttrName()
{
	if(filter)
		delete filter;
}

TokenFunc::TokenFunc(TokenFunc &f)
{
	name = f.name;
	for(int i=0;i<args.size();i++)
		args.push_back(new TokenExpr(*args.at(i)));
}

TokenFunc::~TokenFunc()
{
	for(int i=0;i<args.size();i++)
		delete args.at(i);
}

TokenNodeList::TokenNodeList(DOMNode node)
{
	nodes.push_back(node);
}

TokenNodeList::TokenNodeList(TokenNodeList &list)
{
	nodes = list.nodes;
}