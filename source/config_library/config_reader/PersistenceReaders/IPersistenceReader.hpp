#pragma once

namespace ConfigLib
{
	
class IPersistenceReader
{
public:
	virtual ~IPersistenceReader() = default;
	
	virtual void persistSave(const std::vector<ConfigSection>& configSections) const = 0;	
	virtual void persistDelete() = 0;
	virtual void persistExport(
		const std::string& exportPath,
		const std::vector<ConfigSection>& configSections) const = 0;

};
	
}
