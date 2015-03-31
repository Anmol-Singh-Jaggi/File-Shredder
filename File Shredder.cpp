#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include "boost/filesystem.hpp"

using namespace std;
using namespace boost::filesystem;

static ofstream Log;
static stringstream LogErrorStream;  // Buffer soft errors to output them separately after the informational messages in the log file.

// Iterate through a directory and store everything found ( regular files, directories or any other special files ) in the input container
static void DirectoryIterate( const path& dirPath, vector<path>& dirContents )
{
	if ( is_directory( dirPath ) )
	{
		copy( directory_iterator( dirPath ), directory_iterator(), back_inserter( dirContents ) );
	}
}

// Generate a random file name
static string GenerateRandomFileName()
{
	const size_t maxFileNameLength = 19;
	const size_t fileNameLength = ( rand() % maxFileNameLength ) + 1;
	const string acceptedFileNameCharacters = "0123456789abcdefghijklmnopqrstuvwxyz";
	string fileName( fileNameLength, '*' );
	for ( auto& c : fileName )
	{
		c = acceptedFileNameCharacters[rand() % acceptedFileNameCharacters.length()];
	}
	return fileName;
}

// Rename the input file to a random string
static path RandomRename( const path& inputPath )
{
	const size_t maxRenameAttempts = 10;
	size_t attempts = 0;
	while ( attempts < maxRenameAttempts )
	{
		const path newPath = inputPath.parent_path() / GenerateRandomFileName();
		if ( !exists( newPath ) )
		{
			boost::system::error_code ec;
			rename( inputPath, newPath, ec );
			if ( ec ) // Maybe a reserved filename was generated
			{
				LogErrorStream << "Failure in renaming " << absolute( inputPath ) << " to " << absolute( newPath ) << " : " << ec.message() << endl;
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

// Overwrite the input file with random data
static bool WriteRandomData( const path& inputPath )
{
	boost::system::error_code ec;
	const auto inputFileSize = file_size( inputPath, ec );
	if ( ec )
	{
		LogErrorStream << "Failure in determining the size of " << absolute( inputPath ) << " : "  << ec.message() << endl;
		return false;
	}

	ofstream fout( inputPath.string(), ios::binary );
	if ( !fout )
	{
		LogErrorStream << "Failure in opening " << absolute( inputPath ) << " : " << strerror( errno ) << endl;
		return false;
	}

	vector<unsigned char> buffer( inputFileSize );

	////////////////////////////////////////////////////////////////////
	// Overwrite file with ASCII 255 and ASCII 0 multiple times alternatingly
	int iterations = 5;
	while ( iterations -- )
	{
		unsigned char c = ( iterations & 1 ) ? 255 : 0;
		for ( auto& bufferElement : buffer )
		{
			bufferElement = c;
		}
		fout.seekp( 0, ios::beg );
		fout.write( ( char* )&buffer[0], buffer.size() );
		fout.flush();
	}
	////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
	// Overwrite file with random characters
	for ( auto& bufferElement : buffer )
	{
		bufferElement = rand() % 128;
	}
	fout.seekp( 0, ios::beg );
	fout.write( ( char* )&buffer[0], buffer.size() );
	fout.flush();
	////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
	// Overwrite file with null characters
	for ( auto& bufferElement : buffer )
	{
		bufferElement = 0;
	}
	fout.seekp( 0, ios::beg );
	fout.write( ( char* )&buffer[0], buffer.size() );
	fout.flush();
	////////////////////////////////////////////////////////////////////

	////////////////////////////////////////////////////////////////////
	// Change file size to 0
	fout.close();
	fout.open( inputPath.string(), ios::binary );
	////////////////////////////////////////////////////////////////////

	fout.close();
	return true;
}

// Shred a file
static void ShredFile( const path& inputPath )
{
	if ( !WriteRandomData( inputPath ) )
	{
		return;
	}

	path newPath = RandomRename( inputPath );
	boost::system::error_code ec;
	remove( newPath, ec );
	if ( ec )
	{
		LogErrorStream << "Failure in removing " << newPath << " : " << ec.message() << endl;
	}
}

// Confirm shredding a file
static bool ConfirmShred( const path& inputPath )
{
	cout << "Shred " << absolute( inputPath ) << " ? ( y/n ) ";
	bool confirm = ( cin.get() == 'y' );

	// Clear any trailing input
	cin.clear();
	cin.ignore( numeric_limits<streamsize>::max(), '\n' );
	return confirm;
}

// Shred a file/folder
static void Shred( const path& inputPath )
{
	Log << absolute( inputPath ) << endl;

	boost::system::error_code ec;
	if ( is_directory( inputPath, ec ) )
	{
		vector<path> dirContents;
		try
		{
			DirectoryIterate( inputPath, dirContents );
		}
		catch ( const filesystem_error& ex )
		{
			LogErrorStream << ex.what() << endl;
			return; // Not able to list the directory's contents. No point in proceeding further.
		}

		for ( const auto& item : dirContents )
		{
			Shred( item );
		}

		ec.clear();
		remove( inputPath, ec ); // Remove the directory itself. Will fail if its not empty.
		if ( ec )
		{
			LogErrorStream << "Failure in removing " << absolute( inputPath ) << " : " << ec.message() << endl;
		}
	}
	else // is a file
	{
		if ( ec )
		{
			LogErrorStream << "Failure in determining if " << absolute( inputPath ) << " is a directory or not : " << ec.message() << endl;
			return;
		}
		else
		{

			if ( ConfirmShred( inputPath ) )
			{
				ShredFile( inputPath );
			}
			else
			{
				Log << "Skipping " << absolute( inputPath ) << endl;
			}
		}
	}
}

int main( int argc, char** argv )
{
	const path defaultLogFilePath = "FileShredderLog.txt";
	if ( argc < 2 )
	{
		cout << "Usage : " << argv[0] << " <input_path> [log_file_path=" << defaultLogFilePath << "]\n";
		return -1;
	}

	const path LogFilePath = ( ( argc >= 3 ) ? path( argv[2] ) :  defaultLogFilePath );
	Log.open( LogFilePath.string() );
	if ( !Log )
	{
		cerr << "Error creating " << absolute( LogFilePath ) << " : " << strerror( errno ) << endl;
		return -1;
	}

	Shred( argv[1] );

	if ( Log )
	{
		if ( LogErrorStream.str().empty() )
		{
			cout << "The program ran without any errors.\n";
		}
		else
		{
			Log << "\nERRORS -:\n\n" << LogErrorStream.str() << endl;
			cout << "There were some errors during the execution of this program !\n\nCheck " << absolute( LogFilePath ) << " for details.\n";
			return -1;
		}
	}
}
