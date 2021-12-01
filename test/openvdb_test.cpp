#ifdef _WIN32
#pragma comment(linker, "/subsystem:console")
#endif

#include "openvdb/openvdb.h"

#include <iostream>

#include "vdb/Utilities.h"
#include "vdb/vdb.h"

bool m_vdbLoaded = false;
VDB *m_vdb;
bool m_extLoaded = false;

/// @brief value used to determine which points should be hidden for LOD -
/// uploaded to shader
float m_s;

void loadVDBFile(std::string _file) {
  if (m_vdbLoaded) {
    // remove an old file if it has been loaded
    delete m_vdb;
    m_vdbLoaded = false;
  }

  // give the user the option to load High resolution when first loading or wait
  // unitl later
  /* QMessageBox::StandardButton replyLoad, replyCont;
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

  // TODO : Update this value whenever you want to laod extra data
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

  // I want to frame the entire file in shot when loaded
  // to do this first need to calculate if the width or height of the BBox is
  // greatest

  float edgeLength = m_vdb->getBBox().width();
  if (m_vdb->getBBox().width() < m_vdb->getBBox().height()) {
    edgeLength = m_vdb->getBBox().height();
  }

  // find the half of the length
  float halfEdgeLength = edgeLength / 2.0f;

  // now find the half way point dependent on width or height
  openvdb::Vec3f halfWay = m_vdb->getBBox().centre();

  // to find length of z distance, with a viewing angle of 45 degrees it is a
  // tan theta
  float zLength = halfEdgeLength * tan(Utilities::u_radians(67.5));

  openvdb::Vec3f from;
  from[0] = halfWay[0];
  // move up by a quarter of the bounding box height to give some kind of angle
  // on the view
  from[1] = halfWay[1] + m_vdb->getBBox().height() * 0.25f;
  // multiple zLength by 1.5 to ensure the model will fit onto screen
  from[2] = zLength * 1.5f;

  openvdb::Vec3f to(halfWay);

  // TODO
  // openvdb::Vec4f currentEye  = m_camera->getEye();
  // openvdb::Vec4f currentLook = m_camera->getLook();

  // find the difference between the new position an the current positions
  /* m_currentBaseCameraFrom =
       openvdb::Vec4f(from.x(), from.y(), from.z(), 1.0) - currentEye;
   m_currentBaseCameraTo =
       openvdb::Vec4f(to.x(), to.y(), to.z(), 1.0) - currentLook;

   m_camera->moveEye(m_currentBaseCameraFrom.x(), m_currentBaseCameraFrom.y(),
                     m_currentBaseCameraFrom.z());
   m_camera->moveLook(m_currentBaseCameraTo.x(), m_currentBaseCameraTo.y(),
                      m_currentBaseCameraTo.z());*/

  // set base vectors so that the camera can be returned to these values on a
  // reset once a transformation has taken place
  // m_currentBaseCameraFrom = m_camera->getEye();
  // m_currentBaseCameraTo   = m_camera->getLook();

  // updateGL();
}

// void paintGL() {
//  // update the wheel move amount depending on the distance from the centre of
//  // the bounding box the closer the smaller the amount to move, the further
//  the
//  // greater
//  setWheelZoomSpeed();
//
//  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//  std::string shaderToUse;
//
//  // choose which shader needs to be used for the currently chosen settings
//  if (!m_useCrop) {
//    if (m_colourMapApplied) {
//      shaderToUse = "ColourMap";
//    } else {
//      shaderToUse = "Normal";
//    }
//  } else {
//    if (m_colourMapApplied) {
//      shaderToUse = "ColourMapCrop";
//    } else {
//      shaderToUse = "NormalCrop";
//    }
//  }
//
//  // default all matrices used for transformations
//  openvdb::Mat4s transform;
//  transform.setIdentity();
//  openvdb::Mat4s rotX, rotY;
//  rotX.setIdentity();
//  rotY.setIdentity();
//
//  if (!m_moveAroundOrigin) {
//    // move the object itself
//    if (m_vdbLoaded && !m_resetTransform) {
//      // if not resetting the transform, get new transform and set top VDB
//      rotX.setToRotation(openvdb::Vec3f(1.0, 0.0, 0.0), -m_spinX / 50.0f);
//      rotY.setToRotation(openvdb::Vec3f(0.0, 1.0, 0.0), -m_spinY / 50.0f);
//
//      transform = (rotX * rotY);
//      transform.setTranslation(m_mouseTranslate);
//
//      m_vdb->setTransform(transform);
//    }
//    // if resetting do not set a new transform, instead use the current
//  } else {
//    // move the camera around the scene
//    openvdb::Mat4s t, it, r;
//    t.setIdentity();
//    it.setIdentity();
//    r.setIdentity();
//    openvdb::Vec4f fp;
//
//    openvdb::Vec4f diff = m_camera->getEye();
//    t.setTranslation(diff.getVec3());
//    it = t.inverse();
//    rotY.setToRotation(openvdb::Vec3f(0.0f, 1.0f, 0.0f), m_diffY / 50.0f);
//    rotX.setToRotation(openvdb::Vec3f(1.0f, 0.0f, 0.0f), m_diffX / 50.0f);
//    // combine rotations
//    r = rotX * rotY;
//
//    transform = it * r * t;
//
//    fp = transform * diff;
//
//    openvdb::Vec4f now = fp - diff;
//    // move the placement of the camera
//    m_camera->moveEye(now.x(), now.y(), now.z());
//  }
//
//  if (m_vdbLoaded) {
//    openvdb::Mat4s MVP;
//    openvdb::Mat4s M;
//    // get the transform matrix of the vdb file
//    M   = m_vdb->transform();
//    MVP = M * m_camera->getViewProjectionMatrix();
//
//    // if using crop boxes, draw the crop boxes
//    if (m_useCrop) {
//      // here I will set the limits required to the correct values
//      // these will then be used outside this if statement to actually crop
//      the
//      // drawing
//      openvdb::Mat4s localM;
//      localM.setIdentity();
//      openvdb::Mat4s localMVP = localM * m_camera->getViewProjectionMatrix();
//      m_shaderLib->setActive("Colour");
//      m_shaderLib->setShaderUniformFromMat4("MVP", localMVP);
//      // craw the crop boxes
//      m_vdb->drawCrop(m_shaderLib);
//    }
//
//    // now move to actual shader to use and continue
//    m_shaderLib->setActive(shaderToUse);
//
//    if (m_useCrop) {
//      m_shaderLib->setShaderUniformFromMat4("u_transform",
//      m_vdb->transform()); uploadCropBoxesToShader();
//    }
//
//    m_shaderLib->setShaderUniformFromMat4("MVP", MVP);
//    m_shaderLib->setShaderParam1f("u_s", m_s);
//
//    if (m_cullingEnabled == 1) {
//      // set culling to true
//      m_shaderLib->setShaderParam1i("u_cull", 1);
//    } else {
//      m_shaderLib->setShaderParam1i("u_cull", 0);
//    }
//
//    if (m_drawMesh) {
//      if (m_colourMapApplied) {
//        m_shaderLib->setShaderParam4f("u_colour", m_colourMapColour.x(),
//                                      m_colourMapColour.y(),
//                                      m_colourMapColour.z(), 1.0);
//        m_shaderLib->setShaderParam1i("u_rampPoints", m_rampPoints);
//        if (m_rampPoints) {
//          if (!m_userDefinedRamp) {
//            // automatic ramping so get extremes of the bounding box of the
//            file m_channelRampExtremes = m_vdb->getBBox().asBBoxBare();
//          }
//          m_shaderLib->setShaderParam3f("u_Min", m_channelRampExtremes.minx,
//                                        m_channelRampExtremes.miny,
//                                        m_channelRampExtremes.minz);
//          m_shaderLib->setShaderParam3f("u_Max", m_channelRampExtremes.maxx,
//                                        m_channelRampExtremes.maxy,
//                                        m_channelRampExtremes.maxz);
//        } else {
//          if (!m_userDefinedRamp) {
//            // automatic ramping
//            m_channelRampExtremes = m_vdb->getCurrentChannelExtremes();
//            if (m_colourMapOnCull) {
//              // if ramping onto a cull, upload the correct data to the
//              correct
//              // location
//              setMapOnCullValues();
//            }
//          }
//          // upload the extremes of the channel chosen
//          m_shaderLib->setShaderParam3f("u_Min", m_channelRampExtremes.minx,
//                                        m_channelRampExtremes.miny,
//                                        m_channelRampExtremes.minz);
//          m_shaderLib->setShaderParam3f("u_Max", m_channelRampExtremes.maxx,
//                                        m_channelRampExtremes.maxy,
//                                        m_channelRampExtremes.maxz);
//        }
//      }
//      if (m_cullingEnabled) {
//        // if culling is being used, upload all the culls to the shader
//        uploadCullsToShader();
//      }
//      // draw the vdb file
//      m_vdb->drawVDB();
//    }
//
//    if (m_drawBBox) {
//      m_shaderLib->setActive("Colour");
//      m_shaderLib->setShaderUniformFromMat4("MVP", MVP);
//
//      openvdb::Vec3f colour = m_vdb->getBBox().colour();
//
//      float r = colour.x();
//      float g = colour.y();
//      float b = colour.z();
//
//      m_shaderLib->setShaderParam4f("u_colour", r, g, b, 1.0f);
//      // draw the bounding box
//      m_vdb->drawBBox();
//    }
//
//    if (m_drawVDBTree) {
//      m_shaderLib->setActive("VDBTree");
//      m_shaderLib->setShaderUniformFromMat4("MVP", MVP);
//      // darw the VDB tree
//      m_vdb->drawTree(m_shaderLib);
//    }
//
//    if (m_drawVectors) {
//      // ame format as drawing the mesh although this time it will draw the
//      // vectors
//      if (!m_useCrop) {
//        m_shaderLib->setActive("Vector");
//      } else {
//        m_shaderLib->setActive("VectorCrop");
//        m_shaderLib->setShaderUniformFromMat4("u_transform",
//                                              m_vdb->transform());
//        // upload crop boxes to the shader
//        uploadCropBoxesToShader();
//      }
//      m_shaderLib->setShaderUniformFromMat4("MVP", MVP);
//      m_shaderLib->setShaderParam1f("u_s", m_s);
//      m_shaderLib->setShaderParam1i("u_invert", m_invertVectorColour);
//      m_shaderLib->setShaderParam1i("u_renderMode", m_vectorColourMode);
//      m_shaderLib->setShaderParam1f("u_vectorSize", m_vdb->vectorSize());
//
//      if (m_cullingEnabled) {
//        // upload culling information
//        m_shaderLib->setShaderParam1i("u_cull", 1);
//        uploadCullsToShader();
//      } else {
//        m_shaderLib->setShaderParam1i("u_cull", 0);
//      }
//
//      m_vdb->drawVectors(m_shaderLib);
//    }
//    m_resetTransform = false;
//  }
//
//  if (m_drawGrid) {
//    // use a local transform so the grid will not rotate along with the VDB
//    // volume
//    openvdb::Mat4s localMVP =
//        m_grid->transform() * m_camera->getViewProjectionMatrix();
//    m_shaderLib->setActive("Colour");
//    m_shaderLib->setShaderUniformFromMat4("MVP", localMVP);
//    m_shaderLib->setShaderParam4f("u_colour", 0.6f, 0.6f, 0.6f, 1.0f);
//    // draw the grid
//    m_grid->draw();
//  }
//
//  // update the UI output of the current used GPU memory
//  emit(updateUsedGPUMem());
//}

int main() {
  // Initialize the OpenVDB library.  This must be called at least
  // once per program and may safely be called multiple times.
  // openvdb::initialize();
  //// Create an empty floating-point grid with background value 0.
  // openvdb::FloatGrid::Ptr grid = openvdb::FloatGrid::create();
  // std::cout << "Testing random access:" << std::endl;
  //// Get an accessor for coordinate-based access to voxels.
  // openvdb::FloatGrid::Accessor accessor = grid->getAccessor();
  //// Define a coordinate with large signed indices.
  // openvdb::Coord xyz(1000, -200000000, 30000000);
  //// Set the voxel value at (1000, -200000000, 30000000) to 1.
  // accessor.setValue(xyz, 1.0);
  //// Verify that the voxel value at (1000, -200000000, 30000000) is 1.
  // std::cout << "Grid" << xyz << " = " << accessor.getValue(xyz) << std::endl;
  //// Reset the coordinates to those of a different voxel.
  // xyz.reset(1000, 200000000, -30000000);
  //// Verify that the voxel value at (1000, 200000000, -30000000) is
  //// the background value, 0.
  // std::cout << "Grid" << xyz << " = " << accessor.getValue(xyz) << std::endl;
  //// Set the voxel value at (1000, 200000000, -30000000) to 2.
  // accessor.setValue(xyz, 2.0);
  //// Set the voxels at the two extremes of the available coordinate space.
  //// For 32-bit signed coordinates these are (-2147483648, -2147483648,
  //// -2147483648) and (2147483647, 2147483647, 2147483647).
  // accessor.setValue(openvdb::Coord::min(), 3.0f);
  // accessor.setValue(openvdb::Coord::max(), 4.0f);
  // std::cout << "Testing sequential access:" << std::endl;
  //// Print all active ("on") voxels by means of an iterator.
  // for (openvdb::FloatGrid::ValueOnCIter iter = grid->cbeginValueOn(); iter;
  //     ++iter) {
  //  std::cout << "Grid" << iter.getCoord() << " = " << *iter << std::endl;
  //}
  loadVDBFile("D:/GitHub/Volume-ReSTIR-Vulkan/smoke1/smoke.vdb");
}
