/*
 * zsummerX License
 * -----------
 * 
 * zsummerX is licensed under the terms of the MIT license reproduced below.
 * This means that zsummerX is free software and can be used for both academic
 * and commercial purposes at absolutely no cost.
 * 
 * 
 * ===============================================================================
 * 
 * Copyright (C) 2010-2014 YaweiZhang <yawei_zhang@foxmail.com>.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * 
 * ===============================================================================
 * 
 * (end of COPYRIGHT)
 */


//! zsummerX  http proto test



#include <zsummerX/frameX.h>
using namespace zsummer::log4z;

std::string g_remoteIP = "0.0.0.0";
unsigned short g_remotePort = 8081;
unsigned short g_startIsConnector = 0;  //0 listen, 1 connect


int main(int argc, char* argv[])
{
	if (argc == 2 && 
		(strcmp(argv[1], "--help") == 0 
		|| strcmp(argv[1], "/?") == 0))
	{
		cout <<"please input like example:" << endl;
		cout << "tcpTest remoteIP remotePort startType maxClient sendType interval" << endl;
		cout << "./tcpTest 0.0.0.0 8081 0" << endl;
		cout << "startType: 0 server, 1 client" << endl;
		return 0;
	}
	if (argc > 1)
	{
		g_remoteIP = argv[1];
	}
	if (argc > 2)
	{
		g_remotePort = atoi(argv[2]);
	}
	if (argc > 3)
	{
		g_startIsConnector = atoi(argv[3]);
	}

	
	if (!g_startIsConnector)
	{
		//! start log4z 
		ILog4zManager::getPtr()->config("server.cfg");
		ILog4zManager::getPtr()->start();
	}
	else
	{
		//! start log4z 
		ILog4zManager::getPtr()->config("client.cfg");
		ILog4zManager::getPtr()->start();
	}
	LOGI("g_remoteIP=" << g_remoteIP << ", g_remotePort=" << g_remotePort << ", g_startIsConnector=" << g_startIsConnector );


	//! step 1. start frame manager.
	TcpSessionManager::getRef().start();


	if (g_startIsConnector) //client
	{
		//callback when connect success.
		auto connectedfun = [](SessionID cID)
		{
//			std::string jsonString = R"---(data={%22uid%22:10001,%22session%22:%220192023a7bbd73250516f069df18b500%22})---";
			std::string jsonString = R"---(data=)---";
			jsonString += zsummer::proto4z::urlEncode(std::string(R"---({"uid":10001,"session":"0192023a7bbd73250516f069df18b500"})---"));

			LOGI("send to ConnectorID=" << cID);
			zsummer::proto4z::WriteHTTP wh;
// 			wh.addHead("Accept", " text/html, application/xhtml+xml, */*");
// 			wh.addHead("Accept-Language", "zh-CN");
// 			wh.addHead("User-Agent", "Mozilla/5.0 ");
			wh.addHead("Content-Type", "application/x-www-form-urlencoded");
//			wh.addHead("Accept-Encoding", "utf8");
			wh.addHead("Host", "10.0.0.197");
//			wh.addHead("DNT", "1");
//			wh.addHead("Connection", "Keep-Alive");
			wh.post("/user/oauth", jsonString);
			TcpSessionManager::getRef().sendOrgSessionData(cID, wh.getStream(), wh.getStreamLen());
		};

		//callback when receive http data
		auto msg_ResultSequence_fun = [](SessionID cID, const zsummer::proto4z::PairString &commondLine, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
		{
			if (commondLine.second != "200")
			{
				LOGI("response false. commond=" << commondLine.first << ", commondvalue=" << commondLine.second);
				TcpSessionManager::getRef().kickSession(cID);
				return ;
			}
			LOGI("response success. commond=" << commondLine.first << ", commondvalue=" << commondLine.second << ", content=" << body);
			TcpSessionManager::getRef().kickSession(cID);
			//step 3. stop
			//TcpSessionManager::getRef().stop();
			return ;
		};

		//! register event and message
		MessageDispatcher::getRef().registerOnSessionEstablished(connectedfun); //!connect success
		MessageDispatcher::getRef().registerOnSessionHTTPMessage(msg_ResultSequence_fun);//! message


		//add connector
		tagConnctorConfigTraits traits;
		traits.remoteIP = g_remoteIP;
		traits.remotePort = g_remotePort;
		traits.protoType = PT_HTTP;
		traits.reconnectInterval = 5000;
		traits.reconnectMaxCount = 0;
		TcpSessionManager::getRef().addConnector(traits);

		//! step 2 running
		TcpSessionManager::getRef().run();
	}
	else
	{

		//result when receive http data
		auto msg_ResultSequence_fun = [](SessionID sID, const zsummer::proto4z::PairString &commondLine, const zsummer::proto4z::HTTPHeadMap &head, const std::string & body)
		{
			LOGI("recv request. commond=" << commondLine.first << ", commondvalue=" << commondLine.second);
			zsummer::proto4z::WriteHTTP wh;
			wh.addHead("Accept", " text/html, application/xhtml+xml, */*");
			wh.addHead("Accept-Language", "zh-CN");
			wh.addHead("User-Agent", "Mozilla/5.0 ");
			wh.addHead("Accept-Encoding", "utf8");
			wh.addHead("Host", "www.baidu.com");
			wh.addHead("DNT", "1");
			wh.addHead("Connection", "Keep-Alive");
			wh.response("200", "What's your name ?");
			TcpSessionManager::getRef().sendOrgSessionData( sID, wh.getStream(), wh.getStreamLen());
			TcpSessionManager::getRef().createTimer(2000, std::bind(&TcpSessionManager::kickSession, TcpSessionManager::getPtr(), sID));
			//step 3. stop server.
		//	TcpSessionManager::getRef().createTimer(1000,std::bind(&TcpSessionManager::stop, TcpSessionManager::getPtr()));
			return ;
		};

		//! register message
		MessageDispatcher::getRef().registerOnSessionHTTPMessage(msg_ResultSequence_fun);

		tagAcceptorConfigTraits traits;
		traits.listenPort = g_remotePort;
		traits.protoType = PT_HTTP;
		TcpSessionManager::getRef().addAcceptor(traits);
		//! step 2 running
		TcpSessionManager::getRef().run();
	}
	

	return 0;
}

