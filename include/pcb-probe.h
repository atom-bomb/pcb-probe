/* 
 * File:   pcb-gcode.h
 * Author: ideras
 *
 * Created on January 18, 2013, 8:40 PM
 */

#ifndef PCB_GCODE_H
#define	PCB_GCODE_H

#include "parser.h"
#include "gcode_variants.h"

#define UNIT_INCHES     0
#define UNIT_MM         1

struct Position {
    Real x;
    Real y;
    Real z;
};

struct PCBProbeInfo
{
    int UnitType; //MM by default
    Position Pos;
    double GridSize;
    double SplitOver;
    
    //Board boundaries
    Real MillMinX;
    Real MillMinY;
    Real MillMaxX;
    Real MillMaxY;
    
    //Number of cells in Grid
    int GridMaxX;
    int GridMaxY;
    
    void ResetPos()
    {
        Pos.x = 0;
        Pos.y = 0;
        Pos.z = 0;
    }
   
    GCode_Variant_Name GCode_Type ; 

    Real clear_height ;
    bool clear_height_set ;

    Real traverse_height ;
    bool traverse_height_set ;

    Real route_depth ;
    bool route_depth_set ;

    Real probe_depth ;
    bool probe_depth_set ;

    Real initial_probe ;
    bool initial_probe_set ;

    Real traverse_speed ;
    bool traverse_speed_set ;

    Real probe_speed ;
    bool probe_speed_set ;
};

extern PCBProbeInfo info;

void SetGCodeVariant(GCode_Variant_Name theVariant);

void SetClearHeight(Real theHeight) ;
void SetTraverseHeight(Real theHeight) ;
void SetRouteDepth(Real theDepth) ;

void LoadAndSplitSegments(const char *infile_path);
void DoInterpolation();
void GenerateGCodeWithProbing(const char *outfile_path);


#endif	/* PCB_GCODE_H */

