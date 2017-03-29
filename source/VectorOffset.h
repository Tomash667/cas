#pragma once

// Pointer to vector element that allow resize
template<typename T>
struct VectorOffset
{
	VectorOffset(vector<T>& _data, uint offset) : data(&_data), offset(offset), raw(nullptr)
	{
	}
	VectorOffset(T* raw) : raw(raw)
	{
	}

	T* operator -> ()
	{
		if(raw)
			return raw;
		else
			return &data->at(offset);
	}

	T& operator () ()
	{
		if(raw)
			return *raw;
		else
			return data->at(offset);
	}

private:
	vector<T>* data;
	uint offset;
	T* raw;
};
