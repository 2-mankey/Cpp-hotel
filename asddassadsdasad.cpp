#include <iostream>

class Array {
private:
	int len;
	int* data;
public:
	Array();
	~Array();

	int length() {
		return this->len;
	}

	void reverse() {
		int* new_data = new int[len];
		for (int i = 0; i < len; ++i) {
			new_data[i] = data[len - 1 - i];
		}
		delete[] data;
		data = new_data;
	}

	friend std::ostream& operator<<(std::ostream& os, const Array& arr) {
		for (int i = 0; i < arr.len; ++i) {
			os << arr.data[i] << " ";
		}
		return os;
	}

	friend std::istream& operator>>(std::istream& is, Array& arr) {
		for (int i = 0; i < arr.len; ++i) {
			is >> arr.data[i];
		}
		return is;
	}
};