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

#ifndef __SHADER_H__
#define __SHADER_H__

#ifdef DARWIN
#include <OpenGL/gl3.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glew.h>
#endif

#include <cstdlib>
#include <string>

// These enum definitions have been taken from Jon Maceys NGL Shader.h
// http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib/
/// @enum SHADERTYPE
enum SHADERTYPE { VERTEX, FRAGMENT, GEOMETRY, TESSCONTROL, TESSEVAL };

/// @file Shader.h
/// @brief Class to handle one shader component and handle its compilation and
/// loading
/// @author Callum James
/// @version 1.0
/// @date 12/02/2014
/// Revision History:
/// Initial Version 05/01/2014
/// @class Shader
/// @brief Shader class to handle a single shader component as part of a larger
/// shader family. Controls its compilation and loading and also the type of
/// shader that it is
class Shader {
public:
  /// @brief Constructor of Shader class
  Shader();
  /// @brief Constructor of Shader class
  /// @param [in] _name std::string - name of the shader
  /// @param [in] _type SHADERTYPE - type of the shader
  Shader(std::string _name, SHADERTYPE _type);

  /// @brief Destructor of the Shader class
  ~Shader();

  // compile the current shader
  /// @brief Compile the current shader
  void compile();

  // return if compiled or not
  /// @brief Return if the shader has been compiled or not - return true or
  /// false
  inline bool compiled() { return m_compiled; }

  // load the shader from file name
  /// @brief Load shader from a file
  /// @param [in] _fileName std::string - path to file
  void loadFromFile(std::string _fileName);
  // load the shader from shader contents - read in from a file previous
  // this will not set the file path attribute
  /// @brief Load shader from source
  /// @param [in] _shaderContents char* - Total shader source
  void loadFromSource(const char *_shaderContents);

  // return the handle id of the shader
  /// @brief Get the shader handle - returns GLuint
  inline GLuint getShaderHandle() const { return m_shaderHandle; }
  // return the source of the shader as a single string
  /// @brief Get the shader source - returns std::string
  inline const std::string getShaderSource() const { return m_shaderSource; }
  // if set return the shader file path, else return __file_path_not_set__
  /// @brief Get the shader file path - returns std::string
  inline const std::string getShaderFilePath() const { return m_shaderFile; }

private:
  // name of the shader
  /// @brief The shader name
  std::string m_shaderName;
  // file path to shader file
  /// @brief Path to the shader file
  std::string m_shaderFile;
  // source of the shader used for loading
  /// @brief Full shader source
  std::string m_shaderSource;

  // what type of shader this shader is
  /// @brief Shader type of this shader
  SHADERTYPE m_shaderType;
  // GL handle of the shader
  /// @brief The shader handle for this shader
  GLuint m_shaderHandle;

  // simple boolean flag to indicate whether the shader has been compiled or not
  /// @brief Boolean of if the shader is compiled or not
  bool m_compiled;

  // following two functions are utility functions designed to check and load
  // the shaders from source they will also check to see if it has compiled
  // correctly read in shader from source
  /// @brief Method to extract the text from a file - returns static char*
  /// @param [in] fileName const char* - the file path and name
  static char *textFileRead(const char *fileName);
  // validate the shader once compiled
  /// @brief Method to validate the shader read in - returns static bool
  /// @param [in] shader GLuint - the shader handle
  /// @param [in] _name std::string - the name of the shader
  static bool validateShader(GLuint shader, std::string _name);
};

#endif /* __SHADER_H__ */
