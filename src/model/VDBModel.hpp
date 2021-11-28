#ifndef __VOLUME_RESTIR_MODEL_VDB_HPP__
#define __VOLUME_RESTIR_MODEL_VDB_HPP__

#include "ModelBase.hpp"
#include "vdb/vdb.h"

namespace volume_restir {

class VDBModel : public ModelBase {
public:
  VDBModel(RenderContext* render_context, VkCommandPool command_pool,
           VDB* p_vdb);

private:
  VDB* vdb_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_MODEL_VDB_HPP__ */
