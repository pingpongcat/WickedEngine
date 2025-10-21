#include "wiOSC_BindLua.h"
#include "wiBacklog.h"

namespace wi::lua
{
	// Global storage for Lua callbacks, keyed by C++ OSCReceiver pointer
	// This avoids capturing 'this' in lambdas which becomes invalid when Luna moves objects
	struct ReceiverCallbackData {
		std::vector<OSCReceiver_BindLua::PendingCallback> pending_callbacks;
		std::unordered_map<std::string, int> lua_callbacks;  // address -> funcRef
	};
	static std::unordered_map<void*, ReceiverCallbackData> g_receiver_callbacks;
	// ====================================================================
	// OSCMessage_BindLua Implementation
	// ====================================================================

	Luna<OSCMessage_BindLua>::FunctionType OSCMessage_BindLua::methods[] = {
		lunamethod(OSCMessage_BindLua, GetAddress),
		lunamethod(OSCMessage_BindLua, GetFloat),
		lunamethod(OSCMessage_BindLua, GetDouble),
		lunamethod(OSCMessage_BindLua, GetInt32),
		lunamethod(OSCMessage_BindLua, GetInt64),
		lunamethod(OSCMessage_BindLua, GetString),
		lunamethod(OSCMessage_BindLua, GetFloatCount),
		lunamethod(OSCMessage_BindLua, GetDoubleCount),
		lunamethod(OSCMessage_BindLua, GetInt32Count),
		lunamethod(OSCMessage_BindLua, GetInt64Count),
		lunamethod(OSCMessage_BindLua, GetStringCount),
		{ NULL, NULL }
	};

	Luna<OSCMessage_BindLua>::PropertyType OSCMessage_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int OSCMessage_BindLua::GetAddress(lua_State* L)
	{
		wi::lua::SSetString(L, message.address.c_str());
		return 1;
	}

	int OSCMessage_BindLua::GetFloat(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int index = 0;
		if (argc > 0)
		{
			index = wi::lua::SGetInt(L, 1);
		}
		wi::lua::SSetFloat(L, message.GetFloat(index));
		return 1;
	}

	int OSCMessage_BindLua::GetDouble(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int index = 0;
		if (argc > 0)
		{
			index = wi::lua::SGetInt(L, 1);
		}
		wi::lua::SSetDouble(L, message.GetDouble(index));
		return 1;
	}

	int OSCMessage_BindLua::GetInt32(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int index = 0;
		if (argc > 0)
		{
			index = wi::lua::SGetInt(L, 1);
		}
		wi::lua::SSetInt(L, message.GetInt32(index));
		return 1;
	}

	int OSCMessage_BindLua::GetInt64(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int index = 0;
		if (argc > 0)
		{
			index = wi::lua::SGetInt(L, 1);
		}
		wi::lua::SSetLongLong(L, message.GetInt64(index));
		return 1;
	}

	int OSCMessage_BindLua::GetString(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		int index = 0;
		if (argc > 0)
		{
			index = wi::lua::SGetInt(L, 1);
		}
		wi::lua::SSetString(L, message.GetString(index));
		return 1;
	}

	int OSCMessage_BindLua::GetFloatCount(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)message.GetFloatCount());
		return 1;
	}

	int OSCMessage_BindLua::GetDoubleCount(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)message.GetDoubleCount());
		return 1;
	}

	int OSCMessage_BindLua::GetInt32Count(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)message.GetInt32Count());
		return 1;
	}

	int OSCMessage_BindLua::GetInt64Count(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)message.GetInt64Count());
		return 1;
	}

	int OSCMessage_BindLua::GetStringCount(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)message.GetStringCount());
		return 1;
	}

	void OSCMessage_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<OSCMessage_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

	// ====================================================================
	// OSCReceiver_BindLua Implementation
	// ====================================================================

	Luna<OSCReceiver_BindLua>::FunctionType OSCReceiver_BindLua::methods[] = {
		lunamethod(OSCReceiver_BindLua, Initialize),
		lunamethod(OSCReceiver_BindLua, Update),
		lunamethod(OSCReceiver_BindLua, Shutdown),
		lunamethod(OSCReceiver_BindLua, IsValid),
		lunamethod(OSCReceiver_BindLua, SetCallback),
		lunamethod(OSCReceiver_BindLua, RemoveCallback),
		lunamethod(OSCReceiver_BindLua, ClearCallbacks),
		lunamethod(OSCReceiver_BindLua, HasMessages),
		lunamethod(OSCReceiver_BindLua, PopMessage),
		lunamethod(OSCReceiver_BindLua, GetMessageCount),
		lunamethod(OSCReceiver_BindLua, ClearMessages),
		{ NULL, NULL }
	};

	Luna<OSCReceiver_BindLua>::PropertyType OSCReceiver_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int OSCReceiver_BindLua::Initialize(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		uint16_t port = 7000;  // Default port
		if (argc > 0)
		{
			// When using : syntax, self is at index 1, so arguments start at index 2
			port = (uint16_t)wi::lua::SGetInt(L, 2);
		}
		bool success = receiver.Initialize(port);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCReceiver_BindLua::Update(lua_State* L)
	{
		// Call the C++ receiver's Update() - this will queue callbacks
		receiver.Update();

		// Get the global callback data for this receiver
		void* receiver_ptr = &receiver;

		auto it = g_receiver_callbacks.find(receiver_ptr);
		if (it == g_receiver_callbacks.end())
		{
			return 0;
		}

		// Now process all pending Lua callbacks
		for (const auto& pending : it->second.pending_callbacks)
		{
			// Push the stored function onto the stack
			lua_rawgeti(L, LUA_REGISTRYINDEX, pending.funcRef);

			// Create OSCMessage_BindLua wrapper and push to Lua
			Luna<OSCMessage_BindLua>::push(L, pending.message);

			// Call the function with 1 argument
			if (lua_pcall(L, 1, 0, 0) != 0)
			{
				const char* error = lua_tostring(L, -1);
				wi::backlog::post(std::string("[OSC] Lua callback error: ") + error, wi::backlog::LogLevel::Error);
				lua_pop(L, 1);  // Pop error message
			}
		}

		// Clear pending callbacks for next frame
		it->second.pending_callbacks.clear();

		return 0;
	}

	int OSCReceiver_BindLua::Shutdown(lua_State* L)
	{
		receiver.Shutdown();

		// Clean up global callback data for this receiver
		void* receiver_ptr = &receiver;
		auto receiver_it = g_receiver_callbacks.find(receiver_ptr);
		if (receiver_it != g_receiver_callbacks.end())
		{
			// Unreference all Lua functions
			for (const auto& pair : receiver_it->second.lua_callbacks)
			{
				luaL_unref(L, LUA_REGISTRYINDEX, pair.second);
			}
			// Remove from global map
			g_receiver_callbacks.erase(receiver_it);
		}

		return 0;
	}

	int OSCReceiver_BindLua::IsValid(lua_State* L)
	{
		wi::lua::SSetBool(L, receiver.IsValid());
		return 1;
	}

	int OSCReceiver_BindLua::SetCallback(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 2)
		{
			wi::lua::SError(L, "SetCallback(string address, function callback) requires 2 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);

		if (!lua_isfunction(L, 3))
		{
			wi::lua::SError(L, "SetCallback(string address, function callback) second argument must be a function!");
			return 0;
		}

		// Store the Lua function in the registry and get a reference
		lua_pushvalue(L, 3);  // Push the function to the top
		int funcRef = luaL_ref(L, LUA_REGISTRYINDEX);  // Store in registry and get reference

		// Get the global callback data for this receiver (keyed by C++ receiver pointer)
		void* receiver_ptr = &receiver;
		ReceiverCallbackData& data = g_receiver_callbacks[receiver_ptr];
		data.lua_callbacks[address] = funcRef;

		// Create a C++ callback that queues the message for later Lua processing
		// Capture receiver_ptr, not 'this' (which can become invalid when Luna moves objects)
		receiver.SetCallback(address, [receiver_ptr, address](const wi::osc::OSCMessage& msg) {
			// Find the callback data for this receiver
			auto receiver_it = g_receiver_callbacks.find(receiver_ptr);
			if (receiver_it == g_receiver_callbacks.end())
				return;

			// Find the funcRef for this address
			auto it = receiver_it->second.lua_callbacks.find(address);
			if (it != receiver_it->second.lua_callbacks.end())
			{
				// Queue the callback to be invoked after Update() returns
				receiver_it->second.pending_callbacks.push_back({ it->second, msg });
			}
		});

		return 0;
	}

	int OSCReceiver_BindLua::RemoveCallback(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 1)
		{
			wi::lua::SError(L, "RemoveCallback(string address) requires 1 argument!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);

		// Remove from C++ receiver
		receiver.RemoveCallback(address);

		// Remove from global Lua callbacks map and unreference the function
		void* receiver_ptr = &receiver;
		auto receiver_it = g_receiver_callbacks.find(receiver_ptr);
		if (receiver_it != g_receiver_callbacks.end())
		{
			auto it = receiver_it->second.lua_callbacks.find(address);
			if (it != receiver_it->second.lua_callbacks.end())
			{
				luaL_unref(L, LUA_REGISTRYINDEX, it->second);
				receiver_it->second.lua_callbacks.erase(it);
			}
		}

		return 0;
	}

	int OSCReceiver_BindLua::ClearCallbacks(lua_State* L)
	{
		receiver.ClearCallbacks();

		// Unreference all Lua functions from global map
		void* receiver_ptr = &receiver;
		auto receiver_it = g_receiver_callbacks.find(receiver_ptr);
		if (receiver_it != g_receiver_callbacks.end())
		{
			for (const auto& pair : receiver_it->second.lua_callbacks)
			{
				luaL_unref(L, LUA_REGISTRYINDEX, pair.second);
			}
			receiver_it->second.lua_callbacks.clear();
		}

		return 0;
	}

	int OSCReceiver_BindLua::HasMessages(lua_State* L)
	{
		wi::lua::SSetBool(L, receiver.HasMessages());
		return 1;
	}

	int OSCReceiver_BindLua::PopMessage(lua_State* L)
	{
		wi::osc::OSCMessage msg = receiver.PopMessage();
		Luna<OSCMessage_BindLua>::push(L, msg);
		return 1;
	}

	int OSCReceiver_BindLua::GetMessageCount(lua_State* L)
	{
		wi::lua::SSetInt(L, (int)receiver.GetMessageCount());
		return 1;
	}

	int OSCReceiver_BindLua::ClearMessages(lua_State* L)
	{
		receiver.ClearMessages();
		return 0;
	}

	void OSCReceiver_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<OSCReceiver_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

	// ====================================================================
	// OSCTransmitter_BindLua Implementation
	// ====================================================================

	Luna<OSCTransmitter_BindLua>::FunctionType OSCTransmitter_BindLua::methods[] = {
		lunamethod(OSCTransmitter_BindLua, Initialize),
		lunamethod(OSCTransmitter_BindLua, Shutdown),
		lunamethod(OSCTransmitter_BindLua, IsValid),
		lunamethod(OSCTransmitter_BindLua, SendFloat),
		lunamethod(OSCTransmitter_BindLua, SendDouble),
		lunamethod(OSCTransmitter_BindLua, SendInt32),
		lunamethod(OSCTransmitter_BindLua, SendInt64),
		lunamethod(OSCTransmitter_BindLua, SendString),
		{ NULL, NULL }
	};

	Luna<OSCTransmitter_BindLua>::PropertyType OSCTransmitter_BindLua::properties[] = {
		{ NULL, NULL }
	};

	int OSCTransmitter_BindLua::Initialize(lua_State* L)
	{
		bool success = transmitter.Initialize();
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCTransmitter_BindLua::Shutdown(lua_State* L)
	{
		transmitter.Shutdown();
		return 0;
	}

	int OSCTransmitter_BindLua::IsValid(lua_State* L)
	{
		wi::lua::SSetBool(L, transmitter.IsValid());
		return 1;
	}

	int OSCTransmitter_BindLua::SendFloat(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 3)
		{
			wi::lua::SError(L, "SendFloat(string address, float value, OSCConnection connection) requires 3 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);
		float value = wi::lua::SGetFloat(L, 3);
		OSCConnection_BindLua* conn = Luna<OSCConnection_BindLua>::lightcheck(L, 4);

		if (conn == nullptr)
		{
			wi::lua::SError(L, "SendFloat() third argument must be an OSCConnection!");
			return 0;
		}

		bool success = transmitter.SendFloat(address.c_str(), value, conn->connection);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCTransmitter_BindLua::SendDouble(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 3)
		{
			wi::lua::SError(L, "SendDouble(string address, double value, OSCConnection connection) requires 3 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);
		double value = wi::lua::SGetDouble(L, 3);
		OSCConnection_BindLua* conn = Luna<OSCConnection_BindLua>::lightcheck(L, 4);

		if (conn == nullptr)
		{
			wi::lua::SError(L, "SendDouble() third argument must be an OSCConnection!");
			return 0;
		}

		bool success = transmitter.SendDouble(address.c_str(), value, conn->connection);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCTransmitter_BindLua::SendInt32(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 3)
		{
			wi::lua::SError(L, "SendInt32(string address, int value, OSCConnection connection) requires 3 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);
		int32_t value = wi::lua::SGetInt(L, 3);
		OSCConnection_BindLua* conn = Luna<OSCConnection_BindLua>::lightcheck(L, 4);

		if (conn == nullptr)
		{
			wi::lua::SError(L, "SendInt32() third argument must be an OSCConnection!");
			return 0;
		}

		bool success = transmitter.SendInt32(address.c_str(), value, conn->connection);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCTransmitter_BindLua::SendInt64(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 3)
		{
			wi::lua::SError(L, "SendInt64(string address, int value, OSCConnection connection) requires 3 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);
		int64_t value = wi::lua::SGetLongLong(L, 3);
		OSCConnection_BindLua* conn = Luna<OSCConnection_BindLua>::lightcheck(L, 4);

		if (conn == nullptr)
		{
			wi::lua::SError(L, "SendInt64() third argument must be an OSCConnection!");
			return 0;
		}

		bool success = transmitter.SendInt64(address.c_str(), value, conn->connection);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	int OSCTransmitter_BindLua::SendString(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 3)
		{
			wi::lua::SError(L, "SendString(string address, string value, OSCConnection connection) requires 3 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		std::string address = wi::lua::SGetString(L, 2);
		std::string value = wi::lua::SGetString(L, 3);
		OSCConnection_BindLua* conn = Luna<OSCConnection_BindLua>::lightcheck(L, 4);

		if (conn == nullptr)
		{
			wi::lua::SError(L, "SendString() third argument must be an OSCConnection!");
			return 0;
		}

		bool success = transmitter.SendString(address.c_str(), value.c_str(), conn->connection);
		wi::lua::SSetBool(L, success);
		return 1;
	}

	void OSCTransmitter_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<OSCTransmitter_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

	// ====================================================================
	// OSCConnection_BindLua Implementation
	// ====================================================================

	Luna<OSCConnection_BindLua>::FunctionType OSCConnection_BindLua::methods[] = {
		lunamethod(OSCConnection_BindLua, SetIP),
		lunamethod(OSCConnection_BindLua, SetPort),
		lunamethod(OSCConnection_BindLua, GetIP),
		lunamethod(OSCConnection_BindLua, GetPort),
		{ NULL, NULL }
	};

	Luna<OSCConnection_BindLua>::PropertyType OSCConnection_BindLua::properties[] = {
		{ NULL, NULL }
	};

	OSCConnection_BindLua::OSCConnection_BindLua(lua_State* L)
	{
		// Initialize with default localhost:7001
		connection.ipaddress = { 127, 0, 0, 1 };
		connection.port = 7001;
	}

	int OSCConnection_BindLua::SetIP(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 4)
		{
			wi::lua::SError(L, "SetIP(int ip0, int ip1, int ip2, int ip3) requires 4 arguments!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		connection.ipaddress[0] = (uint8_t)wi::lua::SGetInt(L, 2);
		connection.ipaddress[1] = (uint8_t)wi::lua::SGetInt(L, 3);
		connection.ipaddress[2] = (uint8_t)wi::lua::SGetInt(L, 4);
		connection.ipaddress[3] = (uint8_t)wi::lua::SGetInt(L, 5);

		return 0;
	}

	int OSCConnection_BindLua::SetPort(lua_State* L)
	{
		int argc = wi::lua::SGetArgCount(L);
		if (argc < 1)
		{
			wi::lua::SError(L, "SetPort(int port) requires 1 argument!");
			return 0;
		}

		// When using : syntax, self is at index 1, so arguments start at index 2
		connection.port = (uint16_t)wi::lua::SGetInt(L, 2);
		return 0;
	}

	int OSCConnection_BindLua::GetIP(lua_State* L)
	{
		wi::lua::SSetInt(L, connection.ipaddress[0]);
		wi::lua::SSetInt(L, connection.ipaddress[1]);
		wi::lua::SSetInt(L, connection.ipaddress[2]);
		wi::lua::SSetInt(L, connection.ipaddress[3]);
		return 4;
	}

	int OSCConnection_BindLua::GetPort(lua_State* L)
	{
		wi::lua::SSetInt(L, connection.port);
		return 1;
	}

	void OSCConnection_BindLua::Bind()
	{
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<OSCConnection_BindLua>::Register(wi::lua::GetLuaState());
		}
	}

	// ====================================================================
	// Main OSC Bind Function
	// ====================================================================

	namespace osc
	{
		void Bind()
		{
			static bool initialized = false;
			if (!initialized)
			{
				initialized = true;

				wi::osc::Initialize();

				OSCMessage_BindLua::Bind();
				OSCReceiver_BindLua::Bind();
				OSCTransmitter_BindLua::Bind();
				OSCConnection_BindLua::Bind();

				wi::backlog::post("wi::lua::osc Initialized");
			}
		}
	}
}
