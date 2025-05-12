#include <iostream>

class Clock {
protected:
	int seconds;

public:
	Clock(int h = 0, int m = 0, int s = 0) {
		seconds = h * 3600 + m * 60 + s;
	}

	int getH() const {
		return seconds / 3600;
	}

	int getM() const {
		return (seconds % 3600) / 60;
	}

	int getS() const {
		return seconds % 60;
	}

	virtual void tic() {
		seconds++;
	}

	void print() const {
		printf("%02d:%02d:%02d\n", getH(), getM(), getS());    }
};

class Timer : public Clock {
public:
	Timer(int h = 0, int m = 0, int s = 0) : Clock(h, m, s) {}

	void tic() override {
		if (seconds > 0) {
			seconds--;
		} else {
			std::cout << "Done!\n";
		}
	}
};

class Stopwatch : public Clock {
public:
	Stopwatch() : Clock() {}

	void stop() {
		std::cout << "Total time: ";
		print();
	}
};
