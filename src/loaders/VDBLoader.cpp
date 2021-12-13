#include "loaders/VDBLoader.hpp"

#include "utils/logging.hpp"

void VDBLoader::Load(const std::string filename) {
  spdlog::info("Loading VDB file from: {}", filename);

  // give the user the option to load High resolution when first loading or wait
  // until later
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

  // load the VDB file
  vdb_ = std::make_unique<VDB>(filename);

  // load the basic information from the file
  spdlog::info("Loading Basic information from VDB...");
  if (vdb_->loadBasic()) {
    spdlog::info("Load complete!!");
    is_basic_loaded_ = true;
  } else {
    spdlog::error("Error whilst loading file");
  }

  // Update this value whenever you want to load extra data
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
  spdlog::info("Loading High-Res information from VDB...");
  if (vdb_->loadExt()) {
    spdlog::info("High resolution load complete!!");
    is_detail_loaded_ = true;
  } else {
    spdlog::error("Error whilst loading high resolution volume");
  }

  is_vdb_loaded_ = true;

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
