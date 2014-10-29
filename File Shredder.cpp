// File Shredder.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
using namespace std;


template<typename T>
string to_string( const T obj )  // For Debugging
{
    stringstream ss;
    ss << obj;
    string ret;
    getline( ss, ret );
    return ret.c_str();
}
#define debug( obj ) MessageBox( NULL, ( #obj" = \"" + to_string( obj ) + "\"").c_str(), ( "Line #" + to_string( __LINE__ ) ).c_str(), MB_OKCANCEL | MB_SYSTEMMODAL | MB_ICONINFORMATION)

// Retrieve the system error message for the last-error code
void DisplayErrorBox( LPTSTR lpszFunction, const DWORD dw = GetLastError() )
{

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;

    FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   NULL,
                   dw,
                   MAKELANGID( LANG_NEUTRAL, SUBLANG_DEFAULT ),
                   ( LPTSTR ) &lpMsgBuf,
                   0, NULL );

    // Display the error message and clean up

    lpDisplayBuf = ( LPVOID )LocalAlloc( LMEM_ZEROINIT,
                                         ( lstrlen( ( LPCTSTR )lpMsgBuf ) + lstrlen( ( LPCTSTR )lpszFunction ) + 40 ) * sizeof( TCHAR ) );

    StringCchPrintf( ( LPTSTR )lpDisplayBuf,
                     LocalSize( lpDisplayBuf ) / sizeof( TCHAR ),
                     TEXT( " % s failed with error % d: %s" ),
                     lpszFunction, dw, lpMsgBuf );

    MessageBox( NULL, ( LPCTSTR )lpDisplayBuf, TEXT( "Error" ), MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );

    LocalFree( lpMsgBuf );
    LocalFree( lpDisplayBuf );

}

long long GetFileSize( const string& FilePath ) // Returns the size of a file in bytes
{
    ifstream fin( FilePath, ios::in | ios::ate | ios::binary );
    if ( !fin.good() )
    {
        MessageBox( NULL, ( "Error opening file \"" + FilePath + "\" : " + strerror( errno ) ).c_str(), __FUNCTION__, MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
        return -1;
    }
    return fin.tellg();
}

string get_parent_directory( const string& FileName )
{
    size_t pos = FileName.find_last_of( "\\/" );
    return ( pos == string::npos ) ? "" : FileName.substr( 0, pos );
}

string RenameFile( const string& FileName ) // Renames the input to a random name
{
    int iterations = 10, max_iterations = 50;
    string ret = FileName;
    string ParentDirectory = get_parent_directory( ret );
    ParentDirectory += ( ParentDirectory.size() ? "\\" : "" );
    while ( iterations-- && max_iterations-- )
    {
        size_t i, size = rand() % 10 + 5;
        string NewName( size + 4, '*' );
        for ( i = 0; i < size; i++ )
        {
            NewName[i] = rand() % 26 + 'a';
        }
        NewName[i++] = '.';
        NewName[i++] = rand() % 26 + 'a';
        NewName[i++] = rand() % 26 + 'a';
        NewName[i++] = rand() % 26 + 'a';
        NewName = ParentDirectory + NewName;

        if ( rename( ret.c_str(), NewName.c_str() ) == 0 ) // Success
        {
            ret = NewName;
        }
        else
        {
            iterations++;
        }
        max_iterations--;
    }
    return ret;
}

string RenameDir( const string& DirName )  // Renames the input to a random name
{
    int iterations = 10, max_iterations = 50;
    string ret = DirName;
    string ParentDirectory = get_parent_directory( ret );
    ParentDirectory += ( ParentDirectory.size() ? "\\" : "" );
    while ( iterations-- && max_iterations-- )
    {
        size_t size = rand() % 10 + 5;
        string NewName( size, '*' );
        for ( size_t i = 0; i < size; i++ )
        {
            NewName[i] = rand() % 26 + 'a';
        }
        NewName = ParentDirectory + NewName;

        if ( rename( ret.c_str(), NewName.c_str() ) == 0 ) // Success
        {
            ret = NewName;
        }
        else
        {
            iterations++;
        }
        max_iterations--;
    }
    return ret;
}

void ChangeFileTime( const string& FileName )  // Changes the file creation time to a random number
{
    SYSTEMTIME current_system_time;
    GetSystemTime( &current_system_time );

    current_system_time.wMilliseconds = rand() % 60;
    current_system_time.wSecond = rand() % 60;
    current_system_time.wMinute = rand() % 60;
    current_system_time.wHour = rand() % 24;
    current_system_time.wDayOfWeek = rand() % 7;
    current_system_time.wDay = ( rand() % 28 ) + 1;
    current_system_time.wMonth = ( rand() % 12 ) + 1;
    current_system_time.wYear -= rand() % 5 + 1;

    FILETIME new_file_time;
    if ( SystemTimeToFileTime( &current_system_time, &new_file_time ) == 0 )
    {
        DisplayErrorBox( "SystemTimeToFileTime" );
        return;
    }

    HANDLE FileHandle = CreateFile( FileName.c_str(), FILE_WRITE_ATTRIBUTES, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL );
    if ( FileHandle == INVALID_HANDLE_VALUE )
    {
        DisplayErrorBox( "CreateFile" );
        return;
    }
    if ( SetFileTime( FileHandle, &new_file_time, &new_file_time, &new_file_time ) == 0 )
    {
        DisplayErrorBox( "SetFileTime" );
        return;
    }
    if ( CloseHandle( FileHandle ) == 0 )
    {
        DisplayErrorBox( "CloseHandle" );
        return;
    }
}

ofstream ShredLog( "File Shredder Log.txt", ios::app ); // Logs the names of the files/directories being deleted along with their new names just before deletion

string ShredFile( const string& FilePath )  // Shreds the input file and returns the new name of the file just before deletion
{
    long long FileSize = GetFileSize( FilePath );
    if ( FileSize == -1 )
    {
        return FilePath;
    }

    ofstream fout( FilePath, ios::binary );
    if ( !fout.good() )
    {
        MessageBox( NULL, ( "Error opening file \"" + FilePath + "\" : " + strerror( errno ) ).c_str(), __FUNCTION__, MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
        return FilePath;
    }

    int iterations = 35;

    vector<unsigned char> buffer( ( size_t )FileSize );
    while ( iterations -- )
    {
        unsigned char c = ( iterations & 1 ) ? 255 : 0;
        for ( long long i = 0; i < FileSize; i++ )
        {
            buffer[( size_t )i] = c;
        }
        fout.seekp( 0, ios::beg );
        fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with ASCII 255 and ASCII 0
        fout.flush();
    }

    fout.seekp( 0, ios::beg );
    for ( long long i = 0; i < FileSize; i++ )
    {
        buffer[( size_t )i] = rand() % 128;
    }
    fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with random characters
    fout.flush();

    fout.seekp( 0, ios::beg );
    for ( long long i = 0; i < FileSize; i++ )
    {
        buffer[( size_t )i] = 0;
    }
    fout.write( ( char* )&buffer[0], buffer.size() ); // Overwrite file with Null characters
    fout.flush();

    fout.close();

    fout.open( FilePath, ios::binary ); // Changes the file size to 0
    fout.close();

    string NewName = RenameFile( FilePath ); // Rename File
    ChangeFileTime( NewName );  // Change File-time

    if ( remove( NewName.c_str() ) )
    {
        MessageBox( NULL, ( "Error removing file \"" + FilePath + "\" : " + strerror( errno ) ).c_str(), __FUNCTION__, MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
        return NewName;
    }
    return NewName;
}

string Shred( const string& FilePath )  // Shreds the input file/directory and returns the new name of the file/directory just before deletion
{
    if ( FilePath.empty() )
    {
        return FilePath;
    }

    if ( SetFileAttributes( FilePath.c_str(), FILE_ATTRIBUTE_NORMAL ) == 0 ) // removing attributes like "hidden" which are not compatible with ofstream
    {
        DisplayErrorBox( "SetFileAttributes" );
    }

    ifstream test( FilePath );
    if ( test )  // is a valid file
    {
        test.close();
        // Ask for confirmation
        if ( MessageBox( NULL, ( "Shred file \"" + FilePath + "\" ? " ).c_str(), "Confirm shredding !!", MB_OKCANCEL | MB_SYSTEMMODAL | MB_ICONWARNING ) == IDOK )
        {
            string NewName = ShredFile( FilePath );
            ShredLog << " -File- \"" << FilePath << "\" -> \"" << NewName << "\"" << endl;
            return NewName;
        }
        return FilePath;
    }

    //// Check whether a valid directory or not  ////
    if ( FilePath.size() > MAX_PATH - 3 )
    {
        MessageBox( NULL, ( "Error : The length of the directory name \"" + FilePath + "\" ( " + to_string( FilePath.size() ) + " ) is greater than " + to_string( MAX_PATH - 3 ) + "." ).c_str(), __FUNCTION__, MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
        return FilePath;
    }

    // Ask for confirmation
    if ( MessageBox( NULL, ( "Shred directory \"" + FilePath + "\" ? " ).c_str(), "Confirm shredding !!", MB_OKCANCEL | MB_SYSTEMMODAL | MB_ICONWARNING ) != IDOK )
    {
        return FilePath;
    }

    WIN32_FIND_DATA file_info;
    DWORD dwError = 0;
    HANDLE hFind = FindFirstFile( ( FilePath + "\\*" ).c_str(), &file_info );

    if ( hFind == INVALID_HANDLE_VALUE )  // Invalid directory
    {
        MessageBox( NULL, ( "Error : \"" + FilePath + "\" not a valid file or directory." ).c_str(), __FUNCTION__, MB_OK | MB_SYSTEMMODAL | MB_ICONERROR );
        return FilePath;
    }
    else // is a valid directory
    {
        do  // Loop over all the files of the directory
        {
            if ( string( file_info.cFileName ) != "." &&  string( file_info.cFileName ) != ".." )  // Ignore current and parent directory
            {
                Shred( FilePath + "\\" + file_info.cFileName );
            }
        }
        while ( FindNextFile( hFind, &file_info ) != 0 );

        dwError = GetLastError();
        if ( dwError != ERROR_NO_MORE_FILES )
        {
            DisplayErrorBox( TEXT( "FindFirstFile" ) );
            return FilePath;
        }
        FindClose( hFind );

        string NewName = RenameDir( FilePath );

        ShredLog << " -Directory- \"" << FilePath << "\" -> \"" << NewName << "\"" << endl;

        if ( RemoveDirectory( NewName.c_str() ) == 0 )
        {
            DisplayErrorBox( "RemoveDirectory" );
            return NewName;
        }
        return NewName;
    }
    return FilePath;
}

const string currentDateTime()
{
    time_t now = time( 0 );
    tm tstruct;
    char buf[80];
    tstruct = *localtime( &now );
    strftime( buf, sizeof( buf ), "%Y-%m-%d.%X", &tstruct );
    return buf;
}

int main()
{
    ShredLog << currentDateTime() << endl;

    srand( ( unsigned int )time( NULL ) );

    cout << "Enter file location \n";
    string FilePath;
    getline( cin, FilePath );

    Shred( FilePath );

    MessageBox( NULL, "Shredding complete!\n", "Information", MB_OK | MB_SYSTEMMODAL | MB_ICONINFORMATION );
}
