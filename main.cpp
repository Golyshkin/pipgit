#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <QProcess>

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
   unsigned int added;
   unsigned int deleted;

} PIPGIT_ITEM_T;

typedef struct
{
   bool colors;
   bool detailedStat;

} PIPGIT_CONFIG_T;

const char *PIPGIT_VER = "0.6.4";
const char *TMP_FILE = "null";
const char *PIPGIT_FOLDER = "/pipgit";
const char *PIPGIT_INSP_LOG = "pipgit.inspection.log";
const char *PIPGIT_BR_LOG   = "pipgit.br.log";

char c[256] = { 0 };
ifstream is;
bool isProcced = false;
unsigned int totalChanged = 0;
PIPGIT_CONFIG_T config = { true, true };
QList< PIPGIT_ITEM_T > gItemslist;

QString workPath = QDir::tempPath() + PIPGIT_FOLDER;
QDir workDir( workPath );

void Usage();
void CopyRight( PIPGIT_STATE_T aState );
bool ExtractPatches( PIPGIT_STATE_T aState, int argc, char *argv[] );
void PrintInfo( PIPGIT_STATE_T aState );
void CleanUp();
QString GetCurrentBranch();
void SetConfig( int &argc, char *argv[] );
QList< PIPGIT_ITEM_T > GetDiff( QString aSHA1, QString aSHA2 = QString() );

int main( int argc, char *argv[] )
{
   PIPGIT_STATE_T state = PIPGIT_STATE_NONE;

   SetConfig( argc, argv );

   if ( argc < 2 )
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

   if ( arg1 == "insp" )
   {
      ferr.open ( PIPGIT_INSP_LOG );

      if ( ExtractPatches( PIPGIT_STATE_INSPECTION, argc, argv ) == true )
      {
         PrintInfo( PIPGIT_STATE_INSPECTION );
         CleanUp();
      }
      else
      {
         Usage();
      }
   }
   else if ( arg1 == "br" )
   {
      ferr.open ( PIPGIT_BR_LOG );

      if ( ExtractPatches( PIPGIT_STATE_BR, argc, argv ) == true )
      {
         PrintInfo( PIPGIT_STATE_BR );
         CleanUp();
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

   return 0;
}

void SetConfig( int &aArgc, char *aArgv[] )
{
   // Checking .pipgit
   QFile configFile( QDir::homePath() + "/.pipgit" );

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
                  config.colors = false;
               }

               if ( configPair[0].toLower() == "detailed" && configPair[1].toLower() == "no" )
               {
                  config.detailedStat = false;
               }
            }
         }
      }
   }

   // Checking ARGV overides .pipgit
   for ( int index = 0; index < aArgc; index++ )
   {
      QString argString = aArgv[ index ];
      QStringList curArg = argString.split( "=" );

      if ( curArg.count() == 2 )
      {
         --aArgc;

         if ( curArg[0].toLower() == "--detailed" && curArg[1].toLower() == "yes" )
         {
            config.detailedStat = true;
         }
         else
         {
            config.detailedStat = false;
         }
      }
   }
}

bool ExtractPatches( PIPGIT_STATE_T aState, int argc, char *argv[] )
{
   CopyRight( aState );

   if ( !workDir.exists() && workDir.mkdir( workPath ) == false )
   {
      cout << endl << QString( "Error: Couldn't create [%1] directory..." ).arg( workPath ).toStdString().c_str();

      return false;
   }

   QString cmd, arg3 = (argc == 4) ? argv[3] : "";

   if ( arg3.length() > 5 )
   {
      gItemslist = GetDiff( argv[2], argv[3] );

      cmd = QString( "git format-patch -o %3 --add-header=\"-==-\" --numstat --unified=0 --ignore-space-change --no-binary %1...%2 1>%4/%5" ).
                     arg( argv[2] ).arg( arg3 ).arg( workPath ).arg( workPath ).arg( TMP_FILE );

      if ( config.colors == true )
      {
         #ifdef PIPGIT_LINUX
         cout << endl << QString( "Comparing [\033[1;30m%1\033[00m] with [\033[1;30m%2\033[00m] ... " ).arg( QString( argv[ 2 ] ) ).arg( arg3 ).toStdString().c_str();
         #else
         cout << endl << QString( "Comparing [%1] with [%2]" ).arg( QString( argv[ 2 ] ) ).arg( arg3 ).toStdString().c_str();
         #endif
      }
      else
      {
         cout << endl << QString( "Comparing [%1] with [%2]" ).arg( QString( argv[ 2 ] ) ).arg( arg3 ).toStdString().c_str();
      }

      if ( system( cmd.toStdString().c_str() ) != 0 )
      {
         // TODO: Handle error
      }

      if ( config.colors == true )
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
   }
   else
   {
      gItemslist = GetDiff( argv[2] );

      cmd = QString( "git format-patch -o %2 --add-header=\"-==-\" --numstat --unified=0 --ignore-space-change --no-binary %1 1>%3/%4" ).
                      arg( argv[2] ).arg( workPath ).arg( workPath ).arg( TMP_FILE );

      if ( QString( argv[2] ).toInt() == -1 )
      {
         cout << endl << "Comparing last commit ... ";
      }
      else if ( strlen( argv[2] ) > 5 )
      {
#ifdef PIPGIT_LINUX
         cout << endl << QString( "Comparing [\033[1;30m%1\033[00m] with latest commit ... " ).arg( QString( argv[ 2 ] ) ).toStdString().c_str();
#else
         cout << endl << QString( "Comparing [%1] with latest commit ... " ).arg( QString( argv[ 2 ] ) ).toStdString().c_str();
#endif
      }
      else
      {
         return false;
      }

      if ( system( cmd.toStdString().c_str() ) != 0 )
      {
         // TODO: Handle error
      }

      if ( config.colors == true )
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
   }

   #ifdef PIPGIT_DEBUG
   cout << cmd.toStdString().c_str() << endl;
   #endif

   return true;
}

QList< PIPGIT_ITEM_T > GetDiff( QString aSHA1, QString aSHA2 )
{
   QList< PIPGIT_ITEM_T > ItemsList;

   QProcess proc;

   proc.start( QString( "git diff --numstat %1 %2" ).arg( aSHA1 ).arg( aSHA2 ), QIODevice::ReadOnly );
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
         QStringList curItemList = curSting.split( "\t" );

         if ( curItemList.count() == 3 )
         {
            PIPGIT_ITEM_T itemInfo;

            itemInfo.added    = curItemList[0].simplified().toInt();
            itemInfo.deleted  = curItemList[1].simplified().toInt();
            itemInfo.fileName = curItemList[2].simplified();

            ItemsList.append( itemInfo );
         }
      }
   }

   return ItemsList;
}

QString GetCurrentBranch()
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

void PrintInfo( PIPGIT_STATE_T aState )
{
   QStringList list = workDir.entryList();
   QString shId, userName, description;
   QStringList tmpSHASplit, tmpUserSplit, tmpDescriptionSplit;
   QStringList fileNames;
   QString lastSHAId, lastUserName, lastCommitDesc;
   unsigned int commitsNum = 0;

   foreach ( QString filename, list )
   {
      QString userEmail;

      if ( filename == "." || filename == ".." || filename == TMP_FILE )
      {
         continue;
      }

      QString patchToOpen = QString( "%1/%2" ).arg( workPath ).arg( filename );
      is.open( patchToOpen.toStdString().c_str() );

      if ( is.good() )
      {
         shId.clear();
         userName.clear();
         description.clear();

         is.getline( c, 256 );       // get character from file
         shId.append( &c[0] );
         tmpSHASplit = shId.split( " " );
         lastSHAId = tmpSHASplit[ 1 ];

         is.getline( c, 256 );       // get character from file
         userName.append( &c[0] );
         tmpUserSplit = userName.split( " " );
         userEmail = tmpUserSplit[ tmpUserSplit.length() - 1 ];
         userEmail.chop( 1 );
         lastUserName = userEmail = userEmail.mid( 1 );

         is.getline( c, 256 );       // skip line

         is.getline( c, 256 );       // get description
         description.append( &c[0] );
         tmpDescriptionSplit = description.split( "] " );
         lastCommitDesc = tmpDescriptionSplit[ 1 ];
      }

      if ( aState == PIPGIT_STATE_INSPECTION )
      {
         cout << endl << setw(20) << "Commit No:" << "[" << ++commitsNum << "]";
         cerr << endl << setw(20) << "Commit No:" << "[" << commitsNum << "]";

         if ( config.colors == true )
         {
            #ifdef PIPGIT_LINUX
            cout << endl << setw(20) << "GIT Commit SHA ID:" << QString( "[\033[1;32m%1\033[00m]").arg( lastSHAId ).toStdString().c_str()<< endl;
            cout << setw(20) << "Developer email:" << QString( "[\033[1;33m%1\033[00m]" ).arg( lastUserName ).toStdString().c_str() << endl;
            cout << setw(20) << "Commit Description:" << QString( "[\033[1;32m%1\033[00m]" ).arg( lastCommitDesc ).toStdString().c_str() << endl << endl;
            #else
            cout << endl << setw(20) << "GIT Commit SHA ID:" << QString( "[%1]").arg( lastSHAId ).toStdString().c_str()<< endl;
            cout << setw(20) << "Developer email:" << QString( "[%1]" ).arg( lastUserName ).toStdString().c_str() << endl;
            cout << setw(20) << "Commit Description:" << QString( "[%1]" ).arg( lastCommitDesc ).toStdString().c_str() << endl << endl;
            #endif
         }
         else
         {
            cout << endl << setw(20) << "GIT Commit SHA ID:" << QString( "[%1]").arg( lastSHAId ).toStdString().c_str()<< endl;
            cout << setw(20) << "Developer email:" << QString( "[%1]" ).arg( lastUserName ).toStdString().c_str() << endl;
            cout << setw(20) << "Commit Description:" << QString( "[%1]" ).arg( lastCommitDesc ).toStdString().c_str() << endl << endl;
         }

         cerr << endl << setw(20) << "GIT Commit SHA ID:" << QString( "[%1]").arg( lastSHAId ).toStdString().c_str()<< endl;
         cerr << setw(20) << "Developer email:" << QString( "[%1]" ).arg( lastUserName ).toStdString().c_str() << endl;
         cerr << setw(20) << "Commit Description:" << QString( "[%1]" ).arg( lastCommitDesc ).toStdString().c_str() << endl << endl;

         cout << "----------------------------------------------------------------------------------------------" << endl;
         cerr << "----------------------------------------------------------------------------------------------" << endl;

         if ( config.colors == true )
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
         cout << "----------------------------------------------------------------------------------------------" << endl;
         cerr << "----------------------------------------------------------------------------------------------" << endl;
      }

      while ( is.good() )     // loop while extraction from file is possible
      {
         is.getline( c, 256 );       // get character from file

         if ( is.good() )
         {
            if ( strstr( c, "-==-" ) )
            {
               isProcced = true;
               continue;
            }
            // else if( strstr( c, "diff --git" ) && isProcced)
            else if( strstr( c, "--" ) && isProcced)
            {
               isProcced = false;

               break;
            }

            if ( isProcced && strlen( c ) > 0 )
            {
               int added = 0, deleted = 0;
               const char *pStrFileName = NULL;
               QStringList strItems = QString( c ).split( "\t", QString::SkipEmptyParts );

               added = strItems[0].toInt();
               deleted = strItems[1].toInt();
               fileNames.append( strItems[2] );

               if ( added == deleted )
               {
                  totalChanged += added;
               }
               else
               {
                  totalChanged += added + deleted;
               }

               for ( unsigned int index = 0; index < strlen( c ); index++ )
               {
                  if ( isalpha( c[ index ] ) )
                  {
                     pStrFileName = c + index;
                     break;
                  }
               }

               if ( aState == PIPGIT_STATE_INSPECTION )
               {
                  if ( config.colors == true )
                  {
                     #ifdef PIPGIT_LINUX
                     cout << " \033[1;32m" << setw(15) << added << "\033[00m" << "\033[1;31m" << setw(15) << deleted << "\033[00m" << std::setw( 15 ) << pStrFileName << endl;
                     #else
                     cout << " " << setw(15) << added << setw(15) << deleted << std::setw( 15 ) << pStrFileName << endl;
                     #endif
                  }
                  else
                  {
                     cout << " " << setw(15) << added << setw(15) << deleted << std::setw( 15 ) << pStrFileName << endl;
                  }

                  cerr << " " << setw(15) << added << setw(15) << deleted << std::setw( 15 ) << pStrFileName << endl;
               }
            }
         }
      }

      is.close();
   }

   if ( aState == PIPGIT_STATE_INSPECTION && gItemslist.count() > 0 )
   {
      cout << endl << "SUMMARY CHANGES FOR [" << commitsNum << "] COMMIT(s):" << endl;
      cerr << endl << "SUMMARY CHANGES FOR [" << commitsNum << "] COMMIT(s):" << endl;

      cout << "----------------------------------------------------------------------------------------------" << endl;
      cerr << "----------------------------------------------------------------------------------------------" << endl;

      if ( config.colors == true )
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
      cout << "----------------------------------------------------------------------------------------------" << endl;
      cerr << "----------------------------------------------------------------------------------------------" << endl;

      foreach ( PIPGIT_ITEM_T item, gItemslist )
      {
         if ( config.colors == true )
         {
            #ifdef PIPGIT_LINUX
            cout << " \033[1;32m" << setw(15) << item.added << "\033[00m" << "\033[1;31m" << setw(15) << item.deleted << "\033[00m" << std::setw( 15 ) << item.fileName.toStdString().c_str() << endl;
            #else
            cout << " " << setw(15) << item.added << setw(15) << item.deleted << std::setw( 15 ) << item.fileName.toStdString().c_str() << endl;
            #endif
         }
         else
         {
            cout << " " << setw(15) << item.added << setw(15) << item.deleted << std::setw( 15 ) << item.fileName.toStdString().c_str() << endl;
         }

         cerr << " " << setw(15) << item.added << setw(15) << item.deleted << std::setw( 15 ) << item.fileName.toStdString().c_str() << endl;
      }
   }

   if ( aState == PIPGIT_STATE_BR )
   {
      fileNames.removeDuplicates();

      QString branch = GetCurrentBranch();

      if ( config.colors == true )
      {
         #ifdef PIPGIT_LINUX
         cout << "BRANCH: "  << "[\033[1;32m" << branch.toStdString().c_str()    << "\033[00m]" << endl;
         cout << "SHAID:  "  << "[\033[1;32m" << lastSHAId.toStdString().c_str() << "\033[00m]" << endl;
         cout << "USER:   "  << QString( "[\033[1;33m%1\033[00m]" ).arg( lastUserName ).toStdString().c_str() << endl << endl;
         #else
         cout << "BRANCH: "  << "[" << branch.toStdString().c_str()       << "]" << endl;
         cout << "SHAID:  "  << "[" << lastSHAId.toStdString().c_str()    << "]" << endl;
         cout << "USER:   "  << "[" << lastUserName.toStdString().c_str() << "]" << endl << endl;
         #endif
      }
      else
      {
         cout << "BRANCH: "  << "[" << branch.toStdString().c_str()       << "]" << endl;
         cout << "SHAID:  "  << "[" << lastSHAId.toStdString().c_str()    << "]" << endl;
         cout << "USER:   "  << "[" << lastUserName.toStdString().c_str() << "]" << endl << endl;
      }

      cerr << "BRANCH: "  << "[" << branch.toStdString().c_str()       << "]" << endl;
      cerr << "SHAID:  "  << "[" << lastSHAId.toStdString().c_str()    << "]" << endl;
      cerr << "USER:   "  << "[" << lastUserName.toStdString().c_str() << "]" << endl << endl;

      cout << "FILES:" << endl;
      cerr << "FILES:" << endl;

      foreach ( QString fileName, fileNames )
      {
         cout << "   " << fileName.toStdString().c_str() << endl;
         cerr << "   " << fileName.toStdString().c_str() << endl;
      }

      cout << endl << "TESTED: [Yes/No]" << endl << "COMMENT:" << endl;
      cerr << endl << "TESTED: [Yes/No]" << endl << "COMMENT:" << endl;
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

   if ( config.colors == true )
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

   if ( config.colors == true )
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

void CleanUp()
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

void PrintInfoBR()
{
}

void Usage()
{
   cout << "===========================================================================" << endl;
   cout << "Usage:" << " pipgit <insp|br> <SHA ID1 SHA ID2> | -1" << endl;
   cout << "===========================================================================" << endl;
   cout << "Parameter 1. (insp|br) - Switch output information to Inspection or BR" << endl;
   cout << "Parameter 2. (SHA ID)  - Compare changes between SHA1 & SHA2" << endl;
   cout << "Parameter 2. -1        - Show last commit changes" << endl;

   cout << endl << "Examples:" << endl;
   cout << "\'pipgit insp 7deac3c8436afa65a64f5567869f6b9d2a39a33e 7deac3c8436afa6535432442543445\' - Calculates changes between SHA1 & SHA2" << endl;
   cout << "\'pipgit insp 7deac3c8436afa65a64f5567869f6b9d2a39a33e\' - Calculates changes between selected SHA ID & latest commit" << endl;
   cout << "\'pipgit insp -1\' - Show last commit changes" << endl << endl;

   if ( config.colors == true )
   {
      cout << "\033[01;31mNote: Order of parameters SHA1 & SHA2 make sence for \'git diff\', so, be a careful during typing!\033[00m" << endl << endl;
   }
   else
   {
      cout << "Note: Order of parameters SHA1 & SHA2 make sence for \'git diff\', so, be a careful during typing!" << endl << endl;
   }
}

void CopyRight( PIPGIT_STATE_T aState )
{
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << QString( "PIPGIT util - v.%1 (Christmas Edition) Alexander.Golyshkin@teleca.com (c) 2011-2012" ).arg( PIPGIT_VER ).toStdString().c_str() << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;

   if ( config.colors == true )
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
}
