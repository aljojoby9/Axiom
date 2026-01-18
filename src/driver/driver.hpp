#pragma once

/**
 * @file driver.hpp
 * @brief Compiler driver to orchestrate the build process
 */

#include <string>
#include <vector>
#include <map>

namespace axiom {

struct CompilerConfig {
    std::string input_file;
    std::string output_file;
    bool emit_ir = false;
    bool emit_obj = true;
    bool run_linker = true;
    bool verbose = false;
    int optimization_level = 0; // 0, 1, 2, 3
};

class Driver {
public:
    explicit Driver(const CompilerConfig& config);
    
    /**
     * @brief Run the full compilation pipeline
     * @return 0 on success, non-zero on failure
     */
    int run();

private:
    CompilerConfig config_;
    
    // Pipeline steps
    int compile_to_object();
    int invoke_linker();
    
    // Helpers
    std::string get_output_filename(const std::string& ext);
    std::string find_linker();
    int execute_command(const std::string& cmd);
};

} // namespace axiom
