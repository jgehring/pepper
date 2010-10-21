/*
 * pepper - SCM statistics report generator
 * Copyright (C) 2010 Jonas Gehring
 *
 * file: options.h
 * Command-line option parsing (interface)
 */


#ifndef OPTIONS_H_
#define OPTIONS_H_


#include <map>
#include <string>
#include <vector>


class Options
{
	public:
		struct AuthData {
			std::string username;
			std::string password;
		};

	public:
		Options();

		void parse(int argc, char **argv);

		bool valid() const;
		std::string error() const;

		bool helpRequested() const;
		bool versionRequested() const;

		bool useCache() const;
		std::string cacheDir() const;

		std::string forcedBackend() const;
		std::string repoUrl() const;
		AuthData authData() const;

	private:
		void reset();
		void parse(const std::vector<std::string> &args);
		void putopt(const std::string &key, const std::string &value);

	private:
		bool m_valid;
		std::string m_error;
		mutable std::map<std::string, std::string> m_options;
};


#endif // OPTIONS_H_
