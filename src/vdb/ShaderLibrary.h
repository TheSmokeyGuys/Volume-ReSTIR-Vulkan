//#pragma once
///*
//  Copyright (C) 2014 Callum James
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//*/
//
//#ifndef __SHADERLIBRARY_H__
//#define __SHADERLIBRARY_H__
//
//#include <openvdb/openvdb.h>
//
//#include <vector>
//
//#include "Shader.h"
//#include "ShaderFamily.h"
//
///// @struct NameID
///// @brief Simple structure to map the shader family names to their IDs
//struct NameID {
//  std::string a_name;
//  unsigned int a_id;
//};
//
///// @file ShaderLibrary.h
///// @brief ShaderLibrary class to store and switch all regsitered and compiled
///// shaders
///// @author Callum James
///// @version 1.0
///// @date 12/02/2014
///// Revision History:
///// Initial Version 05/01/2014
///// @class ShaderLibrary
///// @brief ShaderLibrary class to store all compiled and linked shaders and
///// handle access to them for the setting of attributes and uniforms. Also
///// allows the switching of the currently actively shader
//class ShaderLibrary {
//public:
//  /// @brief Constructor for ShaderLibrary class
//  ShaderLibrary();
//  /// @brief Destructor for ShaderLibrary class
//  ~ShaderLibrary();
//
//  // create a new shader family
//  /// @brief Method to create a shader family
//  /// @param [in] _name std::string - name of new family
//  void createShaderFamily(std::string _name);
//  // create a new shader family and attach a vert and frag shader named
//  // correctly
//  /// @brief Method to create a family with an autromatic vertex and fragment
//  /// shader
//  /// @param [in] _name std::string - name of family to create
//  void createVertFragShaderFamily(std::string _name);
//  // create a Vertex and a fragment shader for a basic defualt shader
//  /// @brief Method to create a vertex and fragment shader
//  /// @param [in] _name std::string - family to attach create for
//  void createVertFragShader(std::string _name);
//  // create a new shader
//  /// @brief Create a shader for a specified family
//  /// @param [in] _name std::string - name of family to create for
//  /// @param [in] _type SHADERTYPE - type of shader to create
//  void createShader(std::string _name, SHADERTYPE _type);
//  // add, load, compile and link shader to family
//  /// @brief Add, load, compile and link a shader to a family
//  /// @param [in] _familyName std::string - name of family to add to
//  /// @param [in] _type SHADERTYPE - type of shader to create
//  /// @param [in] _file std::string - file of shader
//  void addShader(std::string _familyName, SHADERTYPE _type, std::string _file);
//  // add, load, compile and link shader to family
//  /// @brief Add, load, compile and link a shader to a family
//  /// @param [in] _familyName std::string - name of family to add to
//  /// @param [in] _type SHADERTYPE - type of shader to create
//  /// @param [in] _string char* - source of shader
//  void addShader(std::string _familyName, SHADERTYPE _type, char *_string);
//  // link a shader up to a family
//  /// @brief Link shader to family
//  /// @param [in] _shader std::string - shader to link
//  /// @param [in] _family std::string - family to link to
//  void linkShaderToFamily(std::string _shader, std::string _family);
//
//  // get the ID of a certain family
//  /// @brief Get ID of a family looked up by name - returns GLuint
//  /// @param [in] _name std::string - name of family to get ID of
//  GLuint getID(std::string _name);
//
//  // compile all currently loaded shader families
//  /// @brief Compile all families
//  void compileAllFamiles();
//
//  // compile all shaders
//  /// @brief Compile all shaders
//  void compileAll();
//
//  // compile passed in named shader
//  /// @brief Compile a shader found by name lookup
//  /// @param [in] _name std::string - name of shadert to compile
//  void compileShader(std::string _name);
//
//  // link all sahder families
//  /// @brief Link all families
//  void linkAll();
//
//  // link passed in names shader
//  /// @brief Link shader specified
//  /// @param [in] _name std::string - shader to link
//  void linkShader(std::string _name);
//
//  // activate one shader family to use for now
//  /// @brief Set specified shader as active
//  /// @param [in] _name std::string - shader to set active
//  void setActive(std::string _name);
//
//  /// @brief Get the number of shaders - return int
//  inline unsigned int getNumShaders() { return m_shaders.size(); }
//
//  /// @brief Return the specified family from name - returns ShaderFamily*
//  /// @param [in] _name std::string - family to retrieve
//  ShaderFamily *getFamily(std::string _name);
//
//  // return location to an attribute location in a shader in the library
//  /// @brief Retrieve attribute location - returns GLint
//  /// @param [in] _shaderName std::string - shader
//  /// @param [in] _paramName std::string - name of attribute to find
//  GLint getAttribLocation(std::string _shaderName, std::string _paramName);
//  // return location of a uniform in a shader in the library
//  /// @brief Retrieve uniform location - returns GLint
//  /// @param [in] _shaderName std::string - shader
//  /// @param [in] _uniformName std::string - name of uniform to find
//  GLint getUniformLocation(std::string _shaderName, std::string _uniformName);
//  // load vert and frag files for one shader
//  /// @brief Load a vertex and fragment shader from file
//  /// @param [in] _shaderName std::string - name of the shader
//  /// @param [in] _vertex std::string - vertex shader file path
//  /// @param [in] _frag std::string - fragment shader file path
//  void loadVertFragShaderFile(std::string _shaderName, std::string _vertex,
//                              std::string _frag);
//  // load a shader from source file
//  /// @brief Load shader from file
//  /// @param [in] _shaderName std::string - name of shader
//  /// @param [in] _sourceFile std::string - source file
//  void loadShaderFile(std::string _shaderName, std::string _sourceFile);
//  // load a shader from source string
//  /// @brief Load shader from source
//  /// @param [in] _shaderName std::string - name of shader
//  /// @param [in] _string char* - source
//  void loadShaderSource(std::string &_shaderName, char *_string);
//  // load vert and frag source for one shader
//  /// @brief Load a vertex and fragment shader from source
//  /// @param [in] _shaderName std::string - name of the shader
//  /// @param [in] _vertex char* - vertex shader source
//  /// @param [in] _frag chjar* - fragment shader source
//  void loadVertFragShaderSource(std::string _shaderName, char *_vertex,
//                                char *_frag);
//
//  // remove all shaders
//  /// @brief Remove all shaders
//  void clean();
//
//  // functions to upload uniforms and parameters
//  /// @brief Upload Matrix 3x3 uniform to shaders
//  /// @param [in] _uniformName const std::string - name of uniform
//  /// @param [in] _m openvdb::Mat3R - matrix to upload
//  void setShaderUniformFromMat3(const std::string &_uniformName,
//                                openvdb::Mat3R _m);
//  /// @brief Upload Matrix 4x4 uniform to shaders
//  /// @param [in] _uniformName const std::string - name of uniform
//  /// @param [in] _m openvdb::Mat4s - matrix to upload
//  void setShaderUniformFromMat4(const std::string &_uniformName,
//                                openvdb::Mat4s _m);
//  /// @brief Upload single float uniform to shaders
//  /// @param [in] _paramName const std::string - name of uniform
//  /// @param [in] _f float - float to upload
//  void setShaderParam1f(const std::string &_paramName, float _f);
//  /// @brief Upload single int uniform to shaders
//  /// @param [in] _paramName const std::string - name of uniform
//  /// @param [in] _i float - int to upload
//  void setShaderParam1i(const std::string &_paramName, int _i);
//
//  /// @brief Upload array of 3 float uniforms to shaders
//  /// @param [in] _paramName const std::string - name of uniform
//  /// @param [in] _f1 float - float to upload
//  /// @param [in] _f2 float - float to upload
//  /// @param [in] _f3 float - float to upload
//  void setShaderParam3f(const std::string &_paramName, float _f1, float _f2,
//                        float _f3);
//  /// @brief Upload array of 4 float uniforms to shaders
//  /// @param [in] _paramName const std::string - name of uniform
//  /// @param [in] _f1 float - float to upload
//  /// @param [in] _f2 float - float to upload
//  /// @param [in] _f3 float - float to upload
//  /// @param [in] _f4 float - float to upload
//  void setShaderParam4f(const std::string &_paramName, float _f1, float _f2,
//                        float _f3, float _f4);
//  /// @brief Upload array of 4 int uniforms to shaders
//  /// @param [in] _paramName const std::string - name of uniform
//  /// @param [in] _i1 int - int to upload
//  /// @param [in] _i2 int - int to upload
//  /// @param [in] _i3 int - int to upload
//  /// @param [in] _i4 int - int to upload
//  void setShaderParam4i(const std::string &_paramName, int _i1, int _i2,
//                        int _i3, int _i4);
//
//private:
//  // map of all shader families stored in the library
//  /// @brief All shader families stored in this library
//  std::vector<ShaderFamily *> m_shaderFamilies;
//  // map of all shaders within the library
//  /// @brief All shaders stored in this library
//  std::vector<Shader *> m_shaders;
//
//  /// @brief Names and ID of all shaders
//  std::vector<NameID> m_shaderMap;
//  /// @brief Names and ID of all families
//  std::vector<NameID> m_familyMap;
//
//  /// @brief Null family to return if null needed
//  ShaderFamily *m_nullFamily;
//
//  /// @brief Method to find a shaders ID - return int
//  /// @param [in] _name std::string - shader to find
//  int findShader(std::string _name);
//  /// @brief Method to find a shader family's ID - return int
//  /// @param [in] _name std::string - family to find
//  int findShaderFamily(std::string _name);
//
//  // name of the active shader
//  /// @brief Name of the currently active shader
//  std::string m_activeShader;
//
//  /// @brief Method to return a suffix from the shader type to keep internal
//  /// naming conventions - returns std::string
//  /// @param [in] _type SHADERTYPE - type of shader being passed in
//  std::string suffixFromType(SHADERTYPE _type);
//};
//
//#endif /* __SHADERLIBRARY_H__ */
