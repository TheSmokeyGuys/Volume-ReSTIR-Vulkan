#include <cstdio>
#include <cstdlib>

#include "Camera.hpp"
#include "Renderer.hpp"
#include "SingtonManager.hpp"
#include "VkBootstrap.h"
#include "config/static_config.hpp"
#include "spdlog/spdlog.h"
#include "vdb/Utilities.h"
#include "vdb/vdb.h"

#define DEBUG

using namespace volume_restir;
//const std::string file = std::string(BUILD_DIRECTORY) + "../assets/smoke.vdb";
 //const std::string file = "C:/Users/yangr/Git_Repo/CIS565/Final/Volume-ReSTIR-Vulkan/assets/smoke.vdb"; //Raymond
// const std::string file = "D:/GitHub/Volume-ReSTIR-Vulkan/assets/smoke.vdb";  // Shubham
 //const std::string file = "D:/GitHub/Volume-ReSTIR-Vulkan/assets/bunny_cloud.vdb";  // Shubham
 const std::string file = "D:/GitHub/Volume-ReSTIR-Vulkan/assets/fire.vdb";  // Shubham
//const std::string file =
//    "C:/Users/shine/repos/Volume-ReSTIR-Vulkan/assets/smoke.vdb";  // Zhihao

bool m_vdbLoaded = false;
VDB* m_vdb;
bool m_extLoaded = false;
float m_s;

void loadVDBFile(std::string _file) {
  if (m_vdbLoaded) {
    // remove an old file if it has been loaded
    delete m_vdb;
    m_vdbLoaded = false;
  }

  // give the user the option to load High resolution when first loading or wait
  // unitl later
  /*QMessageBox::StandardButton replyLoad, replyCont;
   QMessageBox *loadHigh = new QMessageBox(this);
   loadHigh->setWindowFlags(Qt::WindowStaysOnTopHint);
   loadHigh->setWindowModality(Qt::WindowModal);
   replyLoad =
       loadHigh->question(this, "Loading VDB File", "Load High Resolution",
                          QMessageBox::Yes | QMessageBox::No);*/
  // bring to the top
  // loadHigh->raise();

  // if (replyLoad == QMessageBox::Yes) {
  //  // if the user has chosen to load the high resolution data, show them the
  //  // performance warning and check to make sure they are happy with their
  //  // decision
  //  QMessageBox confirm(this);
  //  confirm.setWindowModality(Qt::WindowModal);
  //  confirm.raise();
  //  replyCont = confirm.question(
  //      NULL, "Loading High Resolution",
  //      "Loading High Resolution volumes can be memory intensive and time "
  //      "consuming. Are you sure you wish to continue?",
  //      QMessageBox::Yes | QMessageBox::No);
  //  confirm.close();
  //}

  m_vdb = new VDB(_file);

  // TODO
  // Set Total GPU Memory need to check what we use this for
  // m_vdb->setTotalGPUMemKB(m_total_GPU_mem_kb);

  // load the basic iformation from the file
  if (m_vdb->loadBasic()) {
#ifdef DEBUG
    std::cout << "Load complete!!" << std::endl;
#endif
  } else {
    std::cerr << "Error whilst loading file" << std::endl;
  }

  // TODO : Update this value whenever you want to load extra data
  m_extLoaded = true;
  //  if (replyLoad == QMessageBox::Yes && replyCont == QMessageBox::Yes) {
  //    // if it has been chosen to load high resolution, load
  //    if (m_vdb->loadExt()) {
  //#ifdef DEBUG
  //      std::cout << "High resolution load complete!!" << std::endl;
  //#endif
  //      m_extLoaded = true;
  //    } else {
  //      std::cerr << "Error whilst loading high resolution volume" <<
  //      std::endl;
  //    }
  //  }

  // if it has been chosen to load high resolution, load
  if (m_vdb->loadExt()) {
#ifdef DEBUG
    std::cout << "High resolution load complete!!" << std::endl;
#endif
    m_extLoaded = true;
  } else {
    std::cerr << "Error whilst loading high resolution volume" << std::endl;
  }
  // get the s value of the current channel
  m_s = m_vdb->getS();

  m_vdbLoaded = true;

#ifdef EXT_GPU
#ifdef NVIDIA
#ifdef DEBUG
  std::cout << "Current available GPU memory: "
            << currentAvailableGPUMemKB() / 1024 << "MB" << std::endl;
#endif
  emit(updateUsedGPUMem());
#else
#ifdef DEBUG
  printAMDNotSupported();
#endif
#endif
#else
#ifdef DEBUG
  // printNoExtGPU();
#endif
#endif

  // Initialise all crop boxes
  //=====================================================
  // m_vdb->setCrop(-1.0f, 1.0f, 0.0f, 20.0f, -5.0f, 5.0f, 0);
  // m_vdb->setCrop(-1.0f, 1.0f, 0.0f, 20.0f, -5.0f, 5.0f, 1);
  // m_vdb->setCrop(-1.0f, 1.0f, 0.0f, 20.0f, -5.0f, 5.0f, 2);
  // m_vdb->setCrop(-1.0f, 1.0f, 0.0f, 20.0f, -5.0f, 5.0f, 3);
  // m_vdb->setCrop(-1.0f, 1.0f, 0.0f, 20.0f, -5.0f, 5.0f, 4);

  // m_vdb->setCropColour(openvdb::Vec3f(0.0f, 1.0f, 1.0f), 0);
  // m_vdb->setCropColour(openvdb::Vec3f(0.5f, 1.0f, 0.5f), 1);
  // m_vdb->setCropColour(openvdb::Vec3f(1.0f, 0.5f, 1.0f), 2);
  // m_vdb->setCropColour(openvdb::Vec3f(1.0f, 0.5f, 0.5f), 3);
  // m_vdb->setCropColour(openvdb::Vec3f(0.5f, 1.0f, 0.0f), 4);
  //=====================================================
}

int main() {
#ifdef DEBUG
  spdlog::set_level(spdlog::level::debug);
#endif
  spdlog::info("Hello from Volumetric-ReSTIR project!");

  loadVDBFile(file);

  Renderer renderer(m_vdb);

  while (!SingletonManager::GetWindow().ShouldQuit()) {
    glfwPollEvents();
    renderer.Draw();
  }

  // vkDeviceWaitIdle(renderer.RenderContextPtr()->Device().device);

#ifdef _WIN32
  system("pause");
#endif

  return 0;
}
