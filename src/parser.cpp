#include <stdlib.h>
#include <iostream>
#include <sstream>

#include "parser.h"

extern int currentLine;

double ParseNumber(string &line, unsigned int &pos)
{
    double result = 0.0;
    string str = "";

    while (pos < line.length() && !isalpha(line[pos]) && !isspace(line[pos]))
        str += line[pos++];

    result = atof(str.c_str());

    return result;
}

void SkipSpaces(string &line, unsigned int &pos)
{
    while (pos < line.length() && isspace(line[pos]))
        pos++;
}

string NextToken(string &line, unsigned int &pos)
{
    static string last_result = "";
    string result = "";

    while (1) {
        SkipSpaces(line, pos);

        if (pos == line.length())
            return result;

        switch (line[pos]) {
            case '(': //Comment
                while (pos < line.length() && line[pos] != ')')
                    result += line[pos++];

                result += line[pos++];

                return result ;
            case ';': // Comment
                return result;
            case 'X': //generate lines with no commands, we assume last move command
            case 'Y':
                pos = 0;
                return last_result;
            case 'S':
            case 'M':
            case 'G':
            case 'F':
                while (pos < line.length() && !isspace(line[pos]))
                    result += line[pos++];
                last_result = result;
                return result;
            default:
                cerr << currentLine << ": Unknown symbol '" << line[pos] << "'" << endl;
                exit(2);
        }
    }

    return result;
}

void ParseGCodeLine(string& line, GCodeCommand& command)
{
    unsigned int i = 0;

    //Get command name
    string cmd_name = NextToken(line, i);

    command.Clear();
    command.name = cmd_name;

    if (i >= line.length()) //No command, just a comment
        return;

    //Get command arguments
    char argName;
    Real argValue;

    while (1) {

        SkipSpaces(line, i);

        if (i >= line.length())
            break;

        if (isalpha(line[i])) {
            argName = line[i++];
            argValue = ParseNumber(line, i);
            command.addArgument(argName, argValue);
        } else {
            if (line[i] == '(') { //Is a comment
                while (i < line.length() && line[i] != ')')
                    i++;

                i++;
            } else if (line[i] == ';') { //Is another kind of comment
               break;
            } else {
                cerr << currentLine << ": Unknown argument '" << line[i] << "'" << endl;
                exit(3);
            }
        }

    }
}

string GCodeCommand::ToString()
{
    stringstream ss;

    ss.precision(4);

    ss << name;

    for (unsigned int i = 0; i < argNameList.length(); i++) {
        char argName = argNameList[i];
        Real value = arguments[argName];

        if (argName != 'Z' || zformula.empty()) {
            ss << " " << argName << fixed << value;
        } else {
            ss << " " << argName << "[" << zformula << "]";
        }
    }

    return ss.str();
}
