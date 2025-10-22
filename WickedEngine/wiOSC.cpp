#include "wiOSC.h"
#include "wiBacklog.h"

extern "C" {
#include "tinyosc.h"
}

#include <cstdarg>
#include <cstring>

namespace wi::osc
{
	void Initialize()
	{
		wi::backlog::post("wi::osc Initialized");
	}

	void Shutdown()
	{
		wi::backlog::post("wi::osc Shutdown");
	}

	// ====================================================================
	// OSCMessage Implementation
	// ====================================================================

	float OSCMessage::GetFloat(size_t index) const
	{
		if (index < floats.size())
			return floats[index];
		return 0.0f;
	}

	double OSCMessage::GetDouble(size_t index) const
	{
		if (index < doubles.size())
			return doubles[index];
		return 0.0;
	}

	int32_t OSCMessage::GetInt32(size_t index) const
	{
		if (index < int32s.size())
			return int32s[index];
		return 0;
	}

	int64_t OSCMessage::GetInt64(size_t index) const
	{
		if (index < int64s.size())
			return int64s[index];
		return 0;
	}

	const char* OSCMessage::GetString(size_t index) const
	{
		if (index < strings.size())
			return strings[index].c_str();
		return "";
	}

	// ====================================================================
	// OSCReceiver Implementation
	// ====================================================================

	OSCReceiver::OSCReceiver()
	{
		memset(buffer, 0, sizeof(buffer));
	}

	OSCReceiver::~OSCReceiver()
	{
		Shutdown();
	}

	bool OSCReceiver::Initialize(uint16_t port, uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3)
	{
		// Create UDP socket
		if (!wi::network::CreateSocket(&socket))
		{
			wi::backlog::post("wi::osc::OSCReceiver - Failed to create socket", wi::backlog::LogLevel::Error);
			return false;
		}

		// Bind to specified port and IP address
		if (!wi::network::ListenPort(&socket, port, ip0, ip1, ip2, ip3))
		{
			wi::backlog::post("wi::osc::OSCReceiver - Failed to bind to port " + std::to_string(port), wi::backlog::LogLevel::Error);
			return false;
		}

		// Log binding information
		if (ip0 == 0 && ip1 == 0 && ip2 == 0 && ip3 == 0)
		{
			wi::backlog::post("wi::osc::OSCReceiver - Listening on all interfaces, port " + std::to_string(port));
		}
		else
		{
			wi::backlog::post("wi::osc::OSCReceiver - Listening on " +
				std::to_string(ip0) + "." + std::to_string(ip1) + "." +
				std::to_string(ip2) + "." + std::to_string(ip3) + ":" + std::to_string(port));
		}
		return true;
	}

	void OSCReceiver::Update()
	{
		if (!socket.IsValid())
		{
			return;
		}

		// Poll socket with minimal timeout (non-blocking)
		// Process at most 8 messages per frame to avoid blocking the main thread
		int messages_processed = 0;
		const int max_messages_per_frame = 8;

		while (messages_processed < max_messages_per_frame && wi::network::CanReceive(&socket, 1))
		{
			wi::network::Connection sender;
			if (wi::network::Receive(&socket, &sender, buffer, sizeof(buffer)))
			{
				messages_processed++;
				OSCMessage message;

				// Check if it's a bundle or single message
				if (tosc_isBundle(buffer))
				{
					// Parse bundle with actual received byte count
					tosc_bundle bundle;
					tosc_parseBundle(&bundle, buffer, sender.bytesReceived);

					tosc_message osc;
					while (tosc_getNextMessage(&bundle, &osc))
					{
						if (ParseOSCPacket(osc.buffer, osc.len, message))
						{
							// Route to callback if registered
							auto it = callbacks.find(message.address);
							if (it != callbacks.end())
							{
								it->second(message);
							}
							else
							{
								// Queue for manual processing
								std::lock_guard<std::mutex> lock(queue_mutex);
								message_queue.push(message);
							}
						}
					}
				}
				else
				{
					// Parse single message with actual received byte count
					if (ParseOSCPacket(buffer, sender.bytesReceived, message))
					{
						// Route to callback if registered
						auto it = callbacks.find(message.address);
						if (it != callbacks.end())
						{
							it->second(message);
						}
						else
						{
							// Queue for manual processing
							std::lock_guard<std::mutex> lock(queue_mutex);
							message_queue.push(message);
						}
					}
				}
			}
		}
	}

	void OSCReceiver::Shutdown()
	{
		socket.internal_state.reset();
		ClearCallbacks();
		ClearMessages();
	}

	void OSCReceiver::SetChannelPath(const std::string& template_path)
	{
		channel_path_template = template_path;
	}

	std::string OSCReceiver::GetChannelPath(int index) const
	{
		char buffer[256];
		snprintf(buffer, sizeof(buffer), channel_path_template.c_str(), index);
		return std::string(buffer);
	}

	void OSCReceiver::SetCallback(const std::string& address, MessageCallback callback)
	{
		callbacks[address] = callback;
	}

	void OSCReceiver::RemoveCallback(const std::string& address)
	{
		callbacks.erase(address);
	}

	void OSCReceiver::ClearCallbacks()
	{
		callbacks.clear();
	}

	bool OSCReceiver::HasMessages() const
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		return !message_queue.empty();
	}

	OSCMessage OSCReceiver::PopMessage()
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		if (message_queue.empty())
		{
			return OSCMessage(); // Return empty message
		}

		OSCMessage msg = message_queue.front();
		message_queue.pop();
		return msg;
	}

	size_t OSCReceiver::GetMessageCount() const
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		return message_queue.size();
	}

	void OSCReceiver::ClearMessages()
	{
		std::lock_guard<std::mutex> lock(queue_mutex);
		while (!message_queue.empty())
			message_queue.pop();
	}

	bool OSCReceiver::ParseOSCPacket(const char* data, int length, OSCMessage& out_message)
	{
		tosc_message osc;
		if (tosc_parseMessage(&osc, (char*)data, length) != 0)
		{
			wi::backlog::post("wi::osc::OSCReceiver - Failed to parse OSC message", wi::backlog::LogLevel::Warning);
			return false;
		}

		// Extract address
		out_message.address = tosc_getAddress(&osc);

		// Clear previous data
		out_message.floats.clear();
		out_message.doubles.clear();
		out_message.int32s.clear();
		out_message.int64s.clear();
		out_message.strings.clear();

		// Parse arguments based on format string
		const char* format = tosc_getFormat(&osc);
		for (int i = 0; format[i] != '\0'; i++)
		{
			switch (format[i])
			{
			case 'f':
				out_message.floats.push_back(tosc_getNextFloat(&osc));
				break;
			case 'd':
				out_message.doubles.push_back(tosc_getNextDouble(&osc));
				break;
			case 'i':
				out_message.int32s.push_back(tosc_getNextInt32(&osc));
				break;
			case 'h':
				out_message.int64s.push_back(tosc_getNextInt64(&osc));
				break;
			case 's':
			{
				const char* str = tosc_getNextString(&osc);
				if (str != nullptr)
					out_message.strings.push_back(str);
				else
					out_message.strings.push_back("");
				break;
			}
			case 'T': // True - could store as bool, but skip for now
			case 'F': // False
			case 'N': // Nil
			case 'I': // Infinitum
			default:
				// Unsupported or special type - skip
				break;
			}
		}

		return true;
	}

	// ====================================================================
	// OSCTransmitter Implementation
	// ====================================================================

	OSCTransmitter::OSCTransmitter()
	{
		memset(buffer, 0, sizeof(buffer));
	}

	OSCTransmitter::~OSCTransmitter()
	{
		Shutdown();
	}

	bool OSCTransmitter::Initialize()
	{
		if (!wi::network::CreateSocket(&socket))
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to create socket", wi::backlog::LogLevel::Error);
			return false;
		}

		wi::backlog::post("wi::osc::OSCTransmitter - Initialized");
		return true;
	}

	void OSCTransmitter::Shutdown()
	{
		socket.internal_state.reset();
	}

	bool OSCTransmitter::SendFloat(const char* address, float value, const wi::network::Connection& target)
	{
		if (!socket.IsValid())
			return false;

		int len = tosc_writeMessage(buffer, sizeof(buffer), address, "f", value);
		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format float message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}

	bool OSCTransmitter::SendDouble(const char* address, double value, const wi::network::Connection& target)
	{
		if (!socket.IsValid())
			return false;

		int len = tosc_writeMessage(buffer, sizeof(buffer), address, "d", value);
		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format double message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}

	bool OSCTransmitter::SendInt32(const char* address, int32_t value, const wi::network::Connection& target)
	{
		if (!socket.IsValid())
			return false;

		int len = tosc_writeMessage(buffer, sizeof(buffer), address, "i", value);
		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format int32 message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}

	bool OSCTransmitter::SendInt64(const char* address, int64_t value, const wi::network::Connection& target)
	{
		if (!socket.IsValid())
			return false;

		int len = tosc_writeMessage(buffer, sizeof(buffer), address, "h", value);
		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format int64 message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}

	bool OSCTransmitter::SendString(const char* address, const char* str, const wi::network::Connection& target)
	{
		if (!socket.IsValid())
			return false;

		int len = tosc_writeMessage(buffer, sizeof(buffer), address, "s", str);
		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format string message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}

	bool OSCTransmitter::SendMessage(const char* address, const char* format, const wi::network::Connection& target, ...)
	{
		if (!socket.IsValid())
			return false;

		va_list args;
		va_start(args, target);

		// Build the message using tinyosc's variadic function
		// Note: This is a simplified version - full implementation would need to handle all types
		int len = 0;

		// Simple format parsing - handle common cases
		if (strcmp(format, "f") == 0)
		{
			float f = (float)va_arg(args, double); // float promotes to double in varargs
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, f);
		}
		else if (strcmp(format, "i") == 0)
		{
			int32_t i = va_arg(args, int32_t);
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, i);
		}
		else if (strcmp(format, "s") == 0)
		{
			const char* s = va_arg(args, const char*);
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, s);
		}
		else if (strcmp(format, "ff") == 0)
		{
			float f1 = (float)va_arg(args, double);
			float f2 = (float)va_arg(args, double);
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, f1, f2);
		}
		else if (strcmp(format, "fff") == 0)
		{
			float f1 = (float)va_arg(args, double);
			float f2 = (float)va_arg(args, double);
			float f3 = (float)va_arg(args, double);
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, f1, f2, f3);
		}
		else if (strcmp(format, "fis") == 0)
		{
			float f = (float)va_arg(args, double);
			int32_t i = va_arg(args, int32_t);
			const char* s = va_arg(args, const char*);
			len = tosc_writeMessage(buffer, sizeof(buffer), address, format, f, i, s);
		}
		else
		{
			// For other formats, log a warning
			wi::backlog::post("wi::osc::OSCTransmitter - Unsupported format string: " + std::string(format), wi::backlog::LogLevel::Warning);
			va_end(args);
			return false;
		}

		va_end(args);

		if (len <= 0)
		{
			wi::backlog::post("wi::osc::OSCTransmitter - Failed to format message", wi::backlog::LogLevel::Warning);
			return false;
		}

		return wi::network::Send(&socket, &target, buffer, len);
	}
}
