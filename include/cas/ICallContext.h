#pragma once

#include "cas/Common.h"

namespace cas
{
	class IFunction;
	class IGlobal;
	class IObject;
	class IModule;

	class ICallContext
	{
	public:
		virtual IObject* CreateInstance(IType* type) = 0;
		virtual vector<string>& GetAsserts() = 0;
		virtual std::pair<cstring, int> GetCurrentLocation() = 0;
		virtual std::pair<IFunction*, IObject*> GetEntryPoint() = 0;
		virtual cstring GetException() = 0;
		virtual IObject* GetGlobal(IGlobal* global) = 0;
		virtual IModule* GetModule() = 0;
		virtual cstring GetName() = 0;
		virtual Value GetReturnValue() = 0;
		virtual void PushValue(const Value& val) = 0;
		virtual void Release() = 0;
		virtual bool Run() = 0;
		virtual bool SetEntryPoint(IFunction* func) = 0;
		virtual bool SetEntryPointObj(IFunction* func, IObject* obj) = 0;
		virtual void SetName(cstring name) = 0;

		template<typename... Args>
		IObject* CreateInstance(IType* type, const Args&... args)
		{
			PushValues(args...);
			return CreateInstance(type);
		}

		template<typename T>
		void PushValues(T arg)
		{
			PushValue(arg);
		}
		template<typename T, typename... Args>
		void PushValues(T arg, const Args&... args)
		{
			PushValue(arg);
			PushValues(args...);
		}

		template<typename... Args>
		bool SetEntryPoint(IFunction* func, const Args&... args)
		{
			PushValues(args...);
			return SetEntryPoint(func);
		}
		template<typename... Args>
		bool SetEntryPointObj(IFunction* func, IObject* obj, const Args&... args)
		{
			PushValues(args...);
			return SetEntryPointObj(func, obj);
		}

	protected:
		~ICallContext() {}
	};
}
