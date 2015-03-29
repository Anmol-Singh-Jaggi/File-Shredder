#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <boost/filesystem.hpp>

using namespace std;
using namespace boost::filesystem;

const path LogFileName = "ShredderLog.txt";
ofstream Log;
stringstream LogErrorStream;  // Buffer soft errors to output them separately after the informational messages in the log file.

void DirectoryRecursiveIterate( const path& dirPath, vector<path>& dirContents )
{
	copy( recursive_directory_iterator( dirPath ), recursive_directory_iterator(), back_inserter( dirContents ) );
}

string GenerateRandomFileName()
{
	const size_t maxFileNameLength = 19;
	size_t fileNameLength = ( rand() % maxFileNameLength ) + 1;
	const string acceptedFileNameCharacters = "0123456789abcdefghijklmnopqrstuvwxyz";
	string fileName( fileNameLength, '*' );
	for ( auto& c : fileName )
	{
		c = acceptedFileNameCharacters[rand() % acceptedFileNameCharacters.length()];
	}
	return fileName;
}

path RandomRename( const path& inputPath )
{
	const size_t maxRenameAttempts = 10;
	string newName;
	size_t attempts = 0;
	boost::system::error_code ec;
	while ( attempts < maxRenameAttempts )
	{
		newName = GenerateRandomFileName();
		path newPath = inputPath.parent_path() / newName;
		if ( !exists( newPath ) )
		{
			rename( inputPath, newPath, ec );
			if ( ec ) // Maybe a reserved filename was generated
			{
				LogErrorStream << "Error renaming " << absolute( inputPath ) << " to " << absolute( newPath ) << " : " << ec.message() << endl;
			}
			else // No error
			{
				return newPath;
			}
		}
		attempts++;
	}
	return inputPath;
}

void WriteRandomData( const path& inputPath )
{
	auto inputFileSize = file_size( inputPath );

	ofstream fout( inputPath.string(), ios::binary );
	if ( !fout )
	{
		const string errorMsg = "Error opening " + absolute( inputPath ).string() + " : " + strerror( errno );
		LogErrorStream << errorMsg << endl;
		return;
	}

	int iterations = 5;

	vector<unsigned char> buffer( ( size_t )inputFileSize );
	while ( iterations -- )
	{
		unsigned char c = ( iterations & 1 ) ? 255 : 0;
		for ( decltype( inputFileSize ) i = 0; i < inputFileSize; i++ )
		{
			buffer[( size_t )i] = c;
		}
		fout.seekp( 0, ios::beg );
		fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with ASCII 255 and ASCII 0
		fout.flush();
	}

	fout.seekp( 0, ios::beg );
	for ( decltype( inputFileSize ) i = 0; i < inputFileSize; i++ )
	{
		buffer[( size_t )i] = rand() % 128;
	}
	fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with random characters
	fout.flush();

	fout.seekp( 0, ios::beg );
	for ( decltype( inputFileSize ) i = 0; i < inputFileSize; i++ )
	{
		buffer[( size_t )i] = 0;
	}
	fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with Null characters
	fout.flush();

	fout.close();

	fout.open( inputPath.string(), ios::binary ); // Changes the file size to 0
	fout.close();
}

void ShredFile( const path& inputPath )
{
	try
	{
		WriteRandomData( inputPath );
	}
	catch ( const filesystem_error& ex )
	{
		LogErrorStream << ex.what() << endl;
		return; // Not able to write random data. Abort !
	}

	path newPath;
	try
	{
		newPath = RandomRename( inputPath );
	}
	catch ( const filesystem_error& ex )
	{
		LogErrorStream << ex.what() << endl;
		// Not exiting here because success in renaming a file is not vital to shredding it.
	}

	try
	{
		remove( newPath );
	}
	catch ( const filesystem_error& ex )
	{
		LogErrorStream << ex.what() << endl;
	}

}

void Shred( const path& inputPath )
{
	Log << absolute( inputPath ) << endl;
	if ( exists( inputPath ) )
	{
		if ( is_directory( inputPath ) )
		{
			vector<path> dirContents;
			try
			{
				DirectoryRecursiveIterate( inputPath, dirContents );
			}
			catch ( const filesystem_error& ex )
			{
				LogErrorStream << ex.what() << endl;
				return; // Not able to list the directory's contents. No point in going forward.
			}

			for ( const auto& item : dirContents )
			{
				if ( !is_directory( item ) )
				{
					cout << "Shred " << absolute( item ) << " ? ( y/n ) ";
					if ( cin.get() == 'y' )
					{
						Log << absolute( item ) << endl;
						ShredFile( item );
					}
					else
					{
						Log << "Skipping " << absolute( item ) << endl;
					}

					// Clear any redundant input
					cin.clear();
					cin.ignore( numeric_limits<streamsize>::max(), '\n' );
				}
			}
			try
			{
				remove( inputPath );
			}
			catch ( const filesystem_error& ex )
			{
				LogErrorStream << ex.what() << endl;
			}
		}
		else
		{
			ShredFile( inputPath );
		}
	}
	else
	{
		LogErrorStream << "Error : " << inputPath << " does not exist !" << endl;
	}
}

int main( )
{
	string inputPath;
	cout << "Enter path to shred -:\n";
	getline( cin, inputPath );

	cout << "\n";

	Log.open( LogFileName.string() );
	if ( !Log )
	{
		cerr << "Error creating " << absolute( LogFileName ) << " : " << strerror( errno ) << endl;
	}

	Shred( inputPath );

	if ( Log )
	{
		if ( LogErrorStream.str().empty() )
		{
			cout << "The program ran without any errors.\n";
		}
		else
		{
			Log << "\nERRORS -:\n\n" << LogErrorStream.str() << endl;
			cout << "There were some errors during the execution of this program !\n\nCheck " << absolute( LogFileName ) << " for details.\n";
		}
	}
}
