/*
 * params.h
 *
 *  Created on: Dec 2, 2011
 *      Author: Simon
 */

#ifndef PARAMS_H_
#define PARAMS_H_

#include <cstdlib>
#include <string>
#include <vector>

using namespace std;

typedef int BOOL;

typedef enum _ParamType {
	ENV,
	CFGFILE,
	CMD,
	OTHER
} ParamType;

typedef struct _Parameters {
	ParamType eType;
	string sName;
	string sValue;
} Parameters;

class Params {
private:
	char *pcConfigFilePath;

public:
	vector<Parameters> vArgs;

	Params();
	~Params();
	int setParams(int argc, char *argv[]);
	int setParams(int argc, char *argv[], string sFileName);
	int setCommandLineParams(int argc, char *argv[]);
	int setConfigFileParams(string sFileName);
	void logParameters();
	string getValueForParameter(string sParam);
	void addParameter(Parameters sParam);
	int writeConfigFileParams();
	int updateParameter(string sName, long lValue);
};


#endif /* PARAMS_H_ */
