// train.cpp : Defines the entry point for the console application.
//
#include <condition_variable>
#include <mutex>
#include <memory>
#include <iostream>
#include <map>
#include <list>
#include <chrono>
#include <thread>
#include <string>
#include <csignal>
#include <random>
#include <cctype>
#include <set>

namespace
{
	volatile std::sig_atomic_t gSignalStatus = 0;
}

void signal_handler(int signal)
{
	gSignalStatus = signal;
}

using namespace std;

const unsigned int TRAINSTATIONS_COUNT = 8;
const unsigned int TRAINS_COUNT = 4;
//const unsigned int TRAINS_COUNT = 12;

class Semaphore {
public:
	explicit Semaphore(int count_ = 1)
		: count(count_) {}

	inline void Notify()
	{
		std::unique_lock<std::mutex> lock(mtx);
		count++;
		cv.notify_one();
	}

	inline void Wait()
	{
		std::unique_lock<std::mutex> lock(mtx);

		while (count == 0){
			cv.wait(lock);
		}
		count--;
	}

private:
	std::mutex mtx;
	std::condition_variable cv;
	int count;
};

template< typename Stream >
class AtomicLogger {
public:

	explicit AtomicLogger(Stream& stream) :
		m_mutex(),
		m_stream(stream)
	{
	}

	void log(const std::string& str) {
		std::unique_lock<std::mutex> lock(m_mutex);
		m_stream << str << std::endl;
	}

private:
	mutable mutex m_mutex;
	Stream& m_stream;
};
typedef AtomicLogger< std::ostream > AtomicLoggerOstream;
typedef shared_ptr< AtomicLoggerOstream > AtomicLoggerOstreamPtr;

class Cargo{
public:
	Cargo() : Cargo(0, 0, 0) {}
	Cargo(unsigned int id_, unsigned int size_, unsigned int destinationId_) : id(id_), size(size_), destinationId(destinationId_) {}
	unsigned int id;
	unsigned int size;
	unsigned int destinationId;
};

struct CargoComparator {
	bool operator() (const Cargo& lhs, const Cargo& rhs) const{
		return lhs.size < rhs.size;
	}
};

class Station
{
private:
	unsigned int id;
	mutex cargoMutex;
	std::set<Cargo, CargoComparator> cargoList;
	Semaphore isNextSectionFree;
public:
	Station() : id() {}
	void SetId(unsigned int id_)
	{
		id = id_;
	}
	void WaitForNextSection()
	{
		isNextSectionFree.Wait();
	}
	void SetNextSectionFree()
	{
		isNextSectionFree.Notify();
	}
	Cargo TakeCargoSmallerThan(unsigned int size)
	{
		std::unique_lock<std::mutex> lock(cargoMutex);
		if (cargoList.empty())
			return Cargo(0, 0, 0);

		//cargoList.sort([](const Cargo& a, const Cargo& b) { return a.size < b.size; });
		if (cargoList.begin()->size < size)
		{
			Cargo cargo = *cargoList.begin();
			cargoList.erase(cargoList.begin());
			return cargo;
		}
		return Cargo(0, 0, 0);
	}

	void addCargo(Cargo item)
	{
		std::unique_lock<std::mutex> lock(cargoMutex);
		cargoList.insert(item);
	}
};

static mutex stationListMutex;
Station stationList[TRAINSTATIONS_COUNT];

class Train
{
public:
	enum state { STATION, SECTION };

	//ASSUMPTION: All train starts at station (because of default state of semaphore)
	Train(unsigned int id_, unsigned int speed_, unsigned int cargoCapacityLeft_, unsigned int lastStationId_, AtomicLoggerOstreamPtr logger_) :
		id(id_), speed(speed_), cargoCapacityLeft(cargoCapacityLeft_), lastStationId(lastStationId_), trainState(Train::STATION), logger(logger_)
	{}

	void TrainDepart()
	{
		Station& currentStation = stationList[lastStationId];
		logger->log(getInfo() + " wants to leave station " + to_string(lastStationId));
		currentStation.WaitForNextSection();
		logger->log(getInfo() + " is travelling to " + to_string((lastStationId + 1) % TRAINSTATIONS_COUNT));
	}

	void TrainArrive()
	{
		int stationIdToUnlock = lastStationId;
		if (stationIdToUnlock == 0)
			stationIdToUnlock = TRAINSTATIONS_COUNT;
		Station& previousStation = stationList[stationIdToUnlock - 1];

		previousStation.SetNextSectionFree();
		logger->log(getInfo() + " arrived to " + to_string(lastStationId));
	}

	void LoadCargoAtStation()
	{
		Station& currentStation = stationList[lastStationId];
		
		Cargo cargoItem;
		while ((cargoItem = currentStation.TakeCargoSmallerThan(cargoCapacityLeft)).size > 0)
		{
			logger->log(getInfo() + " loading cargo of size: " + to_string(cargoItem.size));
			cargoCapacityLeft -= cargoItem.size;

			destinationIdCargo.insert(make_pair(cargoItem.destinationId,cargoItem));

			std::this_thread::sleep_for(std::chrono::seconds(cargoItem.size));
			logger->log(getInfo() + " loaded cargo of size: " + to_string(cargoItem.size) + " for station: " + to_string(cargoItem.destinationId));
		}

		logger->log(getInfo() + " no (more) cargo for this train.");
	}

	void UnloadCargoAtStation()
	{
		multimap<unsigned int, Cargo>::iterator cargoIterator;
		while ((cargoIterator = destinationIdCargo.find(lastStationId)) != destinationIdCargo.end())
		{
			unsigned int cargoSize = cargoIterator->second.size;
			logger->log(getInfo() + " unloading cargo of size: " + to_string(cargoSize));
			cargoCapacityLeft += cargoSize;

			std::this_thread::sleep_for(std::chrono::seconds(cargoSize));
			logger->log(getInfo() + " unloaded cargo of size: " + to_string(cargoSize) + " for station: " + to_string(cargoIterator->second.destinationId));
			destinationIdCargo.erase(cargoIterator);
		}

	}


	void run()
	{
		while (gSignalStatus == 0) //ctrl+c (SIGINT) causes trains to go to train stations and stop the program
		{
			if (trainState == STATION)
			{
				UnloadCargoAtStation();
				LoadCargoAtStation();
				TrainDepart();

				trainState = SECTION;
				std::this_thread::sleep_for(std::chrono::seconds(speed));
			}
			if (trainState == SECTION)
			{
				lastStationId = (lastStationId + 1) % TRAINSTATIONS_COUNT;
				TrainArrive();
				trainState = STATION;
				std::this_thread::sleep_for(std::chrono::seconds(speed));
			}
		}
	}
private:
	unsigned int id;
	unsigned int speed;

	unsigned int cargoCapacityLeft;
	multimap<unsigned int, Cargo> destinationIdCargo;

	unsigned int lastStationId;
	state trainState;
	AtomicLoggerOstreamPtr logger;

	string getInfo()
	{
		return "Train=" + to_string(id) + ", cargoCapacityLeft=" + to_string(cargoCapacityLeft) + ": ";
	}
};

class Random
{
private:
	std::random_device rd;
	std::default_random_engine e1;
	std::uniform_int_distribution<unsigned int> uniform_dist;
public:
	explicit Random(unsigned int max) : uniform_dist(1, max)
	{
		e1.seed(rd());
	}
	unsigned int generate()
	{
		return uniform_dist(e1);
	}

};

class CargoCreator
{
public:
	explicit CargoCreator(AtomicLoggerOstreamPtr logger_) : logger(logger_) {}
	void run()
	{
		const unsigned int MAX_CARGO = 10;
		const unsigned int MAX_SLEEP = 2;
		Random randomCargoSize(MAX_CARGO);
		Random randomSleepSize(MAX_SLEEP);
		Random randomStationSize(TRAINSTATIONS_COUNT);

		unsigned int cargoId = 0;
		while (gSignalStatus == 0)
		{
			std::this_thread::sleep_for(std::chrono::seconds(randomSleepSize.generate()));
			unsigned int cargoSize = randomCargoSize.generate();
			unsigned int fromStation = randomStationSize.generate() - 1;
			unsigned int toStation = randomStationSize.generate() - 1;
			stationList[fromStation].addCargo(Cargo(cargoId++, cargoSize, toStation));
			logger->log("Generating cargo at station " + to_string(fromStation) + " of size: " + to_string(cargoSize) + " for station: " + to_string(toStation));
		}
	}
private:
	AtomicLoggerOstreamPtr logger;
};

int main()
{
	std::signal(SIGINT, signal_handler);
	AtomicLoggerOstreamPtr p_logger(new AtomicLoggerOstream(std::cout));
	
	for (unsigned int i = 0; i < TRAINSTATIONS_COUNT; ++i)
	{
		stationList[i].SetId(i);
	}

	std::thread t[TRAINS_COUNT];

	for (unsigned int i = 0; i < TRAINS_COUNT; ++i)
	{
		//arguments for Train: id, speed, cargoCapacity, start station id, logger) :
		t[i] = thread(&Train::run, Train(i, (i+1) , i * 4, i % TRAINSTATIONS_COUNT, p_logger));
	}

	std::thread cargoThread = thread(&CargoCreator::run, CargoCreator(p_logger));

	cargoThread.join();

	for (unsigned int i = 0; i < TRAINS_COUNT; ++i)
		t[i].join();

	return 0;
}

