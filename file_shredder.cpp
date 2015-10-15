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

// File handles for log files
static ofstream logError, logInfo;

typedef unsigned long long ULL;

// Returns the number of files in the input directory
// The progress status is printed after every `milestone` files are visited.
// Here "file" refers to regular files
ULL GetNumberOfFiles( const path& inputPath = ".", const ULL& milestone = 1000 )
{
	if ( !is_directory( inputPath ) )
	{
		return 1;
	}

	ULL numberOfFiles = 0;
	if ( milestone == 0 )
	{
		numberOfFiles = std::distance( recursive_directory_iterator( inputPath ), recursive_directory_iterator() );
	}
	else
	{
		ULL filesVisitedIterator = 0;
		for ( recursive_directory_iterator it( inputPath ); it != recursive_directory_iterator(); ++it )
		{
			if ( is_regular_file( *it ) )
			{
				filesVisitedIterator++;
				if ( filesVisitedIterator == milestone )
				{
					numberOfFiles += filesVisitedIterator;
					filesVisitedIterator = 0;
					cout << "\rFiles visited = " << numberOfFiles << std::flush;
				}
			}
		}

		numberOfFiles += filesVisitedIterator;
		if ( filesVisitedIterator )
		{
			cout << "\rFiles visited = " << numberOfFiles << std::flush;
		}

		cout << endl;
	}

	return numberOfFiles;
}


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
			if ( ec )
			{
				// Maybe a reserved filename was generated
				logError << "Failure in renaming " << absolute( inputPath ) << " to " << absolute( newPath ) << " : " << ec.message() << endl;
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
		logError << "Failure in determining the size of " << absolute( inputPath ) << " : "  << ec.message() << endl;
		return false;
	}

	ofstream fout( inputPath.string(), ios::binary );
	if ( !fout )
	{
		logError << "Failure in opening " << absolute( inputPath ) << " : " << strerror( errno ) << endl;
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
		logError << "Failure in removing " << newPath << " : " << ec.message() << endl;
	}
}

// Confirm shredding a file
// Returns true if the user confirms, false otherwise
static bool ConfirmShred( const path& inputPath )
{
	static ULL fileCount = 1;
	cout << "\n" << fileCount++ << ". Shred " << absolute( inputPath ) << " ?\n( y/n ) ";
	const bool confirm = ( cin.get() == 'y' );

	// Clear any trailing input
	cin.clear();
	cin.ignore( numeric_limits<streamsize>::max(), '\n' );
	return confirm;
}

// Shred a file/folder
static void Shred( const path& inputPath )
{
	logInfo << absolute( inputPath ) << endl;

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
			logError << ex.what() << endl;
			return; // Not able to list the directory's contents. No point in proceeding further.
		}

		for ( const auto& item : dirContents )
		{
			Shred( item );
		}

		ec.clear();
		// Remove the input directory itself
		// Will fail if not empty
		remove( inputPath, ec );
		if ( ec )
		{
			logError << "Failure in removing " << absolute( inputPath ) << " : " << ec.message() << endl;
		}
	}
	// input is a file
	else if ( !ec )
	{
		if ( ConfirmShred( inputPath ) )
		{
			logInfo << "Shredding " << absolute( inputPath ) << endl;
			ShredFile( inputPath );
		}
		else
		{
			logInfo << "Skipping " << absolute( inputPath ) << endl;
		}
	}
	else
	{
		logError << "Failure in determining if " << absolute( inputPath ) << " is a directory or not : " << ec.message() << endl;
	}
}

int main( int argc, char** argv )
{
	path logFolderPath = "./logs";

	if ( argc < 2 )
	{
		cout << "Usage : " << argv[0] << " <input_path> [log_file_folder=" << logFolderPath << "]\n";
		return -1;
	}

	if ( argc > 2 )
	{
		logFolderPath = path( argv[2] );
	}
	try
	{
		const path inputPath = canonical( argv[1] );

		create_directories( logFolderPath );
		const path errorLogPath = logFolderPath / "errors.txt";
		logError.open( errorLogPath.string() );
		const path infoLogPath = logFolderPath / "info.txt";
		logInfo.open( infoLogPath.string() );

		cout << "Calculating number of files to shred ...\n";
		const ULL filesInInputPath = GetNumberOfFiles( inputPath );
		cout << "Files to shred = " << filesInInputPath << "\n";

		cout << "\nInitiating the shredding process ...\n\n";
		Shred( inputPath );
		cout << "\n\n";

	}
	catch ( const filesystem_error& ex )
	{
		cerr << ex.what() << endl;
		logError << ex.what() << endl;
	}

}

