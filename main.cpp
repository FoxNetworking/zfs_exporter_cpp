#include <cassert>
#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <libzfs/libzfs.h>

zprop_source_t source;

double getPropDouble(zpool_handle_t *poolHandle, zpool_prop_t poolFlag) {
  char *value = new char[ZFS_MAXPROPLEN];

  // Obtain property for given pool with max length
  int result = zpool_get_prop(poolHandle, poolFlag, value,
                              std::numeric_limits<int>::max(), &source, B_TRUE);
  assert(result == 0);

  // Convert given property string to double
  std::string::size_type sz;
  double final = std::stod(value, &sz);
  delete[] value;

  return final;
}

int main(int argc, char **argv) {
  libzfs_handle_t *zfsHandle = libzfs_init();
  assert(zfsHandle != nullptr);

  // create an http server running on port 8080
  prometheus::Exposer exposer{"[::1]:9312"};

  // create a metrics registry with component=main labels applied to all its
  // metrics
  auto registry = std::make_shared<prometheus::Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto &alloc_size_family = prometheus::BuildGauge()
                                .Name("rpool_alloc_size")
                                .Help("ZFS Allocated Bytes")
                                .Register(*registry);

  auto &capacity_family = prometheus::BuildGauge()
                              .Name("rpool_capacity")
                              .Help("ZFS Pool Capacity Percentage")
                              .Register(*registry);

  auto &total_size_family = prometheus::BuildGauge()
                                .Name("rpool_total_size")
                                .Help("ZFS Pool Size")
                                .Register(*registry);

  auto &alloc_size = alloc_size_family.Add({{"pool_name", "rpool"}});
  auto &capacity = capacity_family.Add({{"pool_name", "rpool"}});
  auto &total_size = total_size_family.Add({{"pool_name", "rpool"}});

  // ask the exposer to scrape the registry on incoming scrapes
  exposer.RegisterCollectable(registry);

  for (;;) {
    // Obtain property values from zpool
    zpool_handle_t *poolHandle = zpool_open(zfsHandle, "rpool");
    double allocated = getPropDouble(poolHandle, ZPOOL_PROP_ALLOCATED);
    double size = getPropDouble(poolHandle, ZPOOL_PROP_SIZE);
    zpool_close(poolHandle);

    // Update counters
    alloc_size.Set(allocated);
    total_size.Set(size);
    capacity.Set(allocated * (100 / size));

    std::this_thread::sleep_for(std::chrono::seconds(2));
  }

  return 0;
}