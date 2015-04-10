#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <vector>
#include <cstdio>
#include <cstdlib>
#define BOOST_FILESYSTEM_NO_DEPRECATED
#include <boost/filesystem.hpp>

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

// Generates a random file name
static string GenerateRandomFileName()
{
	return unique_path().string();
}

// Renames the input file to a random string
static path RandomRename( const path& inputPath )
{
	const size_t maxRenameAttempts = 10; // Max number of attempts to make in renaming a file.
	// It is possible that the random filename generator returns a name
	// which is same as that of a file in the current directory,
	// or is a reserved filename ( like "com1" on windows )
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
// Returns true if successful, false otherwise
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
	// Overwrite file with ASCII 255 and ASCII 0 multiple times alternately
	int iterations = 5;
	while ( iterations -- )
	{
		const unsigned char c = ( iterations & 1 ) ? 255 : 0;
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

	const path newPath = RandomRename( inputPath );
	boost::system::error_code ec;
	remove( newPath, ec );
	if ( ec )
	{
		LogErrorStream << "Failure in removing " << newPath << " : " << ec.message() << endl;
	}
}

// Confirm shredding a file
// Returns true if the user confirms, false otherwise
static bool ConfirmShred( const path& inputPath )
{
	cout << "Shred " << absolute( inputPath ) << " ? ( y/n ) ";
	const bool confirm = ( cin.get() == 'y' );

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
		remove( inputPath, ec ); // Remove the input directory itself.. Will fail if not empty
		if ( ec )
		{
			LogErrorStream << "Failure in removing " << absolute( inputPath ) << " : " << ec.message() << endl;
		}
	}
	else if ( !ec ) // input is a file
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
	else
	{
		LogErrorStream << "Failure in determining if " << absolute( inputPath ) << " is a directory or not : " << ec.message() << endl;
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
