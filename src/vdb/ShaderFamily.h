//#pragma once
//
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
//#ifndef __SHADERFAMILY_H__
//#define __SHADERFAMILY_H__
//
//#include <vector>
//
//#include "Shader.h"
//
//// use of the word family in this class is used to denote a collection of
//// it is a direct rebranding of program used in GL and will mean the same
/// thing
//
///// @file ShaderFamily.h
///// @brief ShaderFamily class represents one entire shader program and all
///// contained shaders
///// @author Callum James
///// @version 1.0
///// @date 12/02/2014
///// Revision History:
///// Initial Version 05/01/2014
///// @class ShaderFamily
///// @brief ShaderFamily class handles one shader program. It controls the
///// linking and setting of shader attributes and uniforms within the family
/// and
///// allows an easy encapsulation for the creation of shaders as well as using
///// them throughout the application
// class ShaderFamily {
// public:
//  /// @brief Constructor for the ShaderFamily class
//  ShaderFamily();
//  /// @brief Constructor for the ShaderFamily class
//  /// @param [in] _name std::string - name of the shader family
//  ShaderFamily(std::string _name);
//  /// @brief Destructor of the ShaderFamily class
//  ~ShaderFamily();
//
//  // bind this family as the active GL program being used
//  /// @brief Bind this shader family as the active shader family
//  void bind();
//
//  // unbind this family to allow another GL program to take control
//  /// @brief Unbind the shader family
//  void unbind();
//
//  // attach a shader to this family, to be used
//  /// @brief Attach a shader to this family
//  /// @param [in] _shader Shader* - the shader to attach
//  void attachShader(Shader* _shader);
//
//  // bind an attribute to the attached shaders in this family
//  /// @brief Method to bind an attributCallum Jamee to the shader family
//  /// @param [in] _index GLuint - index of the shader to bind
//  /// @param [in] _attribName const std::string - attribute name
//  void bindAttrib(GLuint _index, const std::string& _attribName);
//
//  // run through the attached shaders and compile any ones that have not been
//  // compiled
//  /// @brief Compile all attached shaders
//  void compile();
//
//  // link the family up
//  /// @brief Link this shader family
//  void link();
//
//  // return the id for this family
//  /// @brief get the ID of this ShaderFamily - returns GLuint
//  inline GLuint getID() const { return m_familyID; }
//
//  // returns the location of the attribute with the passed in name if it
//  exists
//  /// @brief Method to get a uniform location within the attached shaders -
//  /// returns GLuint
//  /// @param [in] _name const char* - the name of the uniform to find
//  GLuint getUniformLocation(const char* _name);
//
//  // function to allow manual enabling of the vertex attrib array with the
//  // passed in id
//  /// @brief Method to enable a Vertex Attribute Array
//  /// @param [in] _loc GLuint - location to enable
//  void enableVertAttribArray(GLuint _loc);
//
//  // function to allow manual disabling of the vertex attrib array with the
//  // passed in id
//  /// @brief Method to disable a Vertex Attribute Array
//  /// @param [in] _loc GLuint - location to disable
//  void disableVertAttribArray(GLuint _loc);
//
//  // collection of functions to set Uniforms within the shaders
//  /// @brief Set a single float uniform on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _f float - value to set
//  void setUniform1f(const char* _var, float _f);
//  /// @brief Set 2 float uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _f0 float - value to set
//  /// @param [in] _f1 float - value to set
//  void setUniform2f(const char* _var, float _f0, float _f1);
//  /// @brief Set 3 float uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _f0 float - value to set
//  /// @param [in] _f1 float - value to set
//  /// @param [in] _f2 float - value to set
//  void setUniform3f(const char* _var, float _f0, float _f1, float _f2);
//  /// @brief Set 4 float uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _f0 float - value to set
//  /// @param [in] _f1 float - value to set
//  /// @param [in] _f2 float - value to set
//  /// @param [in] _f3 float - value to set
//  void setUniform4f(const char* _var, float _f0, float _f1, float _f2,
//                    float _f3);
//  /// @brief Set 1D float array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _f const float* - value to upload
//  void setUniform1fv(const char* _var, size_t _count, const float* _f);
//  /// @brief Set 2D float array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _f const float* - value to upload
//  void setUniform2fv(const char* _var, size_t _count, const float* _f);
//  /// @brief Set 3D float array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _f const float* - value to upload
//  void setUniform3fv(const char* _var, size_t _count, const float* _f);
//  /// @brief Set 4D float array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _f const float* - value to upload
//  void setUniform4fv(const char* _var, size_t _count, const float* _f);
//  /// @brief Set a single integer uniform on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _i int - value to set
//  void setUniform1i(const char* _var, GLint _i);
//  /// @brief Set 2 integer uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _i0 int - value to set
//  /// @param [in] _i1 int - value to set
//  void setUniform2i(const char* _var, GLint _i0, GLint _i1);
//  /// @brief Set 3 integer uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _i0 int - value to set
//  /// @param [in] _i1 int - value to set
//  /// @param [in] _i2 int - value to set
//  void setUniform3i(const char* _var, GLint _i0, GLint _i1, GLint _i2);
//  /// @brief Set 4 integer uniforms on a shader
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _i0 int - value to set
//  /// @param [in] _i1 int - value to set
//  /// @param [in] _i2 int - value to set
//  /// @param [in] _i3 int - value to set
//  void setUniform4i(const char* _var, GLint _i0, GLint _i1, GLint _i2,
//                    GLint _i3);
//  /// @brief Set 1D int array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _i const GLint* - value to upload
//  void setUniform1iv(const char* _var, size_t _count, const GLint* _i);
//  /// @brief Set 2D int array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _i const GLint* - value to upload
//  void setUniform2iv(const char* _var, size_t _count, const GLint* _i);
//  /// @brief Set 3D int array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _i const GLint* - value to upload
//  void setUniform3iv(const char* _var, size_t _count, const GLint* _i);
//  /// @brief Set 4D int array
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - elements in array
//  /// @param [in] _i const GLint* - value to upload
//  void setUniform4iv(const char* _var, size_t _count, const GLint* _i);
//  /// @brief Upload a 2x2 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix2fv(const char* _var, size_t _count, bool _transpose,
//                           const float* _m);
//  /// @brief Upload a 3x3 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix3fv(const char* _var, size_t _count, bool _transpose,
//                           const float* _m);
//  /// @brief Upload a 4x4 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix4fv(const char* _var, size_t _count, bool _transpose,
//                           const float* _m);
//  /// @brief Upload a 2x3 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix2x3fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Upload a 2x4 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix2x4fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Upload a 3x2 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix3x2fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Upload a 3x4 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix3x4fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Upload a 4x2 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix4x2fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Upload a 4x3 matrix to the shaders
//  /// @param [in] _var const char* - uniform name
//  /// @param [in] _count size_t - size of data
//  /// @param [in] _transpose bool - transposed matrix
//  /// @param [in] _m const float* - data to upload
//  void setUniformMatrix4x3fv(const char* _var, size_t _count, bool _transpose,
//                             const float* _m);
//  /// @brief Get value of specified uniform
//  /// @param [in] _name const char* - name of uniform to find
//  /// @param [in,out] o_f float* - return value
//  void getUniformfv(const char* _name, float* o_f);
//  /// @brief Get value of specified uniform
//  /// @param [in] _name const char* - name of uniform to find
//  /// @param [in,out] o_i int* - return value
//  void getUniformiv(const char* _name, int* o_i);
//
// private:
//  // list of all shaders belonging to this familt
//  /// @brief Vector of all attached shaders
//  std::vector<Shader*> m_shaders;
//
//  // GL family ID for this object
//  /// @brief The ID of this ShaderFamily
//  GLuint m_familyID;
//
//  // name of the family to use in look up
//  /// @brief Name of this family used for lookups
//  std::string m_familyName;
//
//  // boolean to signify if this family is currently active or not
//  /// @brief Boolean of whether the family is active or not
//  bool m_active;
//
//  // boolean to signify if the family (program) has yet been linked
//  /// @brief Boolean of whether or not the family has been linked
//  bool m_link;
//
//  // following function validates the program
//  /// @brief Method to validate this family - returns true or false
//  /// @param [in] program GLuint - the family to validate
//  static bool validateProgram(GLuint program);
//};
//
//#endif /* __SHADERFAMILY_H__ */
