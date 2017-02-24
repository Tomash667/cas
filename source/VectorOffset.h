#pragma once

// Pointer to vector element that allow resize
template<typename T>
struct VectorOffset
{
	inline VectorOffset(vector<T>& _data, uint offset) : data(&_data), offset(offset)
	{

	}

	inline T* operator -> ()
	{
		return &data->at(offset);
	}

	inline T& operator () ()
	{
		return data->at(offset);
	}

	vector<T>* data;
	uint offset;
};
