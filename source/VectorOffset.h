#pragma once

// Pointer to vector element that allow resize
template<typename T>
struct VectorOffset
{
	VectorOffset(vector<T>& _data, uint offset) : data(&_data), offset(offset)
	{

	}

	T* operator -> ()
	{
		return &data->at(offset);
	}

	T& operator () ()
	{
		return data->at(offset);
	}

	vector<T>* data;
	uint offset;
};
