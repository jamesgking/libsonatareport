#include <iostream>

#include "../utils/logger.h"
#include "element_report.h"
#include "implementation_interface.hpp"
#include "soma_report.h"
#include "sonatareport.h"

namespace bbp {
namespace sonata {

double SonataReport::atomic_step_ = 1e-8;
double SonataReport::min_steps_to_record_ = 0.0;
bool SonataReport::first_report = true;
int SonataReport::rank_ = 0;
#ifdef SONATA_REPORT_HAVE_MPI
MPI_Comm SonataReport::has_nodes_ = MPI_COMM_WORLD;
SonataReport::communicators_t SonataReport::communicators_;
#endif

void SonataReport::clear() {
    for (auto& kv : reports_) {
        logger->trace("Deleting report: {} from rank {}", kv.first, SonataReport::rank_);
    }
    reports_.clear();
}

bool SonataReport::is_empty() {
    return reports_.empty();
}

std::shared_ptr<Report> SonataReport::create_report(const std::string& name,
                                                    const std::string& kind,
                                                    double tstart,
                                                    double tend,
                                                    double dt,
                                                    const std::string& units) {
    if (kind == "compartment" || kind == "synapse" || kind == "summation") {
        reports_.emplace(name, std::make_shared<ElementReport>(name, tstart, tend, dt, units));
    } else if (kind == "soma") {
        reports_.emplace(name, std::make_shared<SomaReport>(name, tstart, tend, dt, units));
    } else {
        throw std::runtime_error("Kind '" + kind + "' doesn't exist!");
    }
    logger->trace("Creating report {} type {} tstart {} and tstop {} from rank {}",
                  name,
                  kind,
                  tstart,
                  tend,
                  rank_);

    return get_report(name);
}

std::shared_ptr<Report> SonataReport::get_report(const std::string& name) const {
    return reports_.at(name);
}

bool SonataReport::report_exists(const std::string& name) const {
    return reports_.find(name) != reports_.end();
}

void SonataReport::create_communicators() {
    std::vector<std::string> report_names;
    report_names.reserve(reports_.size());
    for (auto iter = reports_.begin(); iter != reports_.end();) {
        if (iter->second->is_empty()) {
            // Remove reports without nodes
            iter = reports_.erase(iter);
        } else {
            report_names.push_back(iter->first);
            ++iter;
        }
    }
    // Create communicator groups
    rank_ = Implementation::init(report_names);
    if (rank_ == 0 && !is_empty()) {
        logger->info("Initializing communicators and preparing SONATA datasets");
    }
}

void SonataReport::prepare_datasets() {
    for (auto& kv : reports_) {
        logger->trace("Preparing datasets of report {} from rank {}", kv.first, rank_);
        kv.second->prepare_dataset();
    }
}

void SonataReport::create_spikefile(const std::string& output_dir, const std::string& filename) {
    std::string report_name = output_dir + "/" + filename;
    spike_data_ = std::make_unique<SonataData>(report_name);
}

void SonataReport::add_spikes_population(const std::string& population_name,
                                         uint64_t population_offset,
                                         const std::vector<double>& spike_timestamps,
                                         const std::vector<uint64_t>& spike_node_ids,
                                         const std::string& order_by) {
    auto population = std::make_unique<Population>(
        population_name, population_offset, order_by, spike_timestamps, spike_node_ids);
    spike_data_->add_population(std::move(population));
}

void SonataReport::write_spike_populations() {
    spike_data_->write_spike_populations();
}

void SonataReport::close_spikefile() {
    spike_data_->close();
}

}  // namespace sonata
}  // namespace bbp
