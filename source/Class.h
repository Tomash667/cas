#pragma  once

struct Type;

// --- Call context class instance
// For class created in script this Class have larger size and class data is stored in it, starting at adr
// for code class, adr points to class created in code
struct Class
{
#ifdef CHECK_LEAKS
	static vector<Class*> all_classes;
#endif

	int refs;
	Type* type;
	bool is_code;
#ifdef CHECK_LEAKS
	bool attached;
#endif
	int* adr; // must be last member in Class

	int* data()
	{
		return adr;
	}
	
	byte* data(uint offset)
	{
		return ((byte*)adr) + offset;
	}

	template<typename T>
	T& at(uint offset)
	{
		return *(T*)data(offset);
	}

	static Class* Create(Type* type);
	static Class* CreateCode(Type* type, int* real_class);
	static Class* Copy(Class* base);

	bool Release();
	void Delete();
#ifdef CHECK_LEAKS
	void Deattach();
#endif
};
