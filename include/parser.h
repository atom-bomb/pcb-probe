/* 
 * File:   parser.h
 * Author: ideras
 *
 * Created on January 18, 2013, 6:52 PM
 */

#ifndef PARSER_H
#define	PARSER_H

#include <string>
#include <map>

using namespace std;

typedef long double Real;

struct GCodeCommand {

    GCodeCommand(string name, Real x, Real y) {
        this->name = name;
        argNameList = "XY";
        arguments['X'] = x;
        arguments['Y'] = y;
        zformula = "";
    }
    
    GCodeCommand()
    {
        name = "";
        zformula = "";
        argNameList = "";
    }

    void setXCoord(Real x) {
        arguments['X'] = x;
    }

    void setYCoord(Real y) {
        arguments['Y'] = y;
    }

    void setZCoord(Real z) {
        arguments['Z'] = z;
    }
    
    void setZFormula(string &zformula) {
        if (!hasZCoord())
            argNameList += "Z";
        
        this->zformula = zformula;
    }

    void setFeedRate(Real f) {
        arguments['F'] = f;
    }

    Real getXCoord() {
        return arguments['X'];
    }

    Real getYCoord() {
        return arguments['Y'];
    }

    Real getZCoord() {
        return arguments['Z'];
    }

    Real getFeedRate() {
        return arguments['F'];
    }

    bool hasXCoord() {
        return arguments.find('X') != arguments.end();
    }

    bool hasYCoord() {
        return arguments.find('Y') != arguments.end();
    }
    
    bool hasZCoord() {
        return arguments.find('Z') != arguments.end();
    }

    bool hasFeedRate() {
        return arguments.find('F') != arguments.end();
    }
    
    void addArgument(char argName, Real argValue)
    {
        argNameList += argName;
        arguments[argName] = argValue;
    }
    
    void Clear()
    {
        arguments.clear();
        name = "";
        zformula = "";
        argNameList = "";
    }
    
    void operator=(GCodeCommand &cmd)
    {
        Clear();
        this->name = cmd.name;
        argNameList = cmd.argNameList;
        zformula = cmd.zformula;
        
        for (unsigned int i = 0; i < argNameList.length(); i++ ) {
                char argName = argNameList[i];
                Real value = arguments[argName];
        
                arguments[argName] = value;
        }
    }
    
    string ToString();

    string name;
    string zformula;     //Interpolation Formula for Z Coordinate
    string argNameList;  //Argument Names
    map<char, Real> arguments;
};

void ParseGCodeLine(string &line, GCodeCommand &command);

#endif	/* PARSER_H */

