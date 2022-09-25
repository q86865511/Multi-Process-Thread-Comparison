#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <ctime>
#include <unistd.h> // linux
#include <thread>
#include <cmath>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <error.h>
#include <time.h>

using namespace std;

class Sort {
private :
	string documentName;
	int numOfSeperate;
	int command;
	vector<int> nList;
	
	double cpuTime;

	void DocumentReader() {
		fstream txtReader( ( documentName + ".txt" ).c_str() );
		
		if ( !txtReader.is_open() )
			cout << "Error in opening file !" << endl;
		else {
			int tempInput;
			while ( txtReader.peek() != EOF ) {
				txtReader >> tempInput;
				nList.push_back( tempInput );
			} // while
			
			nList.pop_back();
			 
			txtReader.close();
		} // else
	} // DocumentReader()
	
	void DocumentPrinter() {
		string str;
		stringstream ss;
		ss << command;
		ss >> str;
		ofstream txtReader( ( documentName + "_output" + str + ".txt" ).c_str() );
		
		if ( !txtReader.is_open() )
			cout << "Error in opening file !" << endl;
		else {
			txtReader << "Sort : " << endl;
			for ( int i = 0 ; i < nList.size() ; i ++ )
				txtReader << nList[i] << endl;
			
			txtReader << "CPU Time : " << cpuTime << endl;
			time_t now = time(0);
			tm *ltm = localtime( &now );
			txtReader << "Output Time : ";
			txtReader << 1900 + ltm->tm_year << "-" << 1 + ltm->tm_mon << "-" << ltm->tm_mday;
			txtReader << " " << ltm->tm_hour << ":" << ltm->tm_min << ":" << ltm->tm_sec;
			txtReader << "+8:00";
		} // else 
	} // DocumentPrinter() 
	
	void Merge( vector<int> &finalList, vector< vector<int> > aList ) {
		while ( 1 ) {
			vector< vector<int> > tempList;
			for ( int i = 0 ; i < aList.size() ; i = i + 2 ) {
				vector<int> leftList = aList[i];
				vector<int> rightList;
				if ( i + 1 < aList.size() )
					rightList = aList[i + 1];
				else
					rightList.clear();

				int indexLeft = 0;
				int indexRight = 0;

				vector<int> mergeList;
				while ( ( indexLeft < leftList.size() ) && ( indexRight < rightList.size() ) ) {
					if ( leftList[indexLeft] <= rightList[indexRight] ) {
						mergeList.push_back( leftList[indexLeft] );
						indexLeft ++;
					} // if
					else {
						mergeList.push_back( rightList[indexRight] );
						indexRight ++;
					} // else
				} // while
				
				while ( indexLeft < leftList.size() ) {
					mergeList.push_back( leftList[indexLeft] );
					indexLeft ++;
				} // while
				
				while ( indexRight < rightList.size() ) {
					mergeList.push_back( rightList[indexRight] );
					indexRight ++;
				} // while

				tempList.push_back( mergeList );
			} // for
			
			aList.clear();
			aList.assign( tempList.begin(), tempList.end() );
			
			if ( aList.size() == 1 )
				break;
		} // while
		
		finalList.assign( aList[0].begin(), aList[0].end() );

	} // Merge()

	void Merge_ThreadsType( vector<int> &finalList, vector< vector<int> > aList ) {
		while ( 1 ) {
			vector< vector<int> > tempList;

			vector< int > emptyList;
			int sizeOfThreads;
			if( aList.size() % 2 == 0 )
				sizeOfThreads = aList.size() / 2;
			else
				sizeOfThreads = 1 + aList.size() / 2;

			tempList.resize( sizeOfThreads );
			thread threads[sizeOfThreads];
			int indexOfThreads = 0;

			for ( int i = 0 ; i < aList.size() ; i = i + 2 ) {
				if ( i + 1 < aList.size() )
					threads[indexOfThreads] = thread( MergeList, ref( tempList[indexOfThreads] ), aList[i], aList[i + 1] );
				else {
					emptyList.clear();
					threads[indexOfThreads] = thread( MergeList, ref( tempList[indexOfThreads] ), aList[i], emptyList );
				} // else

				indexOfThreads ++ ;
			} // for

			for ( int i = 0 ; i < sizeOfThreads; i ++ ) {
				threads[i].join();
			} // for

			aList.clear();
			aList.assign( tempList.begin(), tempList.end() );
			
			if ( aList.size() == 1 )
				break;
		} // while
		
		finalList.assign( aList[0].begin(), aList[0].end() );

	} // Merge_ThreadsType

	void Merge_ForkType( vector<int> &finalList, vector< vector<int> > aList ) {
		while ( 1 ) {
			vector< vector<int> > tempList;

			vector< int > emptyList;
			int sizeOfForks;
			if( aList.size() % 2 == 0 )
				sizeOfForks = aList.size() / 2;
			else
				sizeOfForks = 1 + aList.size() / 2;

			tempList.resize( sizeOfForks );
			int indexOfForks = 0;

			key_t keyOfMemory = IPC_PRIVATE; // 自己設定分享的起始ID
			int shmID; // shmget到的共享對象
			int *sharedList; // 存放共享資料的array
			int pid[sizeOfForks]; // 產生子程序所需
			if ( ( shmID = shmget( keyOfMemory,  nList.size() * sizeof( int ) , IPC_CREAT | 0666 ) ) < 0 ) {
				perror("shmget");
        		_exit(1);
			} // if

			if ( ( sharedList = ( int * )shmat( shmID, NULL, 0 ) )== ( int * ) -1 ) {
				perror("shmat");
				_exit(1);
			} // if

			for ( int i = 0 ; i < aList.size() ; i = i + 2 ) {
				pid[indexOfForks] = fork();
				if ( pid[indexOfForks] == -1 ) {
					perror( "fork error" );
					break;
				} // if 失敗
				else if ( pid[indexOfForks] == 0 ) {
					if ( i + 1 < aList.size() )
						MergeList( tempList[indexOfForks], aList[i], aList[i + 1] );
					else {
						emptyList.clear();
						MergeList( tempList[indexOfForks], aList[i], emptyList );
					} // else
					
					sharedList[0] = tempList[indexOfForks].size();
					for ( int j = 0 ; j < tempList[indexOfForks].size() ; j ++ ) 
						sharedList[j + 1] = tempList[indexOfForks][j];

					exit( 0 );
				} // if 子程序
				else if ( pid[indexOfForks] > 0 ) {
					waitpid( pid[indexOfForks], NULL, 0 );
					vector< int > sharedTempList;
					for ( int j = 1 ; j <= sharedList[0] ; j ++ )
						sharedTempList.push_back( sharedList[j] );

					tempList[indexOfForks].assign( sharedTempList.begin(), sharedTempList.end() );
					indexOfForks ++;
				} // if 父程序B
			} // for

			if ( shmdt( sharedList ) == -1 ) {
				perror( "shmdt" );
				_exit(1);
			} // if

			if ( shmctl( shmID, IPC_RMID, NULL ) == -1 ) {
				perror("shmctl");
        		_exit(1);
			} // if

			aList.clear();
			aList.assign( tempList.begin(), tempList.end() );

			if ( aList.size() == 1 )
				break;
		} // while
		
		finalList.assign( aList[0].begin(), aList[0].end() );
	} // Merge()

public :
	static void BubbleSort( vector<int> &aList ) {
		int temp;

		for ( int i = 0 ; i < aList.size() - 1 ; i ++ ) {
			// if ( i % 1000 == 0 )
			//	cout << "Bubble Sort Index : " << i << "	value : " <<  aList[i] << endl;
			bool isSorted = true;
			for ( int j = 0 ; j < aList.size() - 1 - i ; j ++ ) {
				if ( aList[j] > aList[j + 1] ) {
					temp = aList[j];
					aList[j] = aList[j + 1];
					aList[j + 1] = temp;
					isSorted = false;
				} // if
			} // for
			
			if ( isSorted )
				break;
		} // for
	} // BubbleSort()

	static void MergeList( vector< int > &finalList, vector< int > leftList, vector< int > rightList ) {
		int indexLeft = 0;
		int indexRight = 0;

		vector<int> mergeList;
		while ( ( indexLeft < leftList.size() ) && ( indexRight < rightList.size() ) ) {
			if ( leftList[indexLeft] <= rightList[indexRight] ) {
				mergeList.push_back( leftList[indexLeft] );
				indexLeft ++;
			} // if
			else {
				mergeList.push_back( rightList[indexRight] );
				indexRight ++;
			} // else
		} // while
		
		while ( indexLeft < leftList.size() ) {
			mergeList.push_back( leftList[indexLeft] );
			indexLeft ++;
		} // while
		
		while ( indexRight < rightList.size() ) {
			mergeList.push_back( rightList[indexRight] );
			indexRight ++;
		} // while

		finalList.assign( mergeList.begin(), mergeList.end() );
	} // MergeListThreads()

	void CleanNList() {
		nList.clear();
	} // CleanNList()
	 
	void SetInputData( string name, int num, int c ) {
		documentName = name;
		numOfSeperate = num;
		command = c;
	} // SetInputData()
	
	void MissionOne() {
		DocumentReader();

		clock_t start;
		start = clock();
		BubbleSort( nList );
		cpuTime = double( clock() - start ) / CLOCKS_PER_SEC;
		
		DocumentPrinter();
	} // MissionOne()
	
	void MissionTwo() {
		DocumentReader();
		vector< vector<int> > kOfNList;
		int seperateSize = nList.size() / numOfSeperate;
		if ( nList.size() < numOfSeperate ) {
			cout << "Error : Seperate numbers is bigger than List's size" << endl;
			return;
		} // if

		int index = 0;
		clock_t start;
		start = clock();
		
		while ( index < nList.size() ) {
			vector<int> tempList;
			
			for ( int i = 0 ; index < nList.size() && i < seperateSize ; i ++ ) {
				tempList.push_back( nList[index] );
				index ++;
			} // for
			
			BubbleSort( tempList );
			kOfNList.push_back( tempList );
		} // while 

		Merge( nList, kOfNList );

		cpuTime = double( clock() - start ) / CLOCKS_PER_SEC;
		
		DocumentPrinter();
	} // MissionTwo()
	
	void MissionThree() {
		DocumentReader();
		vector< vector<int> > kOfNList;
		int seperateSize = nList.size() / numOfSeperate;
		if ( nList.size() < numOfSeperate ) {
			cout << "Error : Seperate numbers is bigger than List's size" << endl;
			return;
		} // if

		int index = 0;
		clock_t start;
		start = clock();
		
		if ( nList.size() % numOfSeperate != 0 )
			numOfSeperate ++;
		while ( index < nList.size() ) {
			vector<int> tempList;
			
			for ( int i = 0 ; index < nList.size() && i < seperateSize ; i ++ ) {
				tempList.push_back( nList[index] );
				index ++;
			} // for
			
			kOfNList.push_back( tempList );
		} // while 

		key_t keyOfMemory = IPC_PRIVATE; // 自己設定分享的起始ID
		int shmID; // shmget到的共享對象
		int *aList; // 存放共享資料的array


		int pid[numOfSeperate]; // 產生子程序所需
    	shmID = shmget( keyOfMemory, nList.size() * sizeof( int ), IPC_CREAT | 0666 );
		aList = ( int * )shmat( shmID, NULL, 0 );
		for ( int i = 0 ; i < numOfSeperate  ; i ++ ) {
			pid[i] = fork();
			if ( pid[i] == -1 ) {
				perror( "fork error" );
				break;
			} // if 失敗
			else if ( pid[i] == 0 ) {
				BubbleSort( kOfNList[i] );
				aList[0] = kOfNList[i].size();
				for ( int j = 0 ; j < kOfNList[i].size() ; j ++ )
					aList[j + 1] = kOfNList[i][j];
				exit( 0 );
			} // if 子程序
			else if ( pid[i] > 0 ) {
				waitpid( pid[i], NULL, 0 );
				for ( int j = 0 ; j < aList[0] ; j ++ )
					kOfNList[i][j] = aList[j + 1];
				
			} // if 父程序
		} // for
		
		shmdt( aList );
    	shmctl( shmID, IPC_RMID, NULL );    

		Merge_ForkType( nList, kOfNList );

		cpuTime = double( clock() - start ) / CLOCKS_PER_SEC;
		
		DocumentPrinter();
	} // MissionThree()
	
	void MissionFour() {
		DocumentReader();
		vector< vector<int> > kOfNList;
		int seperateSize = nList.size() / numOfSeperate;
		if ( nList.size() < numOfSeperate ) {
			cout << "Error : Seperate numbers is bigger than List's size" << endl;
			return;
		} // if

		int index = 0;
		clock_t start;
		start = clock();
		
		while ( index < nList.size() ) {
			vector<int> tempList;
			
			for ( int i = 0 ; index < nList.size() && i < seperateSize ; i ++ ) {
				tempList.push_back( nList[index] );
				index ++;
			} // for
			
			kOfNList.push_back( tempList );
		} // while 
		
		if ( nList.size() % numOfSeperate != 0 )
			numOfSeperate ++;
		thread threads[numOfSeperate];
		for ( int i = 0 ; i < numOfSeperate ; i ++ ) {
			threads[i] = thread( BubbleSort, ref( kOfNList[i] ) );
		} // for
		
		for ( int i = 0 ; i < numOfSeperate ; i ++ ) {
			threads[i].join();
		} // for
		
		Merge_ThreadsType( nList, kOfNList );

		cpuTime = double( clock() - start ) / CLOCKS_PER_SEC;
		
		DocumentPrinter();
	} // MissionFour()
	
}; // Sort

class Interacter {
private :
	Sort sorterObject;

public :
	void Runner() {
		while ( 1 ) {
			sorterObject.CleanNList();
			
			string documentName;
			int numOfSeperate;
			int command;
			
			cout << "Enter File Name :" << endl;
			cin >> documentName;
			cout << "Enter how many pieces you want to cut :" << endl; 
			cin >> numOfSeperate;
			cout << "Enter the way's numbers:(way1, way2, way3, way4)" << endl; 
			cin >> command;
			
			sorterObject.SetInputData( documentName, numOfSeperate, command );
			
			if ( command == 1 )
				sorterObject.MissionOne();
			else if ( command == 2 )
				sorterObject.MissionTwo();
			else if ( command == 3 )
				sorterObject.MissionThree();
			else if ( command == 4 )
				sorterObject.MissionFour();
			else
				cout << "Error command !" << endl; 
			
			
			string endSituation;
			cout << endl << "Keep sorting other file or way ? ( Y / N ) : ";
			cin >> endSituation;
			if ( endSituation == "Y" || endSituation == "y" )
				continue;
			else if ( endSituation == "N" || endSituation == "Y" )
				break;
			else {
				cout << "Error options !" << endl;
				break;
			} // end
			
		} // while
	} // runner()
}; // Interacter

int main() { 
	Interacter InteracterObject;
	InteracterObject.Runner();
} // main() 
