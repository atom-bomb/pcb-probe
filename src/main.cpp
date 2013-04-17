/* 
 * File:   main.cpp
 * Author: ideras
 *
 * Created on January 18, 2013, 5:46 PM
 */

#include <cstdlib>
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <unistd.h>
#include "parser.h"
#include "pcb-probe.h"

#ifndef VERSION
#define VERSION "Unknown"
#endif

using namespace std;

int main(int argc, char** argv) {
    char bShowHelp = 0 ;
    char opt ;
    extern char *optarg ;
    extern int optind, optopt, opterr ;

    while ((opt = getopt(argc, argv, "emhvc:t:r:")) != -1) {
      switch (opt) {
        case 'e':
          SetGCodeVariant(emc) ;
          break ;
        case 'm':
          SetGCodeVariant(mach3) ;
          break ;
        case 'h':
          bShowHelp = 1 ;
          break ;
        case 'v':
          cerr << argv[0] << " Version: " << VERSION << endl ;
          exit(0) ;
          break ;
        case 'c': // clear height
          SetClearHeight(atof(optarg)) ;
          break ;
        case 't': // traverse height
          SetTraverseHeight(atof(optarg)) ;
          break ;
        case 'r': // route depth
          SetRouteDepth(atof(optarg)) ;
          break ;
        case '?':
          bShowHelp = 1 ;
          break ;
      } /* switch */
    } /* while */

    if (bShowHelp || (argc - optind != 2)) {
        cerr << "Usage: " << argv[0] << " [option] " << "infile outfile" << endl;
        cerr << "Where option may be one of:" << endl ;
        cerr << "  -e  force EMC2 GCode output" << endl ;
        cerr << "  -m  force Mach3 GCode output" << endl ;
        cerr << "  -c (height)  force clear height" << endl ;
        cerr << "  -t (height)  force traverse height" << endl ;
        cerr << "  -r (height)  force route depth" << endl ;
        cerr << "  -v  display program version number" << endl ;
        cerr << "  -h  show this help message" << endl ;
        exit(1);
    }

    char *infile_path = argv[optind];
    char *outfile_path = argv[optind + 1];

    cerr << "Processing input file ... " << infile_path << endl;
    LoadAndSplitSegments(infile_path);
    
    string unit = (info.UnitType == UNIT_INCHES)? "Inches" : "mm";
    cerr << "Board Size (" << unit << "): " << fabs(info.MillMaxX - info.MillMinX) << "x" << fabs(info.MillMinY - info.MillMaxY) << endl << endl;
    
    cerr << "Generating " << GCode_Variants[info.GCode_Type].name << " GCode output in " << outfile_path << endl ;
    DoInterpolation();
    GenerateGCodeWithProbing(outfile_path);
    cerr << "Done." << endl;
    
    return 0;
}

