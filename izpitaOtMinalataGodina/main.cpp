#include <iostream>
#include <fstream>
#include <stdexcept>
#include <cstring>
#include <utility>
#include <ctime>
#include <cstdlib>


template <typename T>
class DataSource
{
public:
	virtual T getNext() = 0;
	virtual T* getNext(int count) = 0;
	virtual bool hasNext() const = 0;
	virtual bool reset() = 0;
	virtual DataSource<T>& operator>>(T& elem) = 0;

	T operator()()
	{
		return getNext();
	}
	operator bool()
	{
		return hasNext();
	}

	virtual DataSource<T>* clone() const = 0;

	virtual ~DataSource() = default;
};

template <typename T>
class DefaultDataSource : public DataSource<T>
{
public:
	T getNext()	override
	{
		return T();
	}
	T* getNext(int count) override
	{
		return new T[count];
	}
	bool hasNext() override
	{
		return true;
	}
	bool reset() override
	{
		return true;
	}
	DefaultDataSource<T>& operator>>(T& elem) override
	{
		elem = T();
		return *this;
	}

	DataSource<T>* clone() const override
	{
		return new DefaultDataSource(*this);
	}

};


template <typename T>
class FileDataSource : public DataSource<T>
{
public:
	FileDataSource(const char* path)
	{
		ifs.open(path);
		if (!ifs.is_open())
		{
			throw std::runtime_error("couldnt open file");
		}
		this->path = new char[strlen(path) + 1];
		strcpy(this->path, path);
	}

	FileDataSource(const FileDataSource<T>& other)
	{
		ifs.open(other.path);
		if (!ifs.is_open())
		{
			throw std::runtime_error("couldnt open file");
		}
		this->path = new char[strlen(other.path) + 1];
		strcpy(this->path, other.path);
	}

	FileDataSource& operator=(const FileDataSource<T>& other)
	{
		if (this != &other)
		{
			FileDataSource<T> temp(other);
			swap(temp);
		}
		return *this;

	}

	T getNext() override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		T temp;
		ifs >> temp;
		return temp;
	}

	T* getNext(int count) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		T* temp = new T[count];
		for (int i = 0; i < count; i++)
		{
			ifs >> temp[i];
		}
		return temp;
		if (!ifs)
		{
			delete[] temp;
			throw std::runtime_error("couldnt read all that");
		}
	}

	bool hasNext() const override
	{
		return (ifs && ifs.peek() != EOF);
	}

	bool reset() override
	{
		ifs.clear();
		ifs.seekg(0, std::ios::beg);
		return true;
	}
	FileDataSource& operator>>(T& elem) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		ifs >> elem;
		return *this;
	}

	DataSource<T>* clone() const override
	{
		return new FileDataSource(*this);
	}

	~FileDataSource()
	{
		ifs.close();
		delete[] path;
	}

	void swap(FileDataSource& other)
	{
		std::swap(ifs, other.ifs);
		std::swap(path, other.path);
	}
	
private:
	std::ifstream ifs;
	char* path;
};

template <typename T>
class ArrayDataSource : public DataSource<T>
{
public:
	ArrayDataSource(const T* arr, unsigned size)
	{
		if (array == nullptr || size == 0) throw std::runtime_error("invalid arrDataSrc info");
		array = new T[size];
		for (int i = 0; i < size; i++)
		{
			try
			{
				array[i] = arr[i];
			}
			catch (...)
			{
				delete[] array;
				throw;
			}
		}
		this->size = size;
		this->cap = size;
		pos = 0;
	}
	ArrayDataSource(const ArrayDataSource& other)
		:ArrayDataSource(other.array, other.size)
	{
	}

	ArrayDataSource<T> operator=(const ArrayDataSource<T>& other)
	{
		if (this != &other)
		{
			ArrayDataSource<T> temp(*this);
			swap(temp);
		}
		return *this;
	}

	T getNext() override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		T res = array[pos];
		pos++;
		return res;
	}
	T* getNext(int count) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		int currPos = pos;
		T* temp = new T[count];
		for (int i = 0; i < count; i++)
		{
			try
			{
			temp[i] = getNext();
			}
			catch (...)
			{
				delete[] temp;
				pos = currPos;
				throw;
			}
		}
		return temp;

	}
	bool hasNext() const override
	{
		return size != pos;
	}
	bool reset() override
	{
		pos = 0;
		return true;
	}
	ArrayDataSource<T>& operator>>(T& elem) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		elem = getNext();
		return *this;
	}

	virtual DataSource<T>* clone() override
	{
		return new ArrayDataSource<T>(*this);
	}

	ArrayDataSource& operator+=(const T& elem)
	{
		if (size == cap)
			resize();
		array[size] = elem;
		size++;
		return *this;
	}
	ArrayDataSource& operator--()
	{
		pos--;
		return *this;
	}
	ArrayDataSource operator--(int)
	{
		ArrayDataSource temp(*this);
		--(*this);
		return temp;
	}
private:
	T* array;
	unsigned size;
	unsigned cap;
	int pos;

	void resize()
	{
		T* newArr = new T[cap * 2];
		for (int i = 0; i < size; i++)
		{
			try
			{
			newArr[i] = array[i];
			}
			catch (...)
			{
				delete[] newArr;
				throw;
			}
		}
		delete[] array;
		array = newArr;
		cap *= 2;
	}

	void swap(ArrayDataSource& other)
	{
		std::swap(array, other.array);
		std::swap(size, other.size);
		std::swap(pos, other.pos);
	}
};

template <typename T>
ArrayDataSource<T> operator+(ArrayDataSource<T> src ,const T& elem)
{
	ArrayDataSource<T> temp = src;
	src += elem;
	return temp;
}


template <typename T>
class AlternateDataSource : public DataSource<T>
{
public:
	AlternateDataSource(const DataSource<T>* const* arr, unsigned size)
	{
		copy(arr, size);
	}

	AlternateDataSource(const AlternateDataSource<T>& other)
	{
		copy(other.array, other.size);
		pos = other.pos;
	}

	AlternateDataSource<T>& operator=(const AlternateDataSource<T>& other)
	{
		AlternateDataSource<T> temp(*this);
		swap(temp);
	}

	~AlternateDataSource()
	{
		for (int i = 0; i < size; i++)
		{
			delete array[i];
		}
		delete array;
	}

	T getNext() override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		while (true)
		{
			try
			{
				if (++pos == size) pos = 0;
				T temp = array[pos]->getNext();
				return temp;
			}
			catch (...)
			{
			}
		}

	}
	T* getNext(int count) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		int currPos = pos;
		T* newArr = new T[count];
		for (int i = 0; i < count; i++)
		{
			try
			{
				newArr[i] = getNext();
			}
			catch (...)
			{
				delete[] newArr;
				pos = currPos;
				throw;
			}
		}
		return newArr;

	}
	bool hasNext() const override
	{
		for (int i = 0; i < size; i++)
		{
			if (array[i]->hasNext()) return true;
		}
		return false;
	}
	bool reset() override
	{
		for (int i = 0; i < size; i++)
		{
			if (!array[i]->reset()) return false;
		}
		pos = 0;
		return true;
	}
	AlternateDataSource<T>& operator>>(T& elem) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		elem = getNext();
		return *this;
	}

	DataSource<T>* clone() const override
	{
		return new AlternateDataSource<T>(*this);
	}


private:
	DataSource<T>** array;
	int size;
	int pos;

	void copy(const DataSource<T>* const* arr, unsigned size)
	{
		DataSource<T>** array = new DataSource<T>*[size] {};
		for (int i = 0; i < size; i++)
		{
			try
			{
			array[i] = arr[i]->clone();
			}
			catch (...)
			{
				for (int j = 0; j < i; j++)
				{
					delete array[j];
				}
				delete[] array;
				throw;
			}
		}
		this->size = size;
		this->pos = 0;
		this->array = array;
	}

	void swap(AlternateDataSource<T>& other)
	{
		std::swap(array, other.array);
		std::swap(size, other.size);
		std::swap(pos, other.pos);
	}
};

template <typename T>
class Generator
{
public:
	virtual T operator()() = 0;

	virtual bool hasNext() const = 0;
	virtual bool reset() = 0;

	virtual Generator<T>* clone() const = 0;
	virtual ~Generator() = default;
};


class FibonacciGenerator : public Generator<int>
{
public:
	FibonacciGenerator(unsigned count)
	{
		this->count = count;
	}

	int operator()() override
	{
		if (pos == 0)
		{
			pos++;
			return 0;
		}
		else if (pos == 1)
		{
			pos++;
			return 1;
		}
		else if (pos == 2)
		{
			first = 1;
			pos++;
			return 1;
		}
		else
		{
			int result = first + second;
			pos++;
			second = first;
			first = result;

			return result;
		}
	}
	bool hasNext() const override
	{
		return pos < count;
	}
	bool reset() override
	{
		pos = 0;
		return true;
	}

	Generator<int>* clone() const override
	{
		return new FibonacciGenerator(*this);
	}

private:
	int count;
	int pos = 0;

	int first = 0;
	int second = 1;
};

template <typename T, typename Func>
class GeneratorWithAFunction : public Generator<T>
{
public:
	GeneratorWithAFunction(Func function)
		:function(function)
	{ }

	T operator()() override
	{
		return function();
	}
	bool hasNext() const override
	{
		return true;
	}
	bool reset() override
	{
		return false;
	}

	Generator<T>* clone() const override
	{
		return new GeneratorWithAFunction(*this);
	}
	
private:
	Func function;
};

template <typename T>
class GeneratorDataSource : public DataSource<T>
{
public:
	GeneratorDataSource(const Generator<T>& gen)
	{
		generator = gen.clone();
	}

	GeneratorDataSource(const GeneratorDataSource& other)
	{
		generator = other.generator->clone();
	}

	GeneratorDataSource& operator=(const GeneratorDataSource& other)
	{
		Generator* temp= other.clone();
		delete generator;
		generator = temp;
	}

	~GeneratorDataSource()
	{
		delete generator;
	}

	T getNext() override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		return (*generator)();
	}
	T* getNext(int count) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		T* newArr = new T[count];
		for (int i = 0; i < count; i++)
		{
			try
			{
				newArr[i] = getNext();
			}
			catch (...)
			{
				delete[] newArr;
				throw;
			}
		}
		return newArr;
	}
	bool hasNext() const override
	{
		return generator->hasNext();
	}
	bool reset() override
	{
		return generator->reset();
	}
	GeneratorDataSource<T>& operator>>(T& elem) override
	{
		if (!hasNext()) throw std::runtime_error("cant sorry");
		elem = getNext();
		return *this;
	}

	DataSource<T>* clone() const override
	{
		return new GeneratorDataSource<T>(*this);
	}

private:
	Generator<T>* generator;
};

int x = std::rand();

int main()
{
	FibonacciGenerator fibGen(25);
	GeneratorWithAFunction<int, int (*)()> randomGen(std::rand);
	GeneratorDataSource<int> fibDataSrc(fibGen);
	GeneratorDataSource<int> randDataSrc(randomGen);

	DataSource<int>** ptrArray = new DataSource<int>* [2];
	ptrArray[0] = &fibDataSrc;
	ptrArray[1] = &randDataSrc;

	AlternateDataSource<int> arrDataSrc(ptrArray, 2);

	for (int i = 0; i < 50; i++)
	{
		std::cout << arrDataSrc.getNext() << ' ';
	}

	delete[] ptrArray;
}