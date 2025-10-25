#pragma once
#include "CommonInclude.h"
#include "wiNetwork.h"

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include <queue>
#include <mutex>

namespace wi::osc
{
	// OSCMessage represents a parsed OSC message
	//	Contains the address pattern and typed argument values
	struct OSCMessage
	{
		std::string address;				// OSC address pattern (e.g., "/ch/1")
		std::vector<float> floats;			// Float arguments (type 'f')
		std::vector<double> doubles;		// Double arguments (type 'd')
		std::vector<int32_t> int32s;		// 32-bit integer arguments (type 'i')
		std::vector<int64_t> int64s;		// 64-bit integer arguments (type 'h')
		std::vector<std::string> strings;	// String arguments (type 's')

		// Helper accessors with bounds checking
		float GetFloat(size_t index = 0) const;
		double GetDouble(size_t index = 0) const;
		int32_t GetInt32(size_t index = 0) const;
		int64_t GetInt64(size_t index = 0) const;
		const char* GetString(size_t index = 0) const;

		// Counts
		inline size_t GetFloatCount() const { return floats.size(); }
		inline size_t GetDoubleCount() const { return doubles.size(); }
		inline size_t GetInt32Count() const { return int32s.size(); }
		inline size_t GetInt64Count() const { return int64s.size(); }
		inline size_t GetStringCount() const { return strings.size(); }
	};

	// OSCReceiver listens for incoming OSC messages on a specified port
	//	Usage:
	//		1. Call Initialize() with desired port
	//		2. Optionally call SetChannelPath() to configure channel addressing
	//		3. Call SetCallback() to register message handlers for specific addresses
	//		4. Call Update() each frame to poll for messages and invoke callbacks
	//	Alternative:
	//		1-2. Same as above
	//		3-4. Use HasMessages() and PopMessage() to manually process queued messages
	class OSCReceiver
	{
	public:
		OSCReceiver();
		~OSCReceiver();

		// Initialize the receiver and bind to specified port and IP address
		//	port	: UDP port to listen on (default 7000)
		//	ip0-ip3	: IP address octets to bind to (default: 0,0,0,0 = all interfaces)
		//	          Examples: 127,0,0,1 for localhost only, 192,168,1,100 for specific interface
		//	returns : true if successful, false on error
		bool Initialize(uint16_t port = 7000, uint8_t ip0 = 0, uint8_t ip1 = 0, uint8_t ip2 = 0, uint8_t ip3 = 0);

		// Update receiver - polls socket and processes incoming messages
		//	Call this once per frame to receive and dispatch messages
		//	Messages will be routed to callbacks or queued for manual processing
		void Update();

		// Shutdown receiver and close socket
		void Shutdown();

		// Check if receiver is initialized and ready
		inline bool IsValid() const { return socket.IsValid(); }

		// Set custom channel path template (e.g., "/ch/%d" or "/fader/%d")
		//	template_path	: Path template with %d for channel number
		//	This affects GetChannelPath() but doesn't change callback registration
		void SetChannelPath(const std::string& template_path);

		// Get the channel path for a given channel index (1-8)
		//	index	: Channel index (1-8)
		//	returns : Formatted channel path (e.g., "/ch/1")
		std::string GetChannelPath(int index) const;

		// Set maximum number of messages to process per Update() call
		//	max_messages : Maximum messages per frame (default: 16)
		//	Higher values reduce latency but increase frame time
		void SetMaxMessagesPerFrame(int max_messages);

		// Message callback function type
		//	Receives const reference to parsed OSC message
		using MessageCallback = std::function<void(const OSCMessage&)>;

		// Register a callback for messages matching a specific address
		//	address		: OSC address pattern to match (e.g., "/ch/1")
		//	callback	: Function to call when matching message arrives
		//	Note: Currently only exact address matching is supported (no wildcards)
		void SetCallback(const std::string& address, MessageCallback callback);

		// Remove callback for specified address
		void RemoveCallback(const std::string& address);

		// Clear all registered callbacks
		void ClearCallbacks();

		// Queue-based API (alternative to callbacks)
		//	Check if any messages are queued for processing
		bool HasMessages() const;

		// Pop oldest message from queue
		//	Returns : OSCMessage (check address to verify validity)
		OSCMessage PopMessage();

		// Get number of queued messages
		size_t GetMessageCount() const;

		// Clear message queue
		void ClearMessages();

	private:
		wi::network::Socket socket;
		char buffer[2048];	// Buffer for receiving UDP packets

		std::unordered_map<std::string, MessageCallback> callbacks;
		std::queue<OSCMessage> message_queue;
		mutable std::mutex queue_mutex;

		std::string channel_path_template = "/ch/%d";	// Default channel path template
		int max_messages_per_frame = 16;				// Maximum messages to process per Update()

		// Internal helper to parse raw OSC packet into OSCMessage
		bool ParseOSCPacket(const char* data, int length, OSCMessage& out_message);

		// Internal helper to route message to callback or queue
		void RouteMessage(const OSCMessage& message);
	};

	// OSCTransmitter sends OSC messages to remote destinations
	//	Usage:
	//		1. Call Initialize() to create socket
	//		2. Use SendFloat/SendInt/SendMessage to transmit data
	class OSCTransmitter
	{
	public:
		OSCTransmitter();
		~OSCTransmitter();

		// Initialize transmitter socket
		//	returns : true if successful, false on error
		bool Initialize();

		// Shutdown transmitter and close socket
		void Shutdown();

		// Check if transmitter is initialized and ready
		inline bool IsValid() const { return socket.IsValid(); }

		// Send a single float value
		//	address		: OSC address (e.g., "/volume")
		//	value		: Float value to send
		//	target		: Destination IP address and port
		//	returns		: true if sent successfully
		bool SendFloat(const char* address, float value, const wi::network::Connection& target);

		// Send a single double value
		bool SendDouble(const char* address, double value, const wi::network::Connection& target);

		// Send a single 32-bit integer
		bool SendInt32(const char* address, int32_t value, const wi::network::Connection& target);

		// Send a single 64-bit integer
		bool SendInt64(const char* address, int64_t value, const wi::network::Connection& target);

		// Send a string
		bool SendString(const char* address, const char* str, const wi::network::Connection& target);

		// Send multiple values with format string
		//	address		: OSC address
		//	format		: Type format string (e.g., "fis" = float, int, string)
		//				  Supported types: f=float, d=double, i=int32, h=int64, s=string
		//	...			: Variable arguments matching format string
		//	target		: Destination IP address and port
		//	returns		: true if sent successfully
		bool SendMessage(const char* address, const char* format, const wi::network::Connection& target, ...);

	private:
		wi::network::Socket socket;
		char buffer[2048];	// Buffer for formatting OSC messages
	};

	// Helper function to create a network connection from IP components
	//	Usage: auto target = CreateConnection(127, 0, 0, 1, 7001);
	inline wi::network::Connection CreateConnection(uint8_t ip0, uint8_t ip1, uint8_t ip2, uint8_t ip3, uint16_t port)
	{
		wi::network::Connection conn;
		conn.ipaddress[0] = ip0;
		conn.ipaddress[1] = ip1;
		conn.ipaddress[2] = ip2;
		conn.ipaddress[3] = ip3;
		conn.port = port;
		return conn;
	}
}
