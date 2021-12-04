#ifndef __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__
#define __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__

#include "loader/VDBLoader.hpp"
#include "Window.hpp"

namespace volume_restir {

class SingletonManager {
private:
  SingletonManager() {}

public:
  static Window& GetWindow() {
    static Window window;
    return window;
  }

  static VDBLoader& GetVDBLoader() {
    static VDBLoader loader;
    return loader;
  }
};

}  // namespace volume_restir

#endif /* __VOLUME_RESTIR_SINGLETON_MANAGER_HPP__ */
