#include "config_library/config_reader.hpp"
#include "config_library/validation_rules.hpp"
#include <iostream>
#include <vector>
#include <random>
#include <cmath>
#include <numeric>

class MonteCarloConfig : public ConfigLib::ConfigReader {
public:
    MonteCarloConfig() : ConfigReader() {
        std::cout << "MonteCarloConfig constructor started" << std::endl;
        initialize();
        std::cout << "MonteCarloConfig constructor finished" << std::endl;
    }

    std::string getConfigFilePath() const override {
        return "monte_carlo_config.ini";
    }

    std::vector<ConfigLib::ConfigGen::ConfigSection> getConfigSections() const override 
    {
        return {
            {
                "Simulation",
                {
                    {"num_simulations", "int", "10000", "Number of Monte Carlo simulations", &ValidationRules::greaterThanZero},
                    {"initial_price", "double", "100.0", "Initial asset price", &ValidationRules::greaterThanZero},
                    {"time_horizon", "double", "1.0", "Time horizon in years", &ValidationRules::greaterThanZero},
                    {"num_steps", "int", "252", "Number of time steps", &ValidationRules::greaterThanZero},
                    {"risk_free_rate", "double", "0.05", "Risk-free interest rate", &ValidationRules::greaterThanOrEqualToZero},
                    {"volatility", "double", "0.2", "Asset price volatility", &ValidationRules::greaterThanZero}
                }
            }
        };
    }
};

class MonteCarloSimulation {
public:
    MonteCarloSimulation() : config(), rng(std::random_device{}()) {}

    double runSimulation() const {
        int num_simulations = config.getValue<int>("Simulation", "num_simulations");
		std::cout << "num_simulations: " << std::to_string(num_simulations) << std::endl; 
        double initial_price = config.getValue<double>("Simulation", "initial_price");
        double time_horizon = config.getValue<double>("Simulation", "time_horizon");
        int num_steps = config.getValue<int>("Simulation", "num_steps");
        double risk_free_rate = config.getValue<double>("Simulation", "risk_free_rate");
        double volatility = config.getValue<double>("Simulation", "volatility");

        double dt = time_horizon / num_steps;
        double drift = (risk_free_rate - 0.5 * volatility * volatility) * dt;
        double diffusion = volatility * std::sqrt(dt);

        std::vector<double> final_prices;
        final_prices.reserve(num_simulations);

        std::normal_distribution<> normal(0, 1);

        for (int i = 0; i < num_simulations; ++i) {
            double price = initial_price;
            for (int step = 0; step < num_steps; ++step) {
                double z = normal(rng);
                price *= std::exp(drift + diffusion * z);
            }
            final_prices.push_back(price);
        }

        double sum = std::accumulate(final_prices.begin(), final_prices.end(), 0.0);
        double mean = sum / num_simulations;

        double sq_sum = std::inner_product(final_prices.begin(), final_prices.end(), final_prices.begin(), 0.0);
        double stdev = std::sqrt(sq_sum / num_simulations - mean * mean);

        std::cout << "Mean final price: " << mean << std::endl;
        std::cout << "Standard deviation: " << stdev << std::endl;

        return mean;
    }

private:
    MonteCarloConfig config;
    mutable std::mt19937 rng;
};

int main() {
    std::cout << "Monte Carlo Simulation started" << std::endl;
    
    try {
        MonteCarloSimulation simulation;
		
        double result = simulation.runSimulation();

        std::cout << "Simulation completed. Final result: " << result << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    std::cout << "Program finished" << std::endl;
    return 0;
}