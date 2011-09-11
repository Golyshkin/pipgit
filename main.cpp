#include <QtCore/QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

const char * TMP_FILE = "null";
const char * PIPGIT_FOLDER = "/pipgit";

void Usage();
void CopyRight();

int main( int argc, char *argv[] )
{
   char c[256] = { 0 };
   ifstream is;
   bool isProcced = false;
   unsigned int totalChanged = 0;
   QString workPath = QDir::tempPath() + PIPGIT_FOLDER;

   QDir workDir( workPath );

   if ( argc < 2 )
   {
      Usage();
      CopyRight();

      return 0;
   }

   CopyRight();

   if ( !workDir.exists() && workDir.mkdir( workPath ) == false )
   {
      cout << endl << QString( "Error: Couldn't create [%1] directory..." ).arg( workPath ).toStdString().c_str();
   }

   QString arg1 = argv[1];
   QString arg2 = (argc == 3) ? argv[2] : "";

   QString cmd = QString( "git format-patch -o %3 --add-header=\"-==-\" --numstat --unified=0 --ignore-space-change --no-binary %1 %2 1>%4/%5" ).
                          arg( arg1 ).arg( arg2 ).arg( workPath ).arg( workPath ).arg( TMP_FILE );

   cout << endl << QString( "Extracting GIT data for [%1] .......... " ).arg( arg1 ).toStdString().c_str();
   system( cmd.toStdString().c_str() );
   cout << "[ DONE ]" << endl;

   QStringList list = workDir.entryList();
   QString shId, userName, description;
   QStringList tmpSHASplit, tmpUserSplit, tmpDescriptionSplit;

   foreach ( QString filename, list )
   {
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

         is.getline( c, 256 );       // get character from file
         userName.append( &c[0] );
         tmpUserSplit = userName.split( " " );

         is.getline( c, 256 );       // skip line

         is.getline( c, 256 );       // get description
         description.append( &c[0] );
         tmpDescriptionSplit = description.split( "] " );
      }

      cout << endl << QString( "GIT Commit SHA ID:\t[%1]" ).arg( tmpSHASplit[ 1 ] ).toStdString().c_str()<< endl;
      cout << QString( "Developer email:\t[%1]" ).arg( tmpUserSplit[ tmpUserSplit.length() - 1 ] ).toStdString().c_str() << endl;
      cout << QString( "Commit Description:\t[%1]" ).arg( tmpDescriptionSplit[ 1 ] ).toStdString().c_str() << endl << endl;

      cout << "----------------------------------------------------------------------------------------------" << endl;
      cout << "Added:    |     Deleted: | Changed File:" << endl;
      cout << "----------------------------------------------------------------------------------------------" << endl;

      while ( is.good() )     // loop while extraction from file is possible
      {
         is.getline( c, 256 );       // get character from file

         if ( is.good() )
         {
            stringstream  strm( c );

            if ( strstr( c, "-==-" ) )
            {
               isProcced = true;
               continue;
            }
            else if( strstr( c, "diff --git" ) && isProcced)
            {
               isProcced = false;

               break;
            }

            if ( isProcced && strlen( c ) > 0 )
            {
               int added = 0, deleted = 0;
               const char *pStrFileName = NULL;
               stringstream stream( c );

               stream >> added;
               strm.get();

               stream >> deleted;

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

               cout << " " << added  << std::setw( 8 ) << "\t " << deleted << std::setw( 6 ) << "\t    " << pStrFileName << endl;
            }
         }
      }

      is.close();
   }

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
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << "Total Inspection Changes:\t" << totalChanged << " LOC" << endl;
   cout << "Total Inspection Time:\t\t" <<  strInspTime.toStdString() << endl;
   cout << "Total Inspections:\t\t" << inspectionsCount << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;

   return 0;
}

void Usage()
{
   cout << "========================================" << endl;
   cout << "Usage:" << " pipgit <SHA ID, -1...-n> [ Commits ]" << endl;
   cout << "========================================" << endl;
   cout << "Parameter 1. (SHA ID)  - Get GIT SHA ID for base code changes calculate" << endl;
   cout << "              -1..-n   - Get last <n> commits for code changes calculate" << endl;
   cout << "Parameter 2. (Commits) - How many commits should be analyzed by pipgit based on SHA ID. This value must begins from -1 to -5 with step 1" << endl;

   cout << endl << "Example:" << endl;
   cout << "\'pipgit 7deac3c8436afa65a64f5567869f6b9d2a39a33e\''   - Will calculate changes from selected SHA ID to HEAD" << endl;;
   cout << "\'pipgit 7deac3c8436afa65a64f5567869f6b9d2a39a33e -1\' - Will calculate changes from selected SHA ID only" << endl;
   cout << "\'pipgit -1\' - Will calculate changes from last commit" << endl;
   cout << "\'pipgit -3\' - Will calculate changes from 3 last commits" << endl;
}

void CopyRight()
{
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << "PIPGIT util - v.0.5 Alexander.Golyshkin@teleca.com (c) 2011" << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;
   cout << "1. CR Tool Web-Page:\t\thttps://bugtracking.teleca.com/Instances/IviCR" << endl;
   cout << "2. Inspection Tool Web-Page:\thttps://bugtracking.teleca.com/Instances/IviInsp" << endl;
   cout << "3. BR Tool Web-Page:\t\thttps://bugtracking.teleca.com/Instances/IviBR" << endl;
   cout << "----------------------------------------------------------------------------------------------" << endl;

}
