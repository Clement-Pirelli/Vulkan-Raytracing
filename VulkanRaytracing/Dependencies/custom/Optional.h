#pragma once

//optional for POD types
template <class T>
class Optional
{
public:

	Optional(){}
	Optional(T value) {
		setValue(value);
	}

	void setValue(T givenValue)
	{
		value = givenValue;
		set = true;
	}

	T getValue()
	{
		return value;
	}

	bool isSet()
	{
		return set;
	}

private:

	T value = {};
	bool set = false;

};