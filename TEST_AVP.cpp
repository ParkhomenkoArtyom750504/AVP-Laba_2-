// AVP2.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

//#include <algorithm>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <Windows.h>

using namespace std;

#define CACHE_LINE 64
#define SETW 5

class CacheInfo {
public:
	unsigned long size;
	BYTE associativity;

	CacheInfo(BYTE associativity, unsigned long size) {
		this->size = size;
		this->associativity = associativity;
	}
};

CacheInfo GetCacheInfo() {
	DWORD bufSize = 0;
	GetLogicalProcessorInformation(0, &bufSize);
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION* info = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*)malloc(bufSize);
	GetLogicalProcessorInformation(info, &bufSize);

	DWORD maxSize = 0;
	unsigned idx = 0;

	int count = bufSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
	for (int i = 0; i < count; i++) {
		if (info[i].Relationship == RelationCache) {
			if (maxSize < info[i].Cache.Size) {
				maxSize = info[i].Cache.Size;
				idx = i;
				std::cout << "L" << (int)info[idx].Cache.Level << ": "
					<< std::setw(4) << info[idx].Cache.Size / 1024 << " KB ("
					<< info[idx].Cache.LineSize << " bytes cache line, "
					<< std::setw(2) << (int)info[idx].Cache.Associativity << "-way associative)"
					<< std::endl;
			}
		}
	}

	CacheInfo cache(info[idx].Cache.Associativity, info[idx].Cache.Size);

	free(info);
	return cache;
}

int writeFile(const char* fileName, size_t minOffset, size_t maxOffset, int maxN, size_t* minTable, size_t* avrTable,const char* separator = ",")
{
	ofstream file(fileName);
	if (!file.is_open()) {
		std::cerr << "File create failed" << std::endl;
		return -1;
	}

	int countColumns = 0;
	file << "Table with minimum memory access time" << std::endl;
	for (size_t curOffset = minOffset; curOffset <= maxOffset; curOffset *= 2)
	{
		countColumns++;
		file << separator;
		if (curOffset / 1024 < 1024)
			file << curOffset / 1024 << " KB";
		else
			file << curOffset / 1024 / 1024 << " MB";
	}
	file << separator << "average" << std::endl;

	for (int N = 0; N < maxN; N++)
	{
		file << N + 1;
		for (int curCol = 0; curCol < countColumns; curCol++) {
			file << separator << minTable[N * (countColumns + 1) + curCol];
		}
		file << separator << minTable[N * (countColumns + 1) + countColumns];
		file << std::endl;
	}
	file << std::endl;

	file << "Table with average memory access time" << std::endl;
	for (size_t curOffset = minOffset; curOffset <= maxOffset; curOffset *= 2)
	{
		file << separator;
		if (curOffset / 1024 < 1024)
			file << curOffset / 1024 << " KB";
		else
			file << curOffset / 1024 / 1024 << " MB";
	}
	file << separator << "average" << std::endl;

	for (int N = 0; N < maxN; N++)
	{
		file << N + 1;
		for (int curCol = 0; curCol < countColumns; curCol++) {
			file << separator << avrTable[N * (countColumns + 1) + curCol];
		}
		file << separator << avrTable[N * (countColumns + 1) + countColumns];
		file << std::endl;
	}

	file.close();
	return 0;
}

int outputTable(size_t minOffset, size_t maxOffset, int maxN, size_t* table)
{
	int countColumns = 0;

	std::cout << std::setw(2) << "N";
	for (size_t curOffset = minOffset; curOffset <= maxOffset; curOffset *= 2)
	{
		countColumns++;
		if (curOffset / 1024 < 1024)
			std::cout << std::setw(SETW) << curOffset / 1024 << " KB";
		else
			std::cout << std::setw(SETW) << curOffset / 1024 / 1024 << " MB";
	}
	std::cout << std::setw(SETW) << "AVR" << std::endl;

	for (int N = 0; N < maxN; N++)
	{
		std::cout << std::setw(2) << N + 1;
		for (int curCol = 0; curCol < countColumns; curCol++)
		{
			std::cout << std::setw(SETW) << table[N * (countColumns + 1) + curCol] << " us";
		}
		std::cout << std::setw(SETW) << table[N * (countColumns + 1) + countColumns];
		std::cout << std::endl;
	}

	return 0;
}


int main(int argc, char** argv) {
	//std::cout << "SetPriorityClass: " << SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS) << std::endl << std::endl;

	CacheInfo maxCache = GetCacheInfo();

	const size_t maxOffset = 64 * 1024 * 1024;
	const size_t maxOffset_int = maxOffset / sizeof(size_t);

	const size_t minOffset = 32 * 1024; //maxCache.size / maxCache.associativity;
	const size_t minOffset_int = minOffset / sizeof(size_t);

	int maxN = 20;
	int countRepeat = 1;
	int countIteration = maxCache.size * 100 / CACHE_LINE; //maxN * 1000;
	bool output = false;
	const char* filename = "report.csv";
	const char* separator = ";";

	for (int i = 0; i < argc; i++)
	{
		if (strcmp(argv[i], "-i") == 0) {
			countIteration = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-n") == 0) {
			maxN = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-r") == 0) {
			countRepeat = atoi(argv[i + 1]);
		}
		else if (strcmp(argv[i], "-o") == 0) {
			output = true;
		}
		else if (strcmp(argv[i], "-f") == 0) {
			filename = argv[i + 1];
		}
		else if (strcmp(argv[i], "-s") == 0) {
			separator = argv[i + 1];
		}
		else if (strcmp(argv[i], "-?") == 0) {
			cout << endl << "HELP:" << endl;
			cout << setw(5) << "-?" << setw(3) << " " << setw(3) << " " << "help" << endl;
			cout << setw(5) << "-f" << setw(3) << "[]" << setw(3) << " " << "report file" << endl;
			cout << setw(5) << "-s" << setw(3) << "[]" << setw(3) << " " << "seporator in file \",\" \\ \";\" eng\\rus excel" << endl;
			cout << setw(5) << "-n" << setw(3) << "[]" << setw(3) << " " << "max number associativity" << endl;
			cout << setw(5) << "-i" << setw(3) << "[]" << setw(3) << " " << "number of memory access iterations" << endl;
			cout << setw(5) << "-r" << setw(3) << "[]" << setw(3) << " " << "report file" << endl;
			cout << setw(5) << "-o" << setw(3) << " " << setw(3) << " " << "output on display" << endl;
			return 0;
		}
	}

	size_t temp;

	size_t* array = (size_t*)_aligned_malloc(maxN * maxOffset, 64);
	if (array == NULL) {
		std::cerr << "Memory allocation failed" << std::endl;
		return -1;
	}

	cout << endl;
	//std::cout << "First offset = max cache size / associativity" << std::endl;
	cout << "Number of repetitions: " << countRepeat << endl;
	cout << "Iteration of memory access in one repetition: " << countIteration << endl << endl;

	int countColumns = 0, columns = 0;


	for (size_t curOffset = minOffset; curOffset <= maxOffset; curOffset *= 2)
		countColumns++;
	countColumns++; //+1 average column

	//create table for time and fill MAXNUMBER
	size_t* minTable = (size_t*)malloc(maxN * countColumns * sizeof(size_t));
	size_t* avrTable = (size_t*)malloc(maxN * countColumns * sizeof(size_t));

	for (int i = 0; i < maxN; i++)
	{
		for (int j = 0; j < countColumns; j++)
		{
			minTable[i * countColumns + j] = MAXSIZE_T;
			avrTable[i * countColumns + j] = 0;
		}
	}

	for (size_t N = 1; N <= maxN; N++) {
		//column in table (offset)
		columns = 0;
		for (size_t offset_int = minOffset_int; offset_int <= maxOffset_int; offset_int *= 2)
		{
			// init
			for (size_t n = 0; n < N; n++) {
				array[n * offset_int] = (n == N - 1) ?
					0 : // last element
					(n + 1) * offset_int;
			}


			for (int measure = 0; measure < countRepeat; measure++)
			{
				temp = 0;
				std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
				for (size_t i = 0; i < countIteration; i++)
					temp = array[temp];
				std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();

				size_t time = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();

				//find average and mininum time
				avrTable[(N - 1) * countColumns + columns] += time;
				if (time < minTable[(N - 1) * countColumns + columns])
					minTable[(N - 1) * countColumns + columns] = time;
			}
			avrTable[(N - 1) * countColumns + columns] /= countRepeat;
			columns++;
		}
	}

	if (temp == 0)
		std::cout << "";

	//find the total average time
	for (int n = 0; n < maxN; n++)
	{
		minTable[n * countColumns + countColumns - 1] = 0;
		avrTable[n * countColumns + countColumns - 1] = 0;
		for (int j = 0; j < countColumns - 1; j++)
		{
			minTable[n * countColumns + countColumns - 1] += minTable[n * countColumns + j];
			avrTable[n * countColumns + countColumns - 1] += avrTable[n * countColumns + j];
		}
		minTable[n * countColumns + countColumns - 1] /= countColumns - 1;
		avrTable[n * countColumns + countColumns - 1] /= countColumns - 1;
	}

	if (output == true)
	{
		cout << "Table with minimum time" << std::endl;
		outputTable(minOffset, maxOffset, maxN, minTable);
		cout << std::endl;

		cout << "Table with average time" << std::endl;
		outputTable(minOffset, maxOffset, maxN, avrTable);
		cout << std::endl;
	}

	writeFile(filename, minOffset, maxOffset, maxN, minTable, avrTable, separator);

	free(minTable);
	free(avrTable);
	_aligned_free(array);

	system("pause");

	return 0;
}
