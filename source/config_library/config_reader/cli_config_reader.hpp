#pragma once
#include "ini_config_reader.hpp"

namespace ConfigLib
{
template<typename Derived>
class NoPersistenceReader: public ConfigReaderBase
{
public:
	NoPersistenceReader()
	{
		Derived& d = static_cast<Derived&>(*this);
		initializeForCLI(d.getConfigSections());
	}
	
	NoPersistenceReader(int argc, char* argv[])
	{
		Derived& d = static_cast<Derived&>(*this);
		for(int i = 0; i < argc; ++i) rawCLIArgs.emplace_back(argv[i]);
		initializeForCLI(d.getConfigSections());
	}
};

//cli wrapper class. 
template<typename Derived, typename Base>
class CLIFeatureLayer : public Base
{
public:
	CLIFeatureLayer() : Base()
	{
		std::cout << "CLIFeatureLayer default constructor called" << std::endl;
	}
	
	CLIFeatureLayer(int argc, char* argv[]) : Base(argc, argv)
	{
		std::cout << "CLIFeatureLayer argc & argv constructor called" << std::endl;
	}
	
};

} // namespace ConfigLib
