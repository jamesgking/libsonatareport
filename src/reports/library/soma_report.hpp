#include "report.hpp"

class SomaReport: public Report {

  public:
    SomaReport(const std::string& reportName, double tstart, double tend, double dt);

    size_t get_total_elements() const override;
    int add_variable(uint64_t node_id, double* element_value, uint32_t compartment_id) override;

    bool check_add_variable(uint64_t node_id);
};