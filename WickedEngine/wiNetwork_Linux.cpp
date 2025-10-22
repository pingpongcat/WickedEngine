#include "wiPlatform.h"

#ifdef PLATFORM_LINUX
#include "wiNetwork.h"
#include "wiBacklog.h"
#include "wiTimer.h"

#include <string>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>

namespace wi::network
{
	//For easy address conversion
	struct in_addr_union {
		union {
			struct { uint8_t s_b1,s_b2,s_b3,s_b4; } S_un_b;
			struct { uint16_t s_w1,s_w2; } S_un_w;
			uint32_t S_addr;
		};
	};

	struct SocketInternal{
		int handle;
		~SocketInternal(){
			if (handle >= 0)
			{
				int result = close(handle);
				if(result < 0){
					wi::backlog::post("wi::network_Linux error closing socket: " + std::string(strerror(errno)), wi::backlog::LogLevel::Warning);
				}
			}
		}
	};

	SocketInternal* to_internal(const Socket* param)
	{
		return static_cast<SocketInternal*>(param->internal_state.get());
	}

	bool CreateSocket(Socket* sock)
	{
		std::shared_ptr<SocketInternal> socketinternal = std::make_shared<SocketInternal>();
		sock->internal_state = socketinternal;

		socketinternal->handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

		if(socketinternal->handle == -1)
		{
			wi::backlog::post("wi::network_Linux error in CreateSocket: Could not create socket");
			return false;
		}

		// Set socket to non-blocking mode to prevent freezing
		int flags = fcntl(socketinternal->handle, F_GETFL, 0);
		if (flags == -1)
		{
			wi::backlog::post("wi::network_Linux error in CreateSocket: fcntl F_GETFL failed", wi::backlog::LogLevel::Warning);
		}
		else
		{
			if (fcntl(socketinternal->handle, F_SETFL, flags | O_NONBLOCK) == -1)
			{
				wi::backlog::post("wi::network_Linux error in CreateSocket: fcntl F_SETFL failed", wi::backlog::LogLevel::Warning);
			}
		}

		return true;
	}

	bool Send(const Socket* sock, const Connection* connection, const void* data, size_t dataSize)
	{
		if (sock->IsValid()){
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(connection->port);
			in_addr_union address;
			address.S_un_b.s_b1 = connection->ipaddress[0];
			address.S_un_b.s_b2 = connection->ipaddress[1];
			address.S_un_b.s_b3 = connection->ipaddress[2];
			address.S_un_b.s_b4 = connection->ipaddress[3];
			target.sin_addr.s_addr = address.S_addr;

			auto socketinternal = to_internal(sock);
			int result = sendto(socketinternal->handle, (const char*)data, (int)dataSize, 0, (const sockaddr*)&target, sizeof(target));
			if (result < 0)
			{
				wi::backlog::post("wi::network_Linux error in Send: (Error Code: " + std::to_string(result) + ") " + std::string(strerror(result)));
				return false;
			}

			return true;
		}
		return false;
	}

	bool ListenPort(const Socket* sock, uint16_t port, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
	{
		if (sock->IsValid()){
			sockaddr_in target;
			target.sin_family = AF_INET;
			target.sin_port = htons(port);

			// If all IP octets are 0, bind to INADDR_ANY (all interfaces)
			// Otherwise, bind to the specified IP address
			if (ip0 == 0 && ip1 == 0 && ip2 == 0 && ip3 == 0)
			{
				target.sin_addr.s_addr = htonl(INADDR_ANY);
			}
			else
			{
				in_addr_union address;
				address.S_un_b.s_b1 = ip0;
				address.S_un_b.s_b2 = ip1;
				address.S_un_b.s_b3 = ip2;
				address.S_un_b.s_b4 = ip3;
				target.sin_addr.s_addr = address.S_addr;
			}

			auto socketinternal = to_internal(sock);

			int result = bind(socketinternal->handle, (struct sockaddr *)&target , sizeof(target));
			if (result < 0)
			{
				wi::backlog::post("wi::network_Linux error in ListenPort: (Error Code: " + std::to_string(errno) + ") " + std::string(strerror(errno)));
				return false;
			}

			return true;
		}
		return false;
	}

	bool CanReceive(const Socket* sock, long timeout_microseconds)
	{
		if (sock->IsValid()){
			auto socketinternal = to_internal(sock);

			fd_set readfds;
			FD_ZERO(&readfds);
			FD_SET(socketinternal->handle, &readfds);
			timeval timeout;
			timeout.tv_sec = 0;
			timeout.tv_usec = timeout_microseconds;

			// First arg to select() on Linux must be highest fd + 1, NOT 0!
			int result = select(socketinternal->handle + 1, &readfds, NULL, NULL, &timeout);
			if (result < 0)
			{
				// Socket error - log it but don't crash
				wi::backlog::post("wi::network_Linux error in CanReceive: (Error Code: " + std::to_string(errno) + ") " + std::string(strerror(errno)), wi::backlog::LogLevel::Warning);
				return false;
			}

			return FD_ISSET(socketinternal->handle, &readfds);
		}
		return false;
	}

	bool Receive(const Socket* sock, Connection* connection, void* data, size_t dataSize)
	{
		if (sock->IsValid()){
			auto socketinternal = to_internal(sock);

			sockaddr_in sender;
			int targetsize = sizeof(sender);
			int result = recvfrom(socketinternal->handle, (char*)data, (int)dataSize, 0, (sockaddr*)& sender, (socklen_t*)&targetsize);
			if (result < 0)
			{
				// Socket error - log it but don't crash
				// EAGAIN/EWOULDBLOCK means no data available (non-blocking socket)
				if (errno != EAGAIN && errno != EWOULDBLOCK)
				{
					wi::backlog::post("wi::network_Linux error in Receive: (Error Code: " + std::to_string(errno) + ") " + std::string(strerror(errno)), wi::backlog::LogLevel::Warning);
				}
				return false;
			}

			connection->port = htons(sender.sin_port); // reverse byte order from network to host
			connection->bytesReceived = result;  // Store actual bytes received
			in_addr_union address;
			address.S_addr = sender.sin_addr.s_addr;
			connection->ipaddress[0] = address.S_un_b.s_b1;
			connection->ipaddress[1] = address.S_un_b.s_b2;
			connection->ipaddress[2] = address.S_un_b.s_b3;
			connection->ipaddress[3] = address.S_un_b.s_b4;

			return true;
		}
		return false;
	}
}

#endif // LINUX
