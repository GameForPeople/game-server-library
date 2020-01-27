#pragma once

#define WONSY_BASE_GAMESERVER

namespace WonSY
{
	class BaseGameServer
	{
	public:
		void Run();
	
	public:
		BaseGameServer() noexcept;
		
		BaseGameServer(const BaseGameServer&) = delete;
		BaseGameServer& operator=(const BaseGameServer&) = delete;
		BaseGameServer(BaseGameServer&&) = delete;
		BaseGameServer& operator=(BaseGameServer&&) = delete;
	private:

	private:
		// HANDLE iocpHandle;
		// SOCKET listenSocket;
		// SOCKADDR_IN	serverAddr;
	};
}