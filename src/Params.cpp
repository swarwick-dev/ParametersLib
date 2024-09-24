/*
 * params.cpp
 *
 *  Created on: Dec 2, 2011
 *      Author: Simon
 */

#include "Params.h"
#include "Logger.h"
#include "common_constants.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include "gzstream.h"
#include <zlib.h>
#include <assert.h>

extern char** environ;

Params::Params(){
	this->vArgs.clear();

	this->pcConfigFilePath = NULL;
}

Params::~Params(){
	this->vArgs.clear();

	if ( this->pcConfigFilePath != NULL )
			free(this->pcConfigFilePath);
}

int Params::setParams(int argc, char *argv[]){
	int iExitCode = SUCCESS;

	iExitCode = setCommandLineParams(argc, argv);

	return iExitCode;
}

int Params::setParams(int argc, char *argv[], string sFileName){
	int iExitCode = SUCCESS;

	iExitCode = setCommandLineParams(argc, argv);
	if ( iExitCode == SUCCESS )
	{
		iExitCode = setConfigFileParams(sFileName);
	}

	return iExitCode;
}

int Params::setCommandLineParams(int argc, char *argv[])
{
	int iExitCode = SUCCESS;
	BOOL bParam = FALSE;
	int i = 0;
	Parameters sLocalParam;
	char *paramPtr = NULL;
	char *pDBName;
	char *pcPtr = NULL, *pcTmp = NULL;
	char caParameter[1024 + 1] = {'\0'};

	for ( i = 0; environ[i] != NULL; i++ )
	{
		sLocalParam.eType = ENV;
		strncpy(caParameter,environ[i], 1024);
		pcPtr = strtok_r(caParameter,"=", &pcTmp);
		sLocalParam.sName = pcPtr;
		pcPtr = strtok_r(NULL,"=", &pcTmp);
		sLocalParam.sValue = pcPtr;
		this->vArgs.push_back(sLocalParam);
		memset(caParameter,0,1024 + 1);
	}

	if ( argc > 1 )
	{
		/* Get the command line params from within the -a "xxx" segment */
		for ( i = 0; i < argc; i++ )
		{
			if ( i > 0 )
			{
				if ( argv[i][0] == '-')
				{
					if ( bParam == TRUE )
					{
						// previous param had no value so add and then move on
						sLocalParam.eType = CMD;
						sLocalParam.sValue = "";
						this->vArgs.push_back(sLocalParam);
						sLocalParam.sName = "";
						sLocalParam.sValue = "";
						bParam = FALSE;
					}

					sLocalParam.sName = argv[i];
					bParam = TRUE;

					if (strcmp(argv[i],"-a") == 0)
					{
						sLocalParam.eType = CMD;
						sLocalParam.sValue = "";
						this->vArgs.push_back(sLocalParam);
						sLocalParam.sName = "";
						sLocalParam.sValue = "";
						bParam = FALSE;

						if ( i < (argc - 1) ) // still params to process
						{
							i++; // advance to the next one

							// args following -a will usually be enclosed by "" so come in one parameters in argv
							// so it requires strtok based on spaces
							paramPtr = strtok(argv[i]," ");

							do {
								if ( paramPtr[0] == '-' )
								{
									if ( bParam == TRUE )
									{
										// previous param had no value so add and then move on
										sLocalParam.eType = CMD;
										sLocalParam.sValue = "";
										this->vArgs.push_back(sLocalParam);
										sLocalParam.sName = "";
										sLocalParam.sValue = "";
										bParam = FALSE;
									}

									sLocalParam.sName = paramPtr;
									bParam = TRUE;
								}
								else if ( paramPtr != NULL )
								{
									sLocalParam.eType = CMD;
									sLocalParam.sValue = paramPtr;
									this->vArgs.push_back(sLocalParam);
									sLocalParam.sName = "";
									sLocalParam.sValue = "";
									bParam = FALSE;

								}

								paramPtr = strtok(NULL," ");
							} while (paramPtr != NULL);
						}
					}
				}
				else
				{
					sLocalParam.eType = CMD;
					sLocalParam.sValue = argv[i];
					this->vArgs.push_back(sLocalParam);
					sLocalParam.sName = "";
					sLocalParam.sValue = "";
					bParam = FALSE;
				}
			}
		}

		if ( bParam == TRUE )
		{
			sLocalParam.eType = CMD;
			this->vArgs.push_back(sLocalParam);
		}
	}

	return iExitCode;
}

string Params::getValueForParameter(string sParam)
{
	vector<Parameters>::iterator it;

	for (it = this->vArgs.begin(); it!=this->vArgs.end(); it++)
	{
		if ( (*it).sName.compare(sParam) == 0 )
			return (*it).sValue;
	}

	return "ERROR"; /* Parameter not found */
}

void Params::logParameters()
{
	vector<Parameters>::iterator it;

	gl_trace(_BASIC,"Command Line Parameters :");

	for (it = this->vArgs.begin(); it!=this->vArgs.end(); it++)
		gl_trace(_BASIC,"Parameter <%s> \t Value <%s>",(*it).sName.c_str(), (*it).sValue.c_str() );

}

int Params::setConfigFileParams(string sFileName)
{
	int iExitCode = SUCCESS;
	char caParam[2048 + 1] = {'\0'};
	Parameters sLocalParam;
	char *pCharPtr = NULL;
	char *pcPath = NULL;
	char caDir[12 + 1] = "/geneva/bin/";
    igzstream ipStream;
    char c = '\0';

	pcPath = getenv("INTERFACES_PATH");
	if ( pcPath == NULL )
	{
		gl_warn("INTERFACES_PATH environment variable not set using local working directory");
		memset(caDir,0,13);
	}

	this->pcConfigFilePath = (char *) malloc(strlen(pcPath) + strlen(caDir) + sFileName.length() + 1);
	if ( this->pcConfigFilePath == NULL )
	{
		gl_error("Unable to allocate memory for configuration file path");
		iExitCode = FAILURE;
	}
	else
	{
		sprintf(this->pcConfigFilePath, "%s%s%s", pcPath, caDir, sFileName.c_str());

	    ipStream.open(this->pcConfigFilePath);
	    if ( !ipStream.good())
	    {
	    	gl_error("Error opening config file %s", this->pcConfigFilePath);
	    	iExitCode = FAILURE;
	    }
	    else
	    {
	    	while ( ipStream.get(c))
	    	{
				caParam[strlen(caParam)] = c;
				caParam[strlen(caParam) + 1] = '\0';

				if ( c == '\n' || c == '\0' )
				{
					/* Ignore comment lines */
					if ( caParam[0] != '#' )
					{
						pCharPtr = strtok(caParam, "=");
						sLocalParam.sName = pCharPtr;
						pCharPtr = strtok(NULL, "=");

						if ( strlen(pCharPtr) > 0 && pCharPtr[strlen(pCharPtr)-1] == '\n')
							pCharPtr[strlen(pCharPtr)-1] = '\0';

						sLocalParam.sValue = pCharPtr;
						sLocalParam.eType = CFGFILE;
						this->vArgs.push_back(sLocalParam);
					}
					memset(caParam,0, 2048 + 1);
				}
	    	}

	    	ipStream.close();
	    }
	}

	return iExitCode;
}

void Params::addParameter(Parameters sParam)
{
	sParam.eType = OTHER;
	this->vArgs.push_back(sParam);
}

int Params::writeConfigFileParams() {
	int iExitCode = SUCCESS;
	ogzstream opStream;
	int i = 0;
	vector<Parameters>::iterator it;

	opStream.open(this->pcConfigFilePath);

	for (it = this->vArgs.begin(); it!=this->vArgs.end(); it++){
		if ( (*it).eType != CFGFILE )
			continue;

		for ( i = 0; i < (*it).sName.size(); i++){
			opStream << (*it).sName.at(i);
		}

		opStream << '=';

		for ( i = 0; i < (*it).sValue.size(); i++)
		{
			opStream << (*it).sValue.at(i);
		}

		opStream << '\n';
	}

	opStream.close();

	return iExitCode;
}

int Params::updateParameter(string sName, long lValue) {
	int iExitCode = SUCCESS;
	vector<Parameters>::iterator it;
	BOOL bFound = FALSE;
	char caTmp[LONG_LEN + 1] = {'\0'};

	for (it = this->vArgs.begin(); it!=this->vArgs.end(); it++){
		if ( (*it).sName.compare(sName) == 0 ){
			// found parameter to modify
			bFound = TRUE;
			sprintf(caTmp,"%ld", lValue);
			(*it).sValue = caTmp;
		}
	}

	return iExitCode;
}
