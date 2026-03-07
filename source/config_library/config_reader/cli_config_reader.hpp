#pragma once
#include "ini_config_reader.hpp"

namespace ConfigLib
{
struct INI {};
struct CLI {};

template<typename Tag, typename... Types>
struct HasType : std::false_type {}; 

template<typename Tag, typename Head, typename... Tail>
struct HasType<Tag, Head, Tail...> // true if Tag appears anywhere in Types.
	: std::conditional<
			std::is_same<Tag, Head>::value,
			std::true_type, 
			HasType<Tag, Tail...>
	   >::type
{};

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

// selects the persistence base from the type pack TODO (IHT 20260227): Introduce an XML/JSON in addition to INI)
template<typename Derived, bool hasINI>
struct ResolvePersistenceType;

template<typename Derived>
struct ResolvePersistenceType<Derived, true> {using type = INIConfigReader<Derived>;};

template<typename Derived>
struct ResolvePersistenceType<Derived, false>{using type = NoPersistenceReader<Derived>;};

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

//no CLI found in the pack. no need for the cli wrapper class;
template<typename Derived, typename Base, typename... Types>
struct AddFeatures {using type = Base;};

//cli is the next tpye in the pack, so lets wrap bBase with CLIFeatureLayer
template<typename Derived, typename Base, typename... Rest>
struct AddFeatures<Derived, Base, CLI, Rest...>
{
	using type = CLIFeatureLayer<Derived, Base>;
};

//CLI isnt next, so lets keep going through the pack to see if we can find.
template<typename Derived, typename Base, typename Head, typename... Rest>
struct AddFeatures<Derived, Base, Head, Rest...>
{
	using type = typename AddFeatures<Derived, Base, Rest...>::type;
};

template<typename Derived, typename... ConfigReaderTypes>
class ConfigReader : 
	public AddFeatures<
				Derived,
				typename ResolvePersistenceType<
					Derived,
					HasType<INI, ConfigReaderTypes...>::value
				>::type,
				ConfigReaderTypes...
			>::type
{
	static_assert(HasType<INI, ConfigReaderTypes...>::value 
				|| HasType<CLI, ConfigReaderTypes...>::value,
		"ConfigReader requires at least one type. "
		"Try: ConfigReader<MyConfig, ConfigLib::INI> "
		"or:  ConfigReader<MyConfig, ConfigLib::CLI> "
		"or:  ConfigReader<MyConfig, ConfigLib::INI, ConfigLib::CLI>");
    
    using BaseReaderType = typename AddFeatures<
				Derived,
				typename ResolvePersistenceType<
					Derived,
					HasType<INI, ConfigReaderTypes...>::value
				>::type,
				ConfigReaderTypes...
			>::type;
public:
	ConfigReader() {}
	ConfigReader(int argc, char* argv[]) : BaseReaderType(argc, argv)
	{
		static_assert(HasType<CLI, ConfigReaderTypes...>::value,
			"argc and argv constructor overload requires ConfigLib::CLI in the type pack."
			"Try: ConfigReader<MyConfig, ConfigLib::CLI> "
			"or:  ConfigReader<MyConfig, ConfigLib::INI, ConfigLib::CLI>");
	}
};
} // namespace ConfigLib
