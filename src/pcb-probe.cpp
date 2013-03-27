/*
 * XXX TODO: scrape GCode input for route_depth, clear_height, traverse_height and traverse_speed
 * XXX TODO: try to guess GCode variant by looking at GCode input
 * XXX TODO: handle any potential output stream errors and return them to the caller
 */
#include <assert.h>
#include <cmath>
#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <list>
#include "pcb-probe.h"

using namespace std;

#ifndef DEFAULT_GCODE_TYPE
#define DEFAULT_GCODE_TYPE emc
#endif

#define FIRST_GCODE_VARIABLE theGCodeVariant->first_variable
#define LAST_GCODE_VARIABLE theGCodeVariant->last_variable
#define Z_PROBE_RESULT_VARIABLE theGCodeVariant->z_probe_result_variable
#define GCODE_PAUSE_COMMAND theGCodeVariant->pause_command
#define GCODE_PROBE_COMMAND theGCodeVariant->probe_command
#define GCODE_STARTSUB_COMMAND theGCodeVariant->start_sub_command
#define GCODE_ENDSUB_COMMAND theGCodeVariant->end_sub_command
#define GCODE_CALLSUB_COMMAND theGCodeVariant->call_sub_command

PCBProbeInfo info;
const GCode_Variant *theGCodeVariant = &GCode_Variants[DEFAULT_GCODE_TYPE] ;
list<GCodeCommand> cmdList;
map<string, int> cellVariables; //GCode variables associated with every cell in the Grid
int nextVariableNumber = FIRST_GCODE_VARIABLE ;
int currentLine = 0;

/*
 * This routine will split up long distances into chunks
 * if recursively halves the values (so we don't have to
 * worry about being proportional)
 */
static void distance_split(Real from_x, Real from_y, GCodeCommand &command)
{
    Real to_x = command.getXCoord();
    Real to_y = command.getYCoord();

    Real dist_x = to_x - from_x;
    Real dist_y = to_y - from_y;

    if (fabs(dist_x) > info.SplitOver || fabs(dist_y) > info.SplitOver) {
        Real mp_x = from_x + (dist_x / 2);
        Real mp_y = from_y + (dist_y / 2);

        GCodeCommand cmd1(command.name, mp_x, mp_y);
        GCodeCommand cmd2(command.name, to_x, to_y);

        if (command.hasFeedRate())
            cmd1.setFeedRate(command.getFeedRate());


        distance_split(from_x, from_y, cmd1);
        distance_split(mp_x, mp_y, cmd2);
    } else {
        cmdList.push_back(command);
    }
}

static void split_if_needed(GCodeCommand &command)
{
    if (info.Pos.z >= 0 || !command.hasXCoord() || !command.hasYCoord()) {
        cmdList.push_back(command);
    } else {

        /*
         * We now have a start and end position, we can call our recursive routine...
         */
        distance_split(info.Pos.x, info.Pos.y, command);
    }
}

void moveTo(GCodeCommand &command)
{
    if (command.hasXCoord())
        info.Pos.x = command.getXCoord();

    if (command.hasYCoord())
        info.Pos.y = command.getYCoord();

    if (command.hasZCoord())
        info.Pos.z = command.getZCoord();
}

void SetGCodeVariant(GCode_Variant_Name theVariant)
{
  theGCodeVariant = &GCode_Variants[theVariant] ;
  info.GCode_Type = theVariant ;
  nextVariableNumber = FIRST_GCODE_VARIABLE ;
}

void LoadAndSplitSegments(const char *infile_path)
{
    string line;
    ifstream in(infile_path);

    if (!in.is_open()) {
        cerr << "Unable to open file: " << infile_path << endl;
        return;
    }

    GCodeCommand cmd;
    bool definedMillMinX = false;
    bool definedMillMaxX = false;
    bool definedMillMinY = false;
    bool definedMillMaxY = false;

    info.ResetPos();
    info.GridSize = 5;
    info.SplitOver = 5;

    while (in.good()) {

        currentLine++;
        getline(in, line);
        ParseGCodeLine(line, cmd);
        
        if (cmd.name == "G20") {
            //Units in inches
            info.GridSize = info.GridSize / 25.4;
            info.SplitOver = info.SplitOver / 25.4;
            info.UnitType = UNIT_INCHES;
        } else if (cmd.name == "G21") {
            info.UnitType = UNIT_MM;
        }

        if (cmd.name == "G00" || cmd.name == "G01") {
            /*
             * We have a move command, if our z is below zero then this will
             * count towards our area
             */
            split_if_needed(cmd);
            moveTo(cmd);

            if (info.Pos.z < 0) {
                if (!definedMillMinX || (info.Pos.x < info.MillMinX)) {
                    definedMillMinX = true;
                    info.MillMinX = info.Pos.x;
                }
                if (!definedMillMaxX || (info.Pos.x > info.MillMaxX)) {
                    definedMillMaxX = true;
                    info.MillMaxX = info.Pos.x;
                }
                if (!definedMillMinY || (info.Pos.y < info.MillMinY)) {
                    definedMillMinY = true;
                    info.MillMinY = info.Pos.y;
                }
                if (!definedMillMaxY || (info.Pos.y > info.MillMaxY)) {
                    definedMillMaxY = true;
                    info.MillMaxY = info.Pos.y;
                }
            }

        } else if (!cmd.name.empty()) {
            cmdList.push_back(cmd);
        }
    }
    in.close();

    info.GridMaxX = floor((info.MillMaxX - info.MillMinX) / info.GridSize);
    info.GridMaxY = floor((info.MillMaxY - info.MillMinY) / info.GridSize);
}

//Second Pass

string getKey(int gx, int gy)
{
    stringstream ss;

    ss << gx << "," << gy;
    return ss.str();
}

/*
 * This makes sure we have a variable assigned to a given cell
 */
void ensure_cell_variable(int gx, int gy)
{
    string key = getKey(gx, gy);

    if (cellVariables.find(key) == cellVariables.end()) {
        cellVariables[key] = nextVariableNumber++;
        assert(nextVariableNumber < LAST_GCODE_VARIABLE) ;
    }
}

/*
 * This functions returns true if a cell has a variable associated, false otherwise
 */
bool cellHasVariable(int gx, int gy)
{
    string key = getKey(gx, gy);

    return (cellVariables.find(key) != cellVariables.end());
}

int cell_variable(int gx, int gy)
{
    string key = getKey(gx, gy);
    return cellVariables[key];
}

/*
 * Given a co-ordinate we can work out a grid x and y number
 * so we can lookup stuff 
 */

void grid_ref(Real x, Real y, int &ref_x, int &ref_y)
{

    Real zero_x = x - info.MillMinX;
    Real zero_y = y - info.MillMinY;

    ref_x = floor(zero_x / info.GridSize);
    ref_y = floor(zero_y / info.GridSize);
}

/*
 * Given a co-ordinate we need to interpolate the values from
 * the surrounding cells
 */
string interpolate(Real x, Real y)
{

    int cellx, celly;
    grid_ref(x, y, cellx, celly);

    Real os_x = ((x - info.MillMinX) - ((Real) cellx * info.GridSize)) / info.GridSize;
    Real os_y = ((y - info.MillMinY) - ((Real) celly * info.GridSize)) / info.GridSize;

    int px_cell = cellx + (os_x > 0.5 ? 1 : -1);
    int py_cell = celly + (os_y > 0.5 ? 1 : -1);

    if (px_cell < 0 || px_cell > info.GridMaxX) {
        px_cell = cellx;
    }
    if (py_cell < 0 || py_cell > info.GridMaxY) {
        py_cell = celly;
    }

    Real x_pc = 0.5 + (os_x > 0.5 ? 1 - os_x : os_x);
    Real y_pc = 0.5 + (os_y > 0.5 ? 1 - os_y : os_y);

    /*
     * Now we can make sure that each of our cells has a variable in it...
     */
    ensure_cell_variable(cellx, celly);
    ensure_cell_variable(px_cell, celly);
    ensure_cell_variable(cellx, py_cell);
    ensure_cell_variable(px_cell, py_cell);

    /*
     * Now we can work out the interpolation...
     */
    stringstream ss;

    ss.precision(3);
    ss << fixed << (x_pc * y_pc) << "*#" << cell_variable(cellx, celly) << " + " <<
            ((1 - x_pc) * y_pc) << "*#" << cell_variable(px_cell, celly) << " + " <<
            (x_pc * (1 - y_pc)) << "*#" << cell_variable(cellx, py_cell) << " + " <<
            ((1 - x_pc) * (1 - y_pc)) << "*#" << cell_variable(px_cell, py_cell) << " + " <<
            "#3";

    return ss.str();
}

/*
 * Last phase ... add the depth sensing bit to the file
 */
void DoInterpolation()
{
    list<GCodeCommand>::iterator it = cmdList.begin();

    info.ResetPos();
    while (it != cmdList.end()) {
        GCodeCommand cmd = *it;

        if (cmd.name == "G00" || cmd.name == "G01") {
            /*
             * We have a move command, if our z is below zero then this will
             * count towards our area
             */
            moveTo(cmd);

            if (info.Pos.z < 0) {
                /*
                 * First thing we do is allocate a variable number to the grid
                 * square (if it hasn't already got one)
                 */
                string zformula = interpolate(info.Pos.x, info.Pos.y);
                it->setZFormula(zformula);
            }

        }

        it++;
    }

}

void GenerateGCodeWithProbing(const char *outfile_path)
{
    ofstream out(outfile_path);

    if (!out.is_open()) {
        cerr << "Unable to open file: " << outfile_path << endl;
        return;
    }

    list<GCodeCommand>::iterator it = cmdList.begin();

    while (it != cmdList.end()) {
        GCodeCommand cmd = *it;

        /*
         * We'll put our stuff right after the G21
         */
        if (cmd.name == "G21" || cmd.name == "G20") {
            Real clear_height;
            Real traverse_height;
            Real route_depth;
            Real probe_depth;
            Real initial_probe;
                
            if (info.UnitType == UNIT_INCHES) {
                clear_height = 0.47244;
                traverse_height = 0.01969;
                route_depth = -0.001969;
                probe_depth = -0.03937;
                initial_probe = -0.1969;
            } else {
                clear_height = 12.0;
                traverse_height = 0.5;
                route_depth = -0.05;
                probe_depth = -1;
                initial_probe = -5;
            }
            
            out << cmd.ToString() << endl;
            out << "\n"
                    "(Processed with pcb-probe version " << VERSION << 
                    " by Lee Essen, 2011, Ivan de Jesus Deras 2013)"
                    "\n"
                    "(this GCode is intended for " << theGCodeVariant->name << ")\n"
                    "\n"
                    "#1=" << clear_height << "			(clearance height)\n"
                    "#2=" << traverse_height << "       	(traverse height)\n"
                    "#3=" << route_depth << "   		(route depth)\n"
                    "#4=" << probe_depth << "			(probe depth)\n"
                    "#5=400			(traverse speed)\n"
                    "#6=60			(probe speed)\n"
                    "\n"
                    "\n"
                    "M05			(stop motor)\n"
                    "(MSG,PROBE: Position to within 5mm of surface & resume)\n"
                    << GCODE_PAUSE_COMMAND << " (position to within 5m of surface and resume)\n"
                    "G49			(clear any tool offsets)\n"
                    "G92.1			(zero co-ordinate offsets)\n"
                    "G91			(use relative coordinates)\n"
                    << GCODE_PROBE_COMMAND << " Z" << initial_probe << " F[#6]	(probe to find worksurface)\n"
                    "G90			(back to absolute)\n"
                    "G92 Z0			(zero Z)\n"
                    "G00 Z[#1]		(safe height)\n"
                    "(MSG,PROBE: Z-Axis calibrate complete, beginning probe)\n"
                    "\n" ;

            /*
             * Now we can create the code for the depth sensing bit... but
             * we should do it in a fairly optimal way
             */

            int gx, gy, rgx;

            for (gy = 0; gy <= info.GridMaxY; gy++) {
                for (rgx = 0; rgx <= info.GridMaxX; rgx++) {
                    if (gy & 1) {
                        gx = info.GridMaxX - rgx;
                    } else {
                        gx = rgx;
                    }

                    // Find the point in the centre of the grid square...
                    Real px = info.MillMinX + ((Real) gx * info.GridSize) + (info.GridSize / 2);
                    Real py = info.MillMinY + ((Real) gy * info.GridSize) + (info.GridSize / 2);

                    if (!cellHasVariable(gx, gy))
                        continue;

                    int var = cell_variable(gx, gy);

                    out << "(PROBE[" << gx << "," << gy << "] " << px << " " << py << " -> " << var << ")" << endl;
                    out << "#100 = " << px << endl ;
                    out << "#101 = " << py << endl ;
                    out << GCODE_CALLSUB_COMMAND << endl ;
                    out << "#" << var << " = #" << Z_PROBE_RESULT_VARIABLE << endl;
                }
            }

            /*
             * Now before we go into the main mill bit we need to give you a chance
             * to undo the probe connections
             */
            out << "\n"
                    "\n"
                    "G00 Z[#1]		(safe height)\n"
                    "(MSG,PROBE: Probe complete, remove connections & resume)\n"
                    << GCODE_PAUSE_COMMAND << " (Probe complete, remove connections and resume)\n"
                    "(MSG,PROBE: Beginning etch)\n"
                    "\n"
                    "\n";
        } else {
            out << cmd.ToString() << endl;
        }

        it++;
    }
    /* put the probe subroutine at the end of output since it's not obvious how to make mach3 skip
     * over gcode at the beginning of the program text.
     */
    out <<          "\n"
                    "(probe routine)\n"
                    "(params: 100 = x 101 = y 2 = traverse_height 4 = probe_depth 5 = traverse_speed 6 = probe_speed)\n"
                    << GCODE_STARTSUB_COMMAND << "\n"
                    "G00 X[#100] Y[#101] Z[#2] F[#5]\n"
                    << GCODE_PROBE_COMMAND << " Z[#4] F[#6]\n"
                    "G00 Z[#2]\n"
                    << GCODE_ENDSUB_COMMAND << 
                    "\n";
}
