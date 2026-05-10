#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>

namespace ConfigLib {
    class ConfigValue;
    class NumericConfigValue;
}
namespace ValidationRules 
{

	class Rule 
	{
    public:
        virtual ~Rule() {}
        virtual bool operator()(const ConfigLib::ConfigValue& value) const = 0;
        [[nodiscard]] virtual std::string toString() const = 0;
    };
	
	class GreaterThanZero : public Rule
	{
    public:
        bool operator()(const ConfigLib::ConfigValue& value) const override;
        [[nodiscard]] std::string toString() const override { return "Must be greater than zero"; }

        template<typename T> requires std::is_arithmetic_v<T>
        [[nodiscard]] static constexpr bool check(T v) noexcept { return v > T{0}; }
        [[nodiscard]] static const Rule* rulePtr() noexcept;
    };

	class GreaterThanOrEqualToZero : public Rule
	{
	public:
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		[[nodiscard]] std::string toString() const override { return "Must be greater than or equal to zero"; }

        template<typename T> requires std::is_arithmetic_v<T>
        [[nodiscard]] static constexpr bool check(T v) noexcept { return v >= T{0}; }
        [[nodiscard]] static const Rule* rulePtr() noexcept;
	};
	
	class BetweenValues : public Rule 
	{
	public:
		BetweenValues(double min, double max) : min_(min), max_(max) {}
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		[[nodiscard]] std::string toString() const override;
	private:
		double min_;
		double max_;
	};

	class InList : public Rule
	{
	public:
		InList(const std::vector<std::string>& validValues) : validValues_(validValues) {}
		bool operator()(const ConfigLib::ConfigValue& value) const override;
		[[nodiscard]] std::string toString() const override;
	private:
		std::vector<std::string> validValues_;
	};
	
	// Global instances of the "core" supported rules
	extern const GreaterThanZero greaterThanZero;
	extern const GreaterThanOrEqualToZero greaterThanOrEqualToZero;
	
	/* factory functions for "core" supported rules with parameters
	 * The caller is responsible for keeping the rule alive for the lifetime 
	 * of any and all ConfigItems that use the validation rule 
	 * 
	 * e.g.,
	 * static const auto between0And100 = ValidationRules::betweenValues(0, 100);
	 * return {{ "Section", { {"key", "double", "1.0", "desc", between0And100.get()} }}};
	 * */
	[[nodiscard]] std::unique_ptr<Rule> betweenValues(double min, double max);
	[[nodiscard]] std::unique_ptr<Rule> inList(const std::vector<std::string>& validValues);

} // namespace ValidationRules
