#ifndef __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__
#define __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__

#include "common/gltf_loader.hpp"
#include "loaders/VDBLoader.hpp"

class SingletonManager {
private:
  SingletonManager() {}

public:
  static VDBLoader& GetVDBLoader() {
    static VDBLoader loader;
    return loader;
  }

  static GLTFLoader& GetGLTFLoader() {
    static GLTFLoader loader;
    return loader;
  }
};

#endif /* __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__ */
