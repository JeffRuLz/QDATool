#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstring>
#include <vector>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

typedef struct 
{
	uint32_t Compress; 	  //Are you compressing? 0=No 1=Yes
	uint8_t Signature[4]; //Signature 'QDA0'
	uint32_t DataCount;	  //The number of data
	uint8_t Reserved[244];
} TQDAFileHeader;

typedef struct 
{
	uint32_t Offset; 			//Offset from the "beginning" of the file
	uint32_t Length;			//Data length (after storage)
	uint32_t RestoredLength;	//Data length (after decompression)
	uint8_t ID[256];			//ID
} TQDADataHeader;

typedef struct
{
	uint32_t size;
	uint8_t* data;
} TDataEntry;

static void printHelp()
{
	cout << "  -help" << endl;
	cout << "  -print [qda file]" << endl;
	cout << "  -dump [qda file] [output folder]" << endl;
	cout << "  -extract [qda file] [file name]" << endl;
	cout << "  -build [qda file] [input folder]" << endl;
}

static bool verifyQDA(char* fpath)
{
	bool result = false;
	ifstream f (fpath, ios::binary);

	if (f.fail())
		cout << "Could not open '" << fpath << "'." << endl;

	else
	{
		uint32_t magic = 0;
		f.seekg(4, ios_base::beg);
		f.read(reinterpret_cast<char*>(&magic), 4);

		if (magic != 0x30414451) //0ADQ
			cout << "'" << fpath << "'' is not a valid QDA file." << endl;
		else
			result = true;
	}

	f.close();
	return result;
}

static bool isFolder(char* path)
{
	struct stat info;

	if (stat(path, &info) != 0)
		cout << "Could not access '" << path << "'." << endl;
	else if (info.st_mode & S_IFDIR)
		return true;
	else
		cout << "'" << path << "' is not a directory." << endl;

	return false;
}

static void printQDA(int argc, char* argv[])
{
	if (argc <= 2)
	{
		cout << "Invalid usage of '-print'." << endl;
		return;
	}

	if (!verifyQDA(argv[2]))
		return;

	//
	TQDAFileHeader fh;
	TQDADataHeader dh;

	ifstream f (argv[2], ios::binary);
	f.read(reinterpret_cast<char*>(&fh), sizeof(TQDAFileHeader));
	
	cout << "'" << argv[2] << "' contains " << fh.DataCount << " files." << endl;
	cout << "----------------------------" << endl;

	for (int i = 0; i < fh.DataCount; i++)
	{		
		f.read(reinterpret_cast<char*>(&dh), sizeof(TQDADataHeader));
		cout << dh.ID << endl;
	}

	f.close();
}

static void dumpQDA(int argc, char* argv[])
{
	if (argc <= 3)
	{
		cout << "Invalid usage of '-dump'." << endl;
		return;
	}

	if (!verifyQDA(argv[2]))
		return;

	if (!isFolder(argv[3]))
		return;

	//
	TQDAFileHeader fh;
	vector<TQDADataHeader> v;
	ifstream f (argv[2], ios::binary);

	f.read(reinterpret_cast<char*>(&fh), sizeof(TQDAFileHeader));

	for (int i = 0; i < fh.DataCount; i++)
	{
		TQDADataHeader dh;
		f.read(reinterpret_cast<char*>(&dh), sizeof(TQDADataHeader));
		v.push_back(dh);
	}

	for (TQDADataHeader dh: v)
	{
		//build path string
		string fpath;
		fpath.append(argv[3]);
		fpath.append("/");
		fpath.append(reinterpret_cast<char*>(dh.ID));

		//read data into buffer
		uint8_t* buffer = new uint8_t[dh.Length];
		f.seekg(dh.Offset, ios_base::beg);
		f.read(reinterpret_cast<char*>(buffer), dh.Length);	

		//write data into file
		ofstream out (fpath, ios::binary);
		if (out.fail())
			cout << "Error: ";
		else
			out.write(reinterpret_cast<char*>(buffer), dh.Length);
		out.close();

		//cleanup
		delete[] buffer;
		cout << fpath << endl;
	}

	v.clear();
	f.close();
}

static void extractQDA(int argc, char* argv[])
{
	if (argc <= 3)
	{
		cout << "Invalid usage of '-extract'." << endl;
		return;
	}

	if (!verifyQDA(argv[2]))
		return;

	TQDAFileHeader fh;
	TQDADataHeader dh;

	ifstream f (argv[2], ios::binary);
	f.read(reinterpret_cast<char*>(&fh), sizeof(TQDAFileHeader));

	for (int i = 0; i < fh.DataCount; i++)
	{
		f.read(reinterpret_cast<char*>(&dh), sizeof(TQDADataHeader));

		if (strcmp((const char*)dh.ID, argv[3]) == 0)
		{
			uint8_t* buffer = new uint8_t[dh.Length];

			f.seekg(dh.Offset, ios_base::beg);
			f.read(reinterpret_cast<char*>(buffer), dh.Length);

			ofstream out (argv[3], ios::binary);
			if (out.fail())
				cout << "Error: Could not open " << argv[3] << endl;
			else
				out.write(reinterpret_cast<char*>(buffer), dh.Length);
			out.close();

			delete[] buffer;
			break;
		}
	}

	f.close();
}

static void buildQDA(int argc, char* argv[])
{
	//check argument count
	if (argc <= 3)
	{
		cout << "Invalid usage of '-build'." << endl;
		return;
	}

	if (!isFolder(argv[3]))
		return;

	vector<TQDADataHeader> vhead;
	vector<TDataEntry> vdata;
	vector<string> vpaths;
	TQDAFileHeader fh = { 0 };
	uint32_t offset = 0;	

	//setup file header
	fh.Compress = 0;
	fh.Signature[0] = 'Q';
	fh.Signature[1] = 'D';
	fh.Signature[2] = 'A';
	fh.Signature[3] = '0';
	fh.DataCount = 0;

	//count files in input folder
	for (const auto & entry : filesystem::directory_iterator(argv[3]))
	{
		string fname = entry.path().string();

		if (fname.find("Thumbs.db") == string::npos)
		{
			vpaths.push_back(fname);
			fh.DataCount += 1;
		}
	}
	
	offset = sizeof(TQDAFileHeader) + sizeof(TQDADataHeader) * fh.DataCount;

	//setup data headers
	for (int i = 0; i < fh.DataCount; i++)
	{
		ifstream in (vpaths[i], ifstream::binary);

		if (in.fail())
			cout << "Error opening '" << vpaths[i] << "'." << endl;
		else
		{	
			//get file size
			streampos fsize = in.tellg();
			in.seekg(0, ios_base::end);
			fsize = in.tellg() - fsize;
			in.seekg(0, ios_base::beg);

			//setup header
			TQDADataHeader dh = { 0 };
			dh.Offset = offset;
			dh.Length = fsize;
			dh.RestoredLength = dh.Length;

			//get file name
			string fname (vpaths[i]);
			fname = fname.substr(fname.find_last_of("/\\")+1);
			cout << fname << endl;

			for (int a = 0; a < fname.length(); a++)
				dh.ID[a] = fname[a];

			//load data
			TDataEntry d;
			d.size = dh.Length;
			d.data = new uint8_t[d.size];
			
			in.read(reinterpret_cast<char*>(d.data), d.size);

			vdata.push_back(d);
			vhead.push_back(dh);

			offset += dh.Length;
		}

		in.close();		
	}

	//write qda file
	ofstream out (argv[2], ios::binary);

	if (out.fail())
		cout << "Could not open '" << argv[2] << "'." << endl;
	else
	{
		//write file header
		out.write(reinterpret_cast<char*>(&fh), sizeof(TQDAFileHeader));

		//write data headers
		for (TQDADataHeader dh: vhead)
			out.write(reinterpret_cast<char*>(&dh), sizeof(TQDADataHeader));

		//write data
		for (TDataEntry d: vdata)
			out.write(reinterpret_cast<char*>(d.data), d.size);
	}
	
	out.close();	

	//free data
	for (TDataEntry d: vdata)
		delete[] d.data;

	vpaths.clear();
	vdata.clear();
	vhead.clear();	
}

int main(int argc, char* argv[])
{
	if (argc <= 1)
	{
		printHelp();
		return 1;
	}

	if (strcmp(argv[1], "-help") == 0)
		printHelp();
	
	else if (strcmp(argv[1], "-print") == 0)
		printQDA(argc, argv);

	else if (strcmp(argv[1], "-dump") == 0)
		dumpQDA(argc, argv);

	else if (strcmp(argv[1], "-extract") == 0)
		extractQDA(argc, argv);

	else if (strcmp(argv[1], "-build") == 0)
		buildQDA(argc, argv);

	else
	{
		printHelp();
		return 1;
	}

	return 0;
}