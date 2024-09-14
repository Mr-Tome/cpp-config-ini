#ifndef VALIDATION_RULES_H
#define VALIDATION_RULES_H

#include <functional>
#include <string>
#include <vector>

namespace ConfigLib {
    class ConfigValue;
}
namespace ValidationRules {

	class Rule {
    public:
        virtual ~Rule() {}
        virtual bool operator()(const ConfigLib::ConfigValue& value) const = 0;
        virtual std::string toString() const = 0;
    };
	
	class GreaterThanZero : public Rule {
    public:
        bool operator()(const ConfigLib::ConfigValue& value) const override;
        std::string toString() const override { return "Must be greater than zero"; }
    };
	
	class GreaterThanOrEqualToZero : public Rule {
	public:
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		std::string toString() const override { return "Must be greater than or equal to zero"; }
	};
	
	class BetweenValues : public Rule {
	public:
		BetweenValues(double min, double max) : min_(min), max_(max) {}
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		std::string toString() const override;
	private:
		double min_;
		double max_;
	};
	
	class InList : public Rule {
	public:
		InList(const std::vector<std::string>& validValues) : validValues_(validValues) {}
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		std::string toString() const override;
	private:
		std::vector<std::string> validValues_;
	};
	
	// Global instances of the "core" supported rules
	extern const GreaterThanZero greaterThanZero;
	extern const GreaterThanOrEqualToZero greaterThanOrEqualToZero;
	
	// functions for "core" supported rules with parameters
	//BetweenValues betweenValues(double min, double max);
	//InList inList(const std::vector<std::string>& validValues);
	Rule* betweenValues(double min, double max);
    Rule* inList(const std::vector<std::string>& validValues);

}

#endif