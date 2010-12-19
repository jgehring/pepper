/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: options.h
 * Command-line option parsing (interface)
 */


#ifndef OPTIONS_H_
#define OPTIONS_H_


#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "main.h"


class Options
{
	public:
		Options();

		void parse(int argc, char **argv);

		bool helpRequested() const;
		bool backendHelpRequested() const;
		bool scriptHelpRequested() const;
		bool versionRequested() const;
		bool backendListRequested() const;
		bool scriptListRequested() const;

		bool useCache() const;
		bool checkCache() const;
		std::string cacheDir() const;

		std::string forcedBackend() const;
		std::string repoUrl() const;
		std::map<std::string, std::string> backendOptions() const;

		std::string script() const;
		std::map<std::string, std::string> scriptOptions() const;

		static void print(const std::string &option, const std::string &text, std::ostream &out = std::cout);

	private:
		void reset();
		void parse(const std::vector<std::string> &args);
		bool parseOpt(const std::string &arg, std::string *key, std::string *value);
		std::string makeAbsolute(const std::string &path);

	PEPPER_PVARS
		mutable std::map<std::string, std::string> m_options;
		mutable std::map<std::string, std::string> m_backendOptions, m_scriptOptions;
};


#endif // OPTIONS_H_
