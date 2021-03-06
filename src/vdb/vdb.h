#pragma once

/*
  Copyright (C) 2014 Callum James

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __VDB_H__
#define __VDB_H__

#include <openvdb/openvdb.h>

#include "BoundBox.h"
//#include "Camera.h"
//#include "ShaderLibrary.h"
#include "Vertex.hpp"

/// @file VDB.h
/// @brief VDB class in this file handles the loading, drawing and attribute
/// handling of any OpenVDB file loaded in
/// @author Callum James
/// @version 1.0
/// @date 12/02/2014
/// Revision History:
/// Initial Version 05/01/2014
/// @class VDB
/// @brief VDB class handles the loading of a VDB file. Separates the attributes
/// out and makes them accessible to other classes. Handles the templates used
/// within the OpenVDB library as well as all the drawing of the different
/// elements to the file. Adapts to files of different number of channels data.
/// This is where all the OpenVDB specific is done.
class VDB {
public:
  /// @brief Constructor of the VDB class
  VDB();
  /// @brief Constructor of the VDB class
  /// @param [in] _file std::string - file to load
  VDB(std::string _file);
  /// @brief Destructor of the VDB class
  ~VDB();

  /// @brief Basic initiliasation of class
  void init();

  // TODO
  ///// @brief Set the total available GPU memory in KB
  ///// @param [in] _mem GLint - memory
  inline void setTotalGPUMemKB(GLint _mem) { m_total_GPU_mem_kb = _mem; }

  /// @brief Open and Load data from VDB file
  /// @param [in] _file std::string - file to load
  void openFile(std::string _file);
  /// @brief Return if the file has been loaded or not - returns true or false
  inline bool loaded() { return m_loaded; }
  /// @brief Set transform of the VDB (global transform)
  /// @param [in] _matrix openvdb::Mat4s - passed in transform
  inline void setTransform(openvdb::Mat4s _matrix) { m_transform = _matrix; }
  /// @brief Return the current transform - returns openvdb::Mats4s
  inline openvdb::Mat4s transform() { return m_transform; }
  /// @brief Load the basic data for the file - file information and VDB tree
  bool loadBasic();
  /// @brief Load High resolution data
  bool loadExt();
  /// @brief Remove high resolution mesh data
  void removeMeshVAO();
  /// @brief Set the level of detail of the model - returns boolean
  bool changeLOD();

  /// @brief Set the channel to render as points
  /// @param [in] _channel int - the channel to render
  void setPointChannel(int _channel);
  /// @brief Set the channel to render as vectors
  /// @param [in] _channel int - the channel to render
  void setVectorChannel(int _channel);
  /// @brief Get the number of points in a particular grid - returns int
  /// @param [in] _grid int - grid to query
  int getNumPointsAtGrid(int _grid);

  // TODO
  ///// @brief Return the channel extremes of the current channel - returns
  ///// BBoxBare
  // inline BBoxBare getCurrentChannelExtremes() {
  //  return m_channelExtremes->at(m_currentActiveChannelPoints);
  //}
  ///// @brief Return the channel extremes of the specified channel - returns
  ///// BBoxBare
  ///// @param [in] _channel int - the channel to query
  // inline BBoxBare getChannelExtremesAt(int _channel) {
  //  return m_channelExtremes->at(_channel);
  //}

  /// @brief Return the current active point render channel - returns int
  inline int pointChannel() { return m_currentActiveChannelPoints; }
  /// @brief Return the current active vector render channel - returns int
  inline int vectorChannel() { return m_currentActiveChannelVectors; }

  /// @brief Draw the VDB volume
  void drawVDB();

  // TODO
  ///// @brief Draw bounding box
  // inline void drawBBox() { m_bbox->draw(); }

  // TODO
  ///// @brief Draw the VDB tree
  ///// @param [in] _shadLib ShaderLibrary* - active shader library to set
  /// values
  // void drawTree(ShaderLibrary *_shadLib);
  ///// @brief Draw the vectors
  ///// @param [in] _shadLib ShaderLibrary* - active shader library to set
  /// values
  // void drawVectors(ShaderLibrary *_shadLib);
  ///// @brief Draw active crop boxes
  ///// @param [in] _shadLib ShaderLibrary* - active shader library to set
  /// values
  // void drawCrop(ShaderLibrary *_shadLib);

  /// @brief Return the opened filename - returns std::string
  inline std::string filename() { return m_fileName; }
  /// @brief Set the vector size
  /// @param [in] _size float - the size to set the vectors to
  inline void setVectorSize(float _size) { m_vectorSize = _size; }

  /// @brief Set the vector colour
  /// @param [in] _colour openvdb::vec3f - the colour to set the vectors to
  inline void setVectorColour(openvdb::Vec3f _colour) {
    m_vectorColour = _colour;
  }
  /// @brief Get the colour of the vectors - returns openvdb::Vec3f
  inline openvdb::Vec3f vectorColour() { return m_vectorColour; }
  /// @brief Get the length of the vectors - returns float
  inline float vectorSize() { return m_vectorSize; }
  /// @brief Get the load percent factor - returns float
  inline float loadPercent() { return m_loadPercentFactor; }

  /// @brief Set the load percent factor
  /// @param [in] _delta float - load percent factor
  void changeLoadPercentFactor(float _delta);
  /// @brief Set the load percent factor
  /// @param [in] _delta int - load percent factor
  void changeLoadPercentFactor(int _delta);
  /// @brief Set the number of crop boxes to draw
  /// @param [in] _n int - the number of crop boxes to draw
  void setNumCropToDraw(int _n);
  /// @brief Get the number of crop boxes being drawn - returns int
  inline int getNumCropsDrawn() { return m_numCropsToDraw; }

  /// @brief Get the value s used for LOD at the current active channel -
  /// returns float
  float getS();
  /// @brief Get the value s used for LOD at the specified channel - returns
  /// float
  /// @param [in] _index int - the channel to get s from
  inline float getS(int _index) { return m_s.at(_index); }

  // TODO
  /// @brief Build the bounding box for the file
  /// @param [in] _minx float - minimum X value
  /// @param [in] _maxx float - maximum X value
  /// @param [in] _miny float - minimum Y value
  /// @param [in] _maxy float - maximum Y value
  /// @param [in] _minz float - minimum Z value
  /// @param [in] _maxz float - maximum Z value
  void buildBBox(float _minx, float _maxx, float _miny, float _maxy,
                 float _minz, float _maxz);

  /// @brief Get the Bounding Box - returns BoundBox
  inline BoundBox getBBox() { return *m_bbox; }

  std::vector<vDat> AllPoints;

  inline std::vector<volume_restir::Vertex> ToVertexArray() const {
    std::vector<volume_restir::Vertex> vertices;
    for (const auto &v : AllPoints) {
      volume_restir::Vertex vertex;
      vertex.pos    = nvmath::vec3f(v.x, v.y, v.z);
      vertex.normal = nvmath::normalize(
          nvmath::vec3f(v.x, v.y, v.z));  // using positions as normal for now
      vertex.tex_coord = nvmath::vec2f(v.u, v.v);
      vertex.color =
          nvmath::vec3f(v.nx, v.ny, v.nz);  // For Some weird Reason Normal
                                            // Values are same as Color values.
      // vertex.color     = nvmath::vec3f(1, 0, 0); // For Some weird Reason
      // Normal Values are same as Color values.
      vertices.push_back(vertex);
    }
    return vertices;
  }

  // TODO
  ///// @brief Get the crop box at the specified index - returns BoundBox
  ///// @param [in] _index int - the crop box to retrieve
  // inline BoundBox getCBox(int _index) { return m_crop[_index]; }
  ///// @brief Get the crop box from storage from the specified index
  ///// @param [in] _index int - the Crop box to get
  // inline BoundBox getCBoxStorage(int _index) { return m_cropStorage[_index];
  // }

  // TODO
  /// @brief Set cropbox at index
  /// @param [in] _min openvdb::Vec3f - minimum value
  /// @param [in] _max openvdb::Vec3f - maximum value
  /// @param [in] _index int - the crop box to set
  // void setCrop(openvdb::Vec3f _min, openvdb::Vec3f _max, int _index);
  ///// @brief Set cropbox at index
  ///// @param [in] _minx float - minimum X value
  ///// @param [in] _maxx float - maximum X value
  ///// @param [in] _miny float - minimum Y value
  ///// @param [in] _maxy float - maximum Y value
  ///// @param [in] _minz float - minimum Z value
  ///// @param [in] _maxz float - maximum Z value
  ///// @param [in] _index int - the crop box to set
  // void setCrop(float _minx, float _maxx, float _miny, float _maxy, float
  // _minz,
  //             float _maxz, int _index);
  ///// @brief Set cropbox at index
  ///// @param [in] _centre openvdb::Vec3f - centre of crop box
  ///// @param [in] _w float - width
  ///// @param [in] _h float - height
  ///// @param [in] _d float - depth
  ///// @param [in] _index int - the crop box to set
  // void setCrop(openvdb::Vec3f _centre, float _w, float _h, float _d,
  //             int _index);
  ///// @brief Set Crop Width at index
  ///// @param [in] _w float - width
  ///// @param [in] _index int - the crop box to set
  // void setCropW(float _w, int _index);
  ///// @brief Set Crop Height at index
  ///// @param [in] _h float - width
  ///// @param [in] _index int - the crop box to set
  // void setCropH(float _h, int _index);
  ///// @brief Set Crop Depth at index
  ///// @param [in] _d float - width
  ///// @param [in] _index int - the crop box to set
  // void setCropD(float _d, int _index);

  // TODO
  ///// @brief Set Crop at index
  ///// @param [in] _box BoundBox - passed in BoundBox to copy
  ///// @param [in] _index int - the crop box to set
  // void setCrop(BoundBox _box, int _index);
  ///// @brief Set Crop box centre at index
  ///// @param [in] _c openvdb::Vec3f - centre to set
  ///// @param [in] _index int - the crop box to set
  // inline void setCropCentre(openvdb::Vec3f _c, int _index) {
  //  m_crop[_index].setCentre(_c);
  //}
  ///// @brief Set Crop box centre at index
  ///// @param [in] _x float - x value of centre
  ///// @param [in] _y float - y value of centre
  ///// @param [in] _z float - z value of centre
  ///// @param [in] _index int - the crop box to set
  // inline void setCropCentre(float _x, float _y, float _z, int _index) {
  //  m_crop[_index].setCentre(_x, _y, _z);
  //}
  //
  // TODO
  /// @brief Set all crop boxes into storage
  // void setAllCropStorage();

  // TODO
  ///// @brief Set specified crop box to storage
  ///// @param [in] _index int - the crop box to store
  // void setCropStorage(int _index);
  ///// @brief Return all crop boxes from storage
  // void returnAllFromStorage();
  ///// @brief Set specified crop box to retrieve
  ///// @param [in] _index int - the crop box to retrieve
  // void returnFromStorage(int _index);

  // TODO
  ///// @brief Method to translate the specified crop
  ///// @param [in] _t openvdb::vec3f - the vector to translate by
  ///// @param [in] _index int - the crop box to set
  // inline void translateCrop(openvdb::Vec3f _t, int _index) {
  //  m_crop[_index].translate(_t);
  //}
  ///// @brief Method to colour the specified crop
  ///// @param [in] _colour openvdb::vec3f - the colour to set
  ///// @param [in] _index int - the crop box to set
  // inline void setCropColour(openvdb::Vec3f _colour, int _index) {
  //  m_crop[_index].setColour(_colour);
  //}
  ///// @brief Get the colour of the specified crop box - returns openvdb::Vec3f
  ///// @param [in] _index int - the crop box to query
  // inline openvdb::Vec3f cropColour(int _index) {
  //  return m_crop[_index].colour();
  //}

  // TODO
  ///// @brief Get stored available GPU memory in KB - returns GLint
  // inline GLint getCurrentAvailableGPUMemKB() {
  //  return m_current_available_GPU_mem_kb;
  //}

  /// @brief Values to specify whether to draw certain tree levels
  int m_drawTreeLevels[4];
  /// @brief Return the file version - returns std::string
  inline std::string fileVersion() { return m_fileVersion; }
  /// @brief Print out raw information about the file
  void printFileInformation();

  // functions used to return information about the file
  /// @brief Return the number of channels - returns int
  inline int numChannels() { return m_numChannels; }
  /// @brief Return the specified channel name - returns std::string
  /// @param [in] _channel int - channel to query
  inline std::string channelName(int _channel) {
    return m_variableNames[_channel];
  }
  /// @brief Return the specified channel type - returns std::string
  /// @param [in] _channel int - channel to query
  inline std::string channelType(int _channel) {
    return m_variableTypes[_channel];
  }
  /// @brief Return the grid dimensions from the specified channel - returns
  /// openvdb::Coord
  /// @param [in] _grid int - the channel to query
  openvdb::Coord gridDimAt(int _grid);
  /// @brief Return total voxel count - returns int
  inline int voxelCount() { return m_totalVoxels; }
  /// @brief Return total tree depth - returns int
  inline int treeDepth() { return m_treeDepth; }
  /// @brief Return count at tree depth - returns int
  /// @param [in] _depth int - depth to return count from
  int voxelCountAtTreeDepth(int _depth);
  /// @brief Return the number of metadata - returns int
  inline int numMeta() { return m_numMeta; }
  /// @brief Get the meta data name at - returns std::string
  /// @param [in] _index int - where to query for the data
  inline std::string metaNameAt(int _index) { return m_metaNames[_index]; }
  /// @brief Get the meta data value at - returns std::string
  /// @param [in] _index int - where to query for the data
  inline std::string metaValueAt(int _index) { return m_metaValues[_index]; }

private:
  /// @brief Initialise all attributes
  void initParams();
  /// @brief Reset attributes and arrays
  void resetParams();

  /// @brief Get mesh values out of the file on a scalar type
  /// @param [in] _grid typename GridType::ConstPtr - the grid to retrieve
  /// values from
  template <typename GridType>
  void getMeshValuesScalar(typename GridType::ConstPtr _grid);

  // TODO
  /// @brief Get mesh values out of the file on a vector type
  /// @param [in] _grid typename GridType::ConstPtr - the grid to retrieve
  /// values from
  template <typename GridType>
  void getMeshValuesVector(typename GridType::ConstPtr _grid);

  /// @brief Get tree values
  /// @param [in] _grid typename GridType::Ptr - the grid to retrieve values
  /// from
  template <typename GridType>
  void getTreeValues(typename GridType::Ptr _grid);
  /// @brief The file name and path
  std::string m_fileName;
  /// @brief Specifies if the VDB grids have been initialised or not
  bool m_vdbGridsInitialized;
  /// @brief The transform of the VDB
  openvdb::Mat4s m_transform;

  // names of variables stored within the file
  /// @brief Names of variables stored within the file
  std::vector<std::string> m_variableNames;
  // types of the variables stored within the file - mapped to names 1-1
  /// @brief Types of the variables stored within the file - mapped to names 1-1
  std::vector<std::string> m_variableTypes;

  // TODO
  ///// @brief All drawable grids in the file
  // std::vector<VAO> *m_vdbGrids;

  /// @brief The bounding box
  BoundBox *m_bbox;
  /// @brief Vector of extreme values for all channels
  std::vector<BBoxBare> *m_channelExtremes;

  // TODO
  //// Using a 'Crop Box' to limit how much of the model can be seen
  //// Using a Bounding box structure for this as it is exactly what I need
  ///// @brief All crop boxes available
  // BoundBox m_crop[3];
  //// used when scans are run so that the crop box can be returned to its
  //// previous state before it was run
  ///// @brief Shadow array used as storage during crop scans
  // BoundBox m_cropStorage[3];
  ///// @brief VAO for the drawing of the VDB tree
  // VAO *m_vdbTreeVAO;

  /// @brief The current active render channel for points
  int m_currentActiveChannelPoints;
  /// @brief The current active render channel for vectors
  int m_currentActiveChannelVectors;
  /// @brief Channel for loading
  int m_channel;
  /// @brief Total number of channels
  int m_numChannels;

  /// @brief Vector of number of points in each channel
  std::vector<int> m_numPoints;
  /// @brief Vector of all values of s for each channel
  std::vector<float> m_s;
  /// @brief Boolean of whether the file has been opened
  bool m_fileOpened;
  /// @brief Boolean of whether the file has been read
  bool m_fileRead;
  /// @brief Boolean of whether class has been initialised
  bool m_initialised;
  /// @brief Boolean of whether the file has been loaded
  bool m_loaded;
  /// @brief The load percent factor for the file
  float m_loadPercentFactor;
  /// @brief Grid pointer used to access grids
  openvdb::GridPtrVecPtr m_grid;

  /// @brief NUmber of grids used during loading
  int m_numGrids;
  /// @brief Vector of names of the grids
  std::vector<std::string> *m_gridNames;
  /// @brief Vector of the dimensions of the grids
  std::vector<openvdb::Coord> *m_gridDims;
  /// @brief Pointer to all grids
  openvdb::GridCPtrVec m_allG;
  /// @brief Pointer to the metadata found in the file
  openvdb::MetaMap::Ptr m_metadata;
  /// @brief The file version as a string
  std::string m_fileVersion;

  // load functions
  /// @brief Load the bounding box
  bool loadBBox();
  /// @brief Load the Volume Mesh
  bool loadMesh();
  /// @brief Load the VDB tree
  bool loadVDBTree();

  /// @brief Vector of the metadata channels
  std::vector<std::string> m_metaNames;
  /// @brief Vector of the metadata values
  std::vector<std::string> m_metaValues;
  /// @brief Number of metadata channels stored
  int m_numMeta;
  /// @brief Depth of the VDB tree
  int m_treeDepth;
  /// @brief Total number of voxels
  int m_totalVoxels;
  /// @brief Vector of number of voxels at each depth level
  std::vector<int> m_levelCounts;
  /// @brief The number of crop boxes to draw
  int m_numCropsToDraw;
  /// @brief Boolean of whether the extremes have been inited or not
  bool m_extremesInit;

  // stuff for the vectors
  /// @brief The colour of vectors to draw
  openvdb::Vec3f m_vectorColour;
  /// @brief Size of vectors
  float m_vectorSize;

  // TODO
  // information on the graphics card
  /// @brief Total GPU memory in KB
  GLint m_total_GPU_mem_kb;
  ///// @brief Current available GPU memory in KB
  // GLint m_current_available_GPU_mem_kb;
  /// @brief Vector storing the channel value data to upload to a texture buffer
  /// on the GPU
  std::vector<openvdb::Vec4f> *m_channelValueData;
  // TODO
  ///// @brief Texture Buffer ID
  // GLuint m_gridsTBO;
  /// @brief Size of the texture buffer vector
  int m_tboSize;

  /// @brief Push back a VDB tree vert
  void pushBackVDBVert(std::vector<vDat> *_v, openvdb::Vec3f _point,
                       vDat _vert);

  // TODO
  /// @brief Report that the file contains a std::string grid
  void reportStringGridTypeError();
  /// @brief Call appropriate scalar function to get values
  template <typename GridType>
  void callGetValuesGridScalar(typename GridType::Ptr grid);
  /// @brief Call appropriate vector function to get values
  template <typename GridType>
  void callGetValuesGridVector(typename GridType::Ptr grid);
  /// @brief Call appropriate function to get VDB tree values

  // TODO
  template <typename GridType>
  void callGetValuesTree(typename GridType::Ptr grid);

  // TODO
  // inspiration taken from openvdb code examples in OpenVDBCookbook
  // http://www.openvdb.org/documentation/doxygen/codeExamples.html
  /// @brief Process Grid type to call the correct get mesh function
  void processTypedGrid(openvdb::GridBase::Ptr grid);

  /// @brief Process Tree type to call the correct get tree values function
  void processTypedTree(openvdb::GridBase::Ptr grid);
};

#endif /* __VDB_H__ */
