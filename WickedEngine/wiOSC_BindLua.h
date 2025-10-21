#pragma once
#include "wiLua.h"
#include "wiLuna.h"
#include "wiOSC.h"

namespace wi::lua
{
	// OSCMessage_BindLua - Wrapper for parsed OSC messages
	class OSCMessage_BindLua
	{
	public:
		wi::osc::OSCMessage message;

		inline static constexpr char className[] = "OSCMessage";
		static Luna<OSCMessage_BindLua>::FunctionType methods[];
		static Luna<OSCMessage_BindLua>::PropertyType properties[];

		OSCMessage_BindLua(lua_State* L) {}
		OSCMessage_BindLua(const wi::osc::OSCMessage& msg) : message(msg) {}

		int GetAddress(lua_State* L);
		int GetFloat(lua_State* L);
		int GetDouble(lua_State* L);
		int GetInt32(lua_State* L);
		int GetInt64(lua_State* L);
		int GetString(lua_State* L);
		int GetFloatCount(lua_State* L);
		int GetDoubleCount(lua_State* L);
		int GetInt32Count(lua_State* L);
		int GetInt64Count(lua_State* L);
		int GetStringCount(lua_State* L);

		static void Bind();
	};

	// OSCReceiver_BindLua - OSC message receiver
	class OSCReceiver_BindLua
	{
	public:
		wi::osc::OSCReceiver receiver;

		// Pending callback structure (used in global map, not as member)
		struct PendingCallback {
			int funcRef;
			wi::osc::OSCMessage message;
		};

		inline static constexpr char className[] = "OSCReceiver";
		static Luna<OSCReceiver_BindLua>::FunctionType methods[];
		static Luna<OSCReceiver_BindLua>::PropertyType properties[];

		OSCReceiver_BindLua(lua_State* L) {}
		~OSCReceiver_BindLua() {}

		int Initialize(lua_State* L);
		int Update(lua_State* L);
		int Shutdown(lua_State* L);
		int IsValid(lua_State* L);
		int SetCallback(lua_State* L);
		int RemoveCallback(lua_State* L);
		int ClearCallbacks(lua_State* L);
		int HasMessages(lua_State* L);
		int PopMessage(lua_State* L);
		int GetMessageCount(lua_State* L);
		int ClearMessages(lua_State* L);

		static void Bind();
	};

	// OSCTransmitter_BindLua - OSC message transmitter
	class OSCTransmitter_BindLua
	{
	public:
		wi::osc::OSCTransmitter transmitter;

		inline static constexpr char className[] = "OSCTransmitter";
		static Luna<OSCTransmitter_BindLua>::FunctionType methods[];
		static Luna<OSCTransmitter_BindLua>::PropertyType properties[];

		OSCTransmitter_BindLua(lua_State* L) {}
		~OSCTransmitter_BindLua() {}

		int Initialize(lua_State* L);
		int Shutdown(lua_State* L);
		int IsValid(lua_State* L);
		int SendFloat(lua_State* L);
		int SendDouble(lua_State* L);
		int SendInt32(lua_State* L);
		int SendInt64(lua_State* L);
		int SendString(lua_State* L);

		static void Bind();
	};

	// OSCConnection_BindLua - Network connection wrapper for Lua
	class OSCConnection_BindLua
	{
	public:
		wi::network::Connection connection;

		inline static constexpr char className[] = "OSCConnection";
		static Luna<OSCConnection_BindLua>::FunctionType methods[];
		static Luna<OSCConnection_BindLua>::PropertyType properties[];

		OSCConnection_BindLua(lua_State* L);
		~OSCConnection_BindLua() {}

		int SetIP(lua_State* L);
		int SetPort(lua_State* L);
		int GetIP(lua_State* L);
		int GetPort(lua_State* L);

		static void Bind();
	};

	namespace osc
	{
		void Bind();
	}
}
