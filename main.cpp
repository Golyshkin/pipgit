#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <QProcess>
#include <QDebug>

using namespace std;

typedef enum
{
   PIPGIT_STATE_NONE,
   PIPGIT_STATE_INSPECTION,
   PIPGIT_STATE_BR

} PIPGIT_STATE_T;

typedef struct
{
   QString fileName;
   QString changeType;
   unsigned int added;
   unsigned int deleted;

} PIPGIT_FILE_ITEM_T;

typedef struct
{
   int commitNumber;
   QString shaid;
   QString autorName;
   QString autorEmail;
   QString commitDate;
   QString commitDesc;

   QList< PIPGIT_FILE_ITEM_T > files;

} PIPGIT_COMMIT_ITEM_T;

typedef struct
{
   bool colors;
   bool detailedStat;

} PIPGIT_CONFIG_T;

const char *PIPGIT_VER = "0.6.5";
const int BUF_STR_SIZE = 255;

const char *PIPGIT_FOLDER = "/pipgit";
const char *PIPGIT_INSP_LOG = "pipgit.inspection.log";
const char *PIPGIT_BR_LOG   = "pipgit.br.log";

char c[256] = { 0 };
ifstream is;
bool isProcced = false;
unsigned int totalChanged = 0;
PIPGIT_CONFIG_T gConfig = { true, true };
QList< PIPGIT_COMMIT_ITEM_T > gCommitList;
QList< PIPGIT_FILE_ITEM_T > gSummaryList;
PIPGIT_COMMIT_ITEM_T gSummaryBR;

QString workPath = QDir::tempPath() + PIPGIT_FOLDER;
QDir workDir( workPath );

void Usage();
void CopyRight( PIPGIT_STATE_T aState );
void CleanUp();
QString GetCurrentBranch();
bool Configure();
void SetConfig( int &argc, char *argv[] );
void GetTotalDiff( QString aSHA1, QString aSHA2 = QString(), PIPGIT_STATE_T aType = PIPGIT_STATE_INSPECTION );
void GetDetailedDiff( QString aSHA1, QString aSHA2 = QString() );
void PrintInspectionDetails();
void GetBRDetailes();

void PrintString( QString aStr1, QString aStr2, int aColor, bool aPrintToLog, int aWith = 20 );
void PrintFilesHeader();
void PrintFiles( int aAdded, int aDeleted, QString aFileName );
void PrintComparingStatus( QString aSHA1, QString aSHA2 );
void PrintInspection();
void PrintBR();

int main( int argc, char *argv[] )
{
   PIPGIT_STATE_T state = PIPGIT_STATE_NONE;
   bool argsStatus = true;

   SetConfig( argc, argv );

   if ( argc < 3 )
   {
      argsStatus = false;
   }
   else
   {
      if ( ( argc == 3 && strlen( argv[2] ) < 10 ) ||
           ( argc == 4 && strlen( argv[3] ) < 10 ) )
      {
         argsStatus = false;
      }
   }

   if ( argsStatus == false )
   {
      CopyRight( state );
      Usage();

      return 0;
   }

   // Configuring cout & cerr
   cout.setf( ios::left, ios::adjustfield );
   cerr.setf( ios::left, ios::adjustfield );

   ofstream	ferr;
   streambuf *save_sbuf_cerr = ferr.rdbuf();
   streambuf *old_sbuf_cerr = cerr.rdbuf();
   cerr.rdbuf( save_sbuf_cerr );

   QString arg1 = argv[1];
   QString arg3 = (argc == 4) ? argv[3] : "";

   if ( arg1 == "insp" )
   {
      ferr.open ( PIPGIT_INSP_LOG );

      if ( Configure() == true )
      {
         GetDetailedDiff( argv[2], arg3 );
         GetTotalDiff( argv[2], arg3 );
         PrintInspection();
      }
   }
   else if ( arg1 == "br" )
   {
      ferr.open ( PIPGIT_BR_LOG );

      if ( Configure() == true )
      {
         GetDetailedDiff( argv[2], argv[3] );
         GetTotalDiff( argv[2], argv[3], PIPGIT_STATE_BR );
         PrintBR();
      }
      else
      {
         Usage();
      }
   }
   else
   {
      CopyRight( state );
      Usage();
   }

   // To avoid of segmentation fault we need to restore cerr stream pointer
   ferr.close();
   cerr.rdbuf( old_sbuf_cerr );

   CleanUp();

   return 0;
}

void
SetConfig( int &aArgc, char *aArgv[] )
{
   // Checking .pipgit
   QFile configFile( QDir::homePath() + "/.pipgit" );
   int argc = aArgc;

   if ( configFile.exists() == true )
   {
      if ( configFile.open(QIODevice::ReadOnly | QIODevice::Text ) )
      {
         while (!configFile.atEnd())
         {
            QByteArray rawLine = configFile.readLine();

            QString lineStr( rawLine.data() );
            lineStr = lineStr.simplified();

            QStringList configPair( lineStr.split( "=" ) );

            if ( configPair.count() == 2 )
            {
               if ( configPair[0].toLower() == "colors" && configPair[1].toLower() == "no" )
               {
                  gConfig.colors = false;
               }

               if ( configPair[0].toLower() == "details" && configPair[1].toLower() == "no" )
               {
                  gConfig.detailedStat = false;
               }
            }
         }
      }
   }

   // Checking ARGV overides .pipgit
   for ( int index = 0; index < argc; index++ )
   {
      QString argString = aArgv[ index ];
      QStringList curArg = argString.split( "=" );

      if ( curArg.count() == 2 )
      {
         --aArgc;

         if ( curArg[0].toLower() == "--details" )
         {
            gConfig.detailedStat = ( curArg[1].toLower() == "no" ) ? false : true;
         }

         if ( curArg[0].toLower() == "--colors" )
         {
            gConfig.colors = (curArg[1].toLower() == "no" ) ? false : true;
         }
      }
   }
}

bool
Configure()
{
   if ( !workDir.exists() && workDir.mkdir( workPath ) == false )
   {
      cout << endl << QString( "Error: Couldn't create [%1] directory..." ).arg( workPath ).toStdString().c_str();

      return false;
   }

   return true;
}

void PrintInspection()
{
   unsigned int index = 0;

   if ( gConfig.detailedStat == true )
   {
      foreach ( PIPGIT_COMMIT_ITEM_T commit, gCommitList )
      {
         PrintString( "Commit No:", QString( "[%1]" ).arg( ++index ), 32, true );
         PrintString( "GIT Commit SHA ID:", commit.shaid, 32, true );
         PrintString( "Developer Name:", commit.autorName, 33, true );
         PrintString( "Developer E-mail:", commit.autorEmail, 33, true );
         PrintString( "Commit Date:", commit.commitDate, 32, true );
         PrintString( "Commit Description:", commit.commitDesc, 32, true );

         cout << endl;
         cerr << endl;;

         PrintFilesHeader();

         foreach ( PIPGIT_FILE_ITEM_T file, commit.files )
         {
            PrintFiles( file.added, file.deleted, file.fileName );
         }

         cout << endl;
         cerr << endl;;
      }
   }

   cout << "TOTAL CHANGES FOR [" << gCommitList.count() << "] COMMIT(s):" << endl;
   cerr << "TOTAL CHANGES FOR [" << gCommitList.count() << "] COMMIT(s):" << endl;

   PrintFilesHeader();

   foreach ( PIPGIT_FILE_ITEM_T item, gSummaryList )
   {
      PrintFiles( item.added, item.deleted, item.fileName );
   }

   PrintInspectionDetails();
}

void PrintBR()
{
   PrintString( "GIT Commit SHA ID:", gCommitList[0].shaid, 32, true );
   PrintString( "Developer Name:", gCommitList[0].autorName, 33, true );
   PrintString( "Developer E-mail:", gCommitList[0].autorEmail, 33, true );
   PrintString( "Commit Date:", gCommitList[0].commitDate, 32, true );
   PrintString( "Commit Description:", gCommitList[0].commitDesc, 32, true );
   PrintString( "Branch:", QString( "[<color>%1</color>]" ).arg( GetCurrentBranch() ), 32, true );

   cout << endl;
   cerr << endl;;

   cerr << "----------------------------------------------------------------------------------------------" << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;


   cout << "FILES:" << endl;
   cerr << "FILES:" << endl;

   cout << "----------------------------------------------------------------------------------------------" << endl;
   cerr << "----------------------------------------------------------------------------------------------" << endl;

   foreach ( PIPGIT_FILE_ITEM_T item, gSummaryBR.files )
   {
      PrintString( item.changeType, item.fileName, 32, true, 14 );
   }

   cout << "----------------------------------------------------------------------------------------------" << endl;
   cerr << "----------------------------------------------------------------------------------------------" << endl;

   cout << endl << "TESTED: [Yes/No]" << endl << "COMMENT:" << endl << endl;
   cerr << endl << "TESTED: [Yes/No]" << endl << "COMMENT:" << endl << endl;
}

void
PrintString( QString aStr1, QString aStr2, int aColor, bool aPrintToLog, int aWith )
{
   if ( aPrintToLog )
   {
      QString str( aStr2 );

      str.replace( "<color>",  "" );
      str.replace( "</color>", "" );

      cerr << setw( aWith ) << aStr1.toStdString().c_str() << str.toStdString().c_str() << endl;
   }

   if ( gConfig.colors == true )
   {
#ifdef PIPGIT_LINUX
      aStr2.replace( "<color>", QString( "\033[1;%1m" ).arg( aColor ) );
      aStr2.replace( "</color>", "\033[00m" );
#endif
   }

   aStr2.replace( "<color>", ""  );
   aStr2.replace( "</color>", "" );

   cout << setw( aWith ) << aStr1.toStdString().c_str() << aStr2.toStdString().c_str() << endl;
}

void
PrintFilesHeader()
{
   PrintString( "----------------------------------------------------------------------------------------------", "",  0, true );

   if ( gConfig.colors == true )
   {
#ifdef PIPGIT_LINUX
      cout << "\033[1;32mAdded:\033[00m    |    \033[1;31mDeleted:\033[00m   |   Changed File:" << endl;
#else
      cout << "Added:    |    Deleted:   |   Changed File:" << endl;
#endif
   }
   else
   {
      cout << "Added:    |    Deleted:   |   Changed File:" << endl;
   }

   cerr << "Added:    |    Deleted:   |   Changed File:" << endl;

   PrintString( "----------------------------------------------------------------------------------------------", "",  0, true );
}

void
Print2ColorText( QString aStr1, QString aStr2, int aColor, int aColor2 )
{
   QString str1( aStr1 ), str2( aStr2 );;

   str1.replace( "<color>",  "" );
   str1.replace( "</color>", "" );
   str2.replace( "<color>",  "" );
   str2.replace( "</color>", "" );

   cerr << str1.toStdString().c_str() << str2.toStdString().c_str();

   if ( gConfig.colors == true )
   {
#ifdef PIPGIT_LINUX
      aStr1.replace( "<color>", QString( "\033[1;%1m" ).arg( aColor ) );
      aStr1.replace( "</color>", "\033[00m" );

      aStr2.replace( "<color>", QString( "\033[1;%1m" ).arg( aColor2 ) );
      aStr2.replace( "</color>", "\033[00m" );

      cout << aStr1.toStdString().c_str() << aStr2.toStdString().c_str();
#else
   cout << str1.toStdString().c_str() << str2.toStdString().c_str();
#endif
   }
   else
   {
      cout << str1.toStdString().c_str() << str2.toStdString().c_str();
   }
}

void
PrintFiles( int aAdded, int aDeleted, QString aFileName )
{
   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << " \033[1;32m" << setw(15) << aAdded << "\033[00m" << "\033[1;31m" << setw(15) << aDeleted << "\033[00m" << std::setw( 15 ) << aFileName.toStdString().c_str() << endl;
      #else
      cout << " " << setw(15) << aAdded << setw(15) << aDeleted<< std::setw( 15 ) << aFileName.toStdString().c_str() << endl;
      #endif
   }
   else
   {
      cout << " " << setw(15) << aAdded << setw(15) << aDeleted<< std::setw( 15 ) << aFileName.toStdString().c_str() << endl;
   }

   cerr << " " << setw(15) << aAdded << setw(15) << aDeleted<< std::setw( 15 ) << aFileName.toStdString().c_str() << endl;
}

void
PrintComparingStatus( QString aSHA1, QString aSHA2 )
{
   if ( aSHA1.length() > 0 && aSHA2.length() > 0 )
   {
      Print2ColorText( QString( "Comparing [<color>%1</color>]" ).arg( aSHA1 ), QString( " with [<color>%2</color>] ... " ).arg( aSHA2 ), 30, 30 );
   }
   else
   {
      Print2ColorText( QString( "Comparing [<color>%1</color>]" ).arg( aSHA1 ), " with latest checked out commit ... ", 30, 30 );
   }

   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << "[ \033[1;32mDONE\033[00m ]" << endl << endl;
      #else
      cout << "[ DONE ]" << endl << endl;
      #endif
   }
   else
   {
      cout << "[ DONE ]" << endl << endl;
   }

   cerr << "[ DONE ]" << endl << endl;
}

void
GetDetailedDiff( QString aSHA1, QString aSHA2 )
{
   QProcess proc;
   QString fileToSave = workPath + "/detailed_diff.log";
   char tmpBuf[ BUF_STR_SIZE + 1 ] = { 0 };
   PIPGIT_COMMIT_ITEM_T commitItem;

   QString cmd = QString( "git log --numstat --pretty=format:\"Commit No:%nSHAID:%H%nAutor Name:%an%nAutor E-mail:%ae%nCommit Date:%aD%nDescription:%s%n\" %1...%2 >%3/detailed_diff.log" ).arg( aSHA1 ).arg( aSHA2 ).arg( workPath );

   #ifdef PIPGIT_DEBUG
   cout << cmd.toStdString().c_str() << endl;
   #endif

   CopyRight( PIPGIT_STATE_INSPECTION );
   PrintComparingStatus( aSHA1, aSHA2 );

   system( cmd.toStdString().c_str() );

   is.open( fileToSave.toStdString().c_str() );

   while ( is.good() )     // loop while extraction from file is possible
   {
      is.getline( &tmpBuf[0], BUF_STR_SIZE );

      QString curLine( &tmpBuf[0] );

      if ( curLine.contains( "Commit No" ) )
      {
         if ( commitItem.files.count() > 0 )
         {
            gCommitList.append( commitItem );
            commitItem.files.clear();
         }
      }
      else if ( curLine.contains( "SHAID" ) )
      {
         QStringList values( curLine.split( ":" ) );

         commitItem.shaid = QString( "[<color>%1</color>]" ).arg( values[ 1 ] );
      }
      else if ( curLine.contains( "Autor Name" ) )
      {
         QStringList values( curLine.split( ":" ) );

         commitItem.autorName = QString( "[<color>%1</color>]" ).arg( values[ 1 ] );
      }
      else if ( curLine.contains( "Autor E-mail" ) )
      {
         QStringList values( curLine.split( ":" ) );

         commitItem.autorEmail = QString( "[<color>%1</color>]" ).arg( values[ 1 ] );
      }
      else if ( curLine.contains( "Commit Date" ) )
      {
         QStringList values( curLine.split( ":" ) );

         commitItem.commitDate = QString( "[<color>%1</color>]" ).arg( values[ 1 ] );
      }
      else if ( curLine.contains( "Description" ) )
      {
         QStringList values( curLine.split( ":" ) );

         commitItem.commitDesc = QString( "[<color>%1</color>]" ).arg( values[ 1 ] );
      }
      else // File names
      {
         QStringList values( curLine.split( "\t" ) );

         if ( values.count() == 3 )
         {
            PIPGIT_FILE_ITEM_T item;

            item.added    = values[0].toInt();
            item.deleted  = values[1].toInt();
            item.fileName = values[2].simplified();

            commitItem.files.append( item );
         }
      }
   }

   // Checking Last commit
   if ( commitItem.files.count() > 0 )
   {
      gCommitList.append( commitItem );
      commitItem.files.clear();
   }

   is.close();
}

void
GetTotalDiff( QString aSHA1, QString aSHA2, PIPGIT_STATE_T aType )
{
   QProcess proc;
   gSummaryList.clear();
   QString cmd = "git diff %1 %2 %3";

   switch ( aType )
   {
   case PIPGIT_STATE_INSPECTION:
      cmd = cmd.arg( "--numstat " ).arg( aSHA1 ).arg( aSHA2 );
      break;
   case PIPGIT_STATE_BR:
      cmd = cmd.arg( "--name-status " ).arg( aSHA1 ).arg( aSHA2 );
      break;
   default:
      break;
   }

   #ifdef PIPGIT_DEBUG
   cout << cmd.toStdString().c_str() << endl;
   #endif

   proc.start( cmd, QIODevice::ReadOnly );
   // Show process output
   proc.waitForReadyRead();

   QString tmpStr( proc.readAllStandardOutput().data() );

   proc.close();

   QStringList tmpStrList;

   tmpStrList = tmpStr.split( "\n" );

   if ( tmpStrList.count() > 0 )
   {
      foreach ( QString curSting, tmpStrList )
      {
         switch ( aType )
         {
         case PIPGIT_STATE_INSPECTION:
         {
            QStringList curItemList = curSting.split( "\t" );

            if ( curItemList.count() == 3 )
            {
               PIPGIT_FILE_ITEM_T itemInfo;

               itemInfo.added    = curItemList[0].simplified().toInt();
               itemInfo.deleted  = curItemList[1].simplified().toInt();
               itemInfo.fileName = curItemList[2].simplified();

               gSummaryList.append( itemInfo );
            }
            break;
         }
         case PIPGIT_STATE_BR:
         {
            QStringList curItemList = curSting.split( "\t" );

            if ( curItemList.count() == 2 )
            {
               PIPGIT_FILE_ITEM_T file;

               file.changeType = curItemList[0].simplified();

               if ( file.changeType.compare( "A", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Added";
               }
               else if ( file.changeType.compare( "C", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Copied";
               }
               else if ( file.changeType.compare( "D", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Deleted";
               }
               else if ( file.changeType.compare( "M", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Modified";
               }
               else if ( file.changeType.compare( "R", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Renamed";
               }
               else if ( file.changeType.compare( "T", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Changed Type";
               }
               else if ( file.changeType.compare( "U", Qt::CaseInsensitive ) == 0 )
               {
                  file.changeType = "Unmerged";
               }
               else
               {
                  file.changeType = "Unknown";
               }

               file.fileName   = curItemList[1].simplified();

               gSummaryBR.files.append( file );
            }

            break;
         }
         default:
            break;
         }
      }
   }

   return;
}

void
PrintInspectionDetails()
{
   int totalChanged = 0;

   foreach ( PIPGIT_FILE_ITEM_T item, gSummaryList )
   {
      if ( item.added == item.deleted )
      {
         totalChanged += item.added;
      }
      else
      {
         totalChanged += item.added + item.deleted;
      }
   }

   unsigned int inspectionTimeSecs  = totalChanged * 3600 / 150;
   unsigned int inspectionTimeHours = inspectionTimeSecs / 3600;
   unsigned int inspectionTimeMins  = ( inspectionTimeSecs - inspectionTimeHours * 3600 ) / 60;
   unsigned int inspectionsCount    = ( inspectionTimeHours / 2 );

   if ( totalChanged > 0 && totalChanged < 10 )
   {
      inspectionTimeMins = 5;
   }

   if ( inspectionsCount == 0 && totalChanged > 0)
   {
      inspectionsCount = 1;
   }
   else
   {
      inspectionsCount += inspectionTimeHours % 2 ? 1 : 0;
   }

   QString strInspHour = inspectionTimeHours >= 10 ? QString::number( inspectionTimeHours ) : QString( "0%1" ).arg( inspectionTimeHours );
   QString strInspMins = inspectionTimeMins  >= 10 ? QString::number( inspectionTimeMins ) : QString( "0%1" ).arg( inspectionTimeMins );
   QString strInspTime = QString( "%1:%2").arg( strInspHour).arg( strInspMins );

   cout << endl;

   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << "\033[1;30m==============================================================================================\033[00m" << endl;
      #else
      cout << endl << "==============================================================================================" << endl;
      #endif
   }
   else
   {
      cout << endl << "==============================================================================================" << endl;
   }

   cerr << endl << "==============================================================================================" << endl;

   cout << setw(26) << "Total Inspection Changes: "  << totalChanged << " LOC" << endl;
   cerr << setw(26) << "Total Inspection Changes: "  << totalChanged << " LOC" << endl;

   cout << setw(26) << "Total Inspection Time: " <<  strInspTime.toStdString() << endl;
   cerr << setw(26) << "Total Inspection Time: " <<  strInspTime.toStdString() << endl;

   cout << setw(26) << "Total Inspections:" << inspectionsCount << endl;
   cerr << setw(26) << "Total Inspections:" << inspectionsCount << endl;

   cerr << "==============================================================================================" << endl;

   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << "\033[1;30m==============================================================================================\033[00m" << endl;
      #else
      cout << "----------------------------------------------------------------------------------------------" << endl;
      #endif
   }
   else
   {
      cout << "==============================================================================================" << endl;
   }
}

QString
GetCurrentBranch()
{
   QProcess proc;
   proc.start( "git branch", QIODevice::ReadOnly );
   // Show process output
   proc.waitForReadyRead();

   QString branchesStr( proc.readAllStandardOutput().data() );

   proc.close();

   QStringList branches;

   branches = branchesStr.split( "\n" );

   foreach ( QString branch, branches )
   {
      if ( branch.contains( '*' ) == true )
      {
         return branch.mid( 2 );
      }
   }

   return "[ Unknown ]";
}

void
CleanUp()
{
   QStringList listToRemoveFiles = workDir.entryList();

   foreach ( QString filename, listToRemoveFiles )
   {
      if ( filename == "." || filename == ".." )
      {
         continue;
      }

      QFile::remove( workPath + QString( "/" ) + filename );
   }

   if ( workDir.exists() )
   {
      workDir.rmdir( workPath );
   }
}

void Usage()
{
   cout << "Usage:" << " pipgit <insp|br> <SHA ID1> [SHA ID2] [Options]" << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << "Parameter 1.   (insp|br) - Switch output information to Inspection or BR" << endl;
   cout << "Parameter 2,3. (SHA ID)  - Compare changes between SHA1 & SHA2" << endl;

   cout << endl << "Examples:" << endl;
   cout << "\'pipgit insp 7deac3c8436afa65a64f5567869f6b9d2a39a33e 7deac3c8436afa6535432442543445\' - Calculates changes between SHA1 & SHA2" << endl;
   cout << "\'pipgit insp 7deac3c8436afa65a64f5567869f6b9d2a39a33e\' - Calculates changes between selected SHA ID & latest commit" << endl;
   cout << "\'pipgit insp 7deac3c8436afa65a64f5567869f6b9d2a39a33e --details=no --colors=no\' - Calculates changes between selected SHA ID & latest commit without details & color output" << endl << endl;

   const char * const NOTE = "Note: Order of parameters SHA1 & SHA2 make sense for \'git diff\', so, be a careful during typing!";

   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << "\033[01;31mNote: Order of parameters SHA1 & SHA2 make sence for \'git diff\', so, be a careful during typing!\033[00m" << endl << endl;
      #else
      cout << NOTE << endl << endl;
      #endif
   }
   else
   {
      cout << NOTE << endl << endl;
   }
}

void CopyRight( PIPGIT_STATE_T aState )
{
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << QString( "PIPGIT util - v.%1 (Christmas Edition) Alexander.Golyshkin@teleca.com (c) 2011-2012" ).arg( PIPGIT_VER ).toStdString().c_str() << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;

   if ( gConfig.colors == true )
   {
      #ifdef PIPGIT_LINUX
      cout << "1. CR Tool Web-Page:         \033[1;33mhttps://bugtracking.teleca.com/Instances/IviCR\033[00m" << endl;
      cout << "2. Inspection Tool Web-Page: \033[1;33mhttps://bugtracking.teleca.com/Instances/IviInsp\033[00m" << endl;
      cout << "3. BR Tool Web-Page:         \033[1;33mhttps://bugtracking.teleca.com/Instances/IviBR\033[00m" << endl;
      cout << "----------------------------------------------------------------------------------------------" << endl;
      #else
      cout << "1. CR Tool Web-Page:         https://bugtracking.teleca.com/Instances/IviCR" << endl;
      cout << "2. Inspection Tool Web-Page: https://bugtracking.teleca.com/Instances/IviInsp" << endl;
      cout << "3. BR Tool Web-Page:         https://bugtracking.teleca.com/Instances/IviBR" << endl;
      cout << "----------------------------------------------------------------------------------------------" << endl;
      #endif
   }
   else
   {
      cout << "1. CR Tool Web-Page:         https://bugtracking.teleca.com/Instances/IviCR" << endl;
      cout << "2. Inspection Tool Web-Page: https://bugtracking.teleca.com/Instances/IviInsp" << endl;
      cout << "3. BR Tool Web-Page:         https://bugtracking.teleca.com/Instances/IviBR" << endl;
      cout << "----------------------------------------------------------------------------------------------" << endl;
   }

   if ( aState != PIPGIT_STATE_NONE )
   {
      cerr << "----------------------------------------------------------------------------------------------" << endl;
      cerr << QString( "PIPGIT util - v.%1 (Christmas Edition) Alexander.Golyshkin@teleca.com (c) 2011-2012" ).arg( PIPGIT_VER ).toStdString().c_str() << endl;
      cerr << "----------------------------------------------------------------------------------------------" << endl;
   }

   cout << endl;
   cerr << endl;
}
