#ifndef __VOLUME_RESTIR_SCENE_HPP__
#define __VOLUME_RESTIR_SCENE_HPP__

/**
 * @file Scene.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief A scene holds all objects within the rendering scene. It holds all
 * objects in the world.
 *
 * @date 2021-11-28
 */

#include <memory>
#include <vector>

#include "RenderContext.hpp"
#include "model/ModelBase.hpp"

namespace volume_restir {

class Scene {
public:
  /**
   * @brief Creates a default scene in constructor
   *
   * @param render_context
   */
  Scene(RenderContext* render_context);

  const std::vector<std::unique_ptr<ModelBase>>& GetObjects() const {
    return objects_;
  }

  void AddObject(std::unique_ptr<ModelBase>& object) {
    objects_.push_back(std::move(object));
  }

private:
  RenderContext* render_context_;
  std::vector<std::unique_ptr<ModelBase>> objects_;
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_SCENE_HPP__ */
