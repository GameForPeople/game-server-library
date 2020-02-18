#pragma once

template <typename T>
class BaseSingleton
{
public:
	static T& GetInstance()
	{
		static T instance;
		return instance;
	}
protected:
	BaseSingleton() {}
	~BaseSingleton() {}
public:
	BaseSingleton(BaseSingleton const&) = delete;
	BaseSingleton& operator=(BaseSingleton const&) = delete;
};

class ExampleSingleton
	: public BaseSingleton<ExampleSingleton>
{
public:
	ExampleSingleton() = default;
	~ExampleSingleton() = default;

	void Print() { std::cout << " Print BaseSingleton " << std::endl; };
};