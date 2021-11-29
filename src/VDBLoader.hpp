#ifndef __VOLUME_RESTIR_VDB_LOADER_HPP__
#define __VOLUME_RESTIR_VDB_LOADER_HPP__

#include <memory>

#include "vdb/vdb.h"

namespace volume_restir {

class VDBLoader {
public:
  VDBLoader()
      : is_vdb_loaded_(false),
        is_basic_loaded_(false),
        is_detail_loaded_(false) {}

  VDB* GetPtr() { return vdb_.get(); }
  bool IsVDBLoaded() const { return is_vdb_loaded_; }

  void Load(const std::string filename);

private:
  std::unique_ptr<VDB> vdb_;
  bool is_vdb_loaded_;
  bool is_basic_loaded_;
  bool is_detail_loaded_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_VDB_LOADER_HPP__ */
