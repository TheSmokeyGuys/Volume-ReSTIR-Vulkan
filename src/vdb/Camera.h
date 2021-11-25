#pragma once

/*
  Copyright (C) 2014 Jon Macey

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

#ifndef __CAMERA_H__
#define __CAMERA_H__

/*
 * This class has been taken from the Camera class of Jon Maceys NGL
 * A few modifications have been made to make it reliable only on GL and OpenVDB
 * libraries http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib
 */

#include <cmath>

#include "Plane.h"

// camera enums used for intersects with frustrum taken from Jon Maceys NGL
// http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib/
/// @enum CAMERAINTERCEPT
enum CAMERAINTERCEPT { OUTSIDE, INTERSECT, INSIDE };

/// @file Camera.h
/// @class Camera
/// @brief simple camera class to allow movement in an opengl scene.
///  a lot of this stuff is from the HILL book Computer Graphics with OpenGL 2nd
///  Ed Prentice Hall a very good book
/// fustrum culling modified from
/// http://www.lighthouse3d.com/opengl/viewfrustum/index.php?defvf
///  @author Jonathan Macey
/// Modified by Callum James to work with pure OpenGL and OpenVDB math libraries
/// as well as removing features and functions not used in this application
///  @version 5.0
///  @date
/// Last Update added fustrum calculation
/// Revision History : \n
/// 27/08/08 Added experimental projection modes
/// 28/09/09 Updated to NCCA Coding standard \n
///  @todo Finish off the different projections modes at present persp and ortho
///  work
class Camera {
public:
  /// @brief Constructor for Camera class
  Camera();
  /// @brief Constructor for Camera class
  /// @param [in] _eye openvdb::Vec3f - position of the camera
  /// @param [in] _look openvdb::Vec3f - vector position for the camera to look
  /// at
  /// @param [in] _up openvdb::Vec3f - up vector for orientation
  Camera(openvdb::Vec3f _eye, openvdb::Vec3f _look, openvdb::Vec3f _up);

  /// @brief Set a default camera
  void setDefaultCamera();

  /// @brief Roll the camera (around Z)
  /// @param [in] _angle float - angle to rotate by
  void roll(float _angle);
  /// @brief Pitch the camera (around X)
  /// @param [in] _angle float - angle to rotate by
  void pitch(float _angle);
  /// @brief Apply yaw to the camera (around Y)
  /// @param [in] _angle float - angle to rotate by
  void yaw(float _angle);
  /// @brief Slide the camera
  /// @param [in] _du float - amount to change in U
  /// @param [in] _dv float - amount to change in V
  /// @param [in] _dn float - amount to change in N
  void slide(float _du, float _dv, float _dn);

  /// @brief Set camera
  /// @param [in] _eye openvdb::Vec3f - position of the camera
  /// @param [in] _look openvdb::Vec3f - vector position for the camera to look
  /// at
  /// @param [in] _up openvdb::Vec3f - up vector for orientation
  void set(const openvdb::Vec3f &_eye, const openvdb::Vec3f &_look,
           const openvdb::Vec3f &_up);
  /// @brief Method to set the camera frustrums shape
  /// @param [in] _viewAngle float - viewing angle for the camera (FOV)
  /// @param [in] _aspect float - viewing aspect ratio of the camera
  /// @param [in] _near float - near clipping plane distance
  /// @param [in] _far float - far clipping plane distance
  void setShape(float _viewAngle, float _aspect, float _near, float _far);

  /// @brief Method to set aspect ratio
  /// @param _asp float - value to set aspect ratio to
  void setAspect(float _asp);

  /// @brief Moves both the camera and eye
  /// @param _dx float - amount to move in x
  /// @param _dy float - amount to move in y
  /// @param _dz float - amount to move in z
  void moveBoth(float _dx, float _dy, float _dz);
  /// @brief Move both camera and eye
  /// @param _move openvdb::Vec3f - vector to move by
  void moveBoth(openvdb::Vec3f _move);
  /// @brief Tranlsate the eye position
  /// @param _dx float - amount to move in x
  /// @param _dy float - amount to move in y
  /// @param _dz float - amount to move in z
  void moveEye(float _dx, float _dy, float _dz);
  /// @brief Tranlsate the look position
  /// @param _dx float - amount to move in x
  /// @param _dy float - amount to move in y
  /// @param _dz float - amount to move in z
  void moveLook(float _dx, float _dy, float _dz);
  /// @brief Set the viewing angle (FOV)
  /// @param [in] _angle float - viewing angle
  void setViewAngle(float _angle);
  /// @brief Update the camera
  void update();

  /// @brief Set the eye position (camera position)
  /// @param _e openvdb::Vec4f - position to set eye to
  void setEye(openvdb::Vec4f _e);
  /// @brief Set the look position
  /// @param _e openvdb::Vec4f - position to set look to
  void setLook(openvdb::Vec4f _e);

  /// @brief Return the view matrix - returns openvdb::Mat4s
  inline openvdb::Mat4s getViewMatrix() { return m_viewMatrix; }
  /// @brief Return the projection matrix - returns openvdb::Mat4s
  inline openvdb::Mat4s getProjectionMatrix() { return m_projectionMatrix; }
  /// @brief Return the view*projection matrix - returns openvdb::Mat4s
  inline openvdb::Mat4s getViewProjectionMatrix() {
    return m_viewMatrix * m_projectionMatrix;
  }
  /// @brief Return the eye position - returns openvdb::Vec4f
  inline openvdb::Vec4f getEye() { return m_eye; }
  /// @brief Return the look position - returns openvdb::Vec4f
  inline openvdb::Vec4f getLook() { return m_look; }
  /// @brief Return U - returns openvdb::Vec4f
  inline openvdb::Vec4f getU() { return m_u; }
  /// @brief Return V - returns openvdb::Vec4f
  inline openvdb::Vec4f getV() { return m_v; }
  /// @brief Return N - returns openvdb::Vec4f
  inline openvdb::Vec4f getN() { return m_n; }
  /// @brief Return the Field of View - returns float
  inline float getFOV() { return m_FOV; }
  /// @brief Return the aspect ratio - returns float
  inline float getAspect() { return m_aspect; }
  /// @brief Return the near clipping distance - returns float
  inline float getNear() { return m_nearPlane; }
  /// @brief Return the Far clipping distance - returns float
  inline float getFar() { return m_farPlane; }

  /// @brief Set far clipping distance
  /// @param [in] _far float - far clipping distance
  inline void setFarPlane(float _far) { m_farPlane = _far; }
  /// @brief Set near clipping distance
  /// @param [in] _near float - near clipping distance
  inline void setNearPlane(float _near) { m_nearPlane = _near; }

  /// @brief Calculate the camera frustrum
  void calculateFrustum();

  /// @brief Calculate if point is in frustrum or not - returns CAMERAINTERCEPT
  /// (Jon Macey NGL http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib/)
  /// @param [in] _p openvdb::Vec3f - point to evaluate
  // CAMERAINTERCEPT from Jon Maceys NGL
  // http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib/
  CAMERAINTERCEPT isPointInFrustum(openvdb::Vec3f &_p);

protected:
  /// @brief Calculate perspective projection
  void setPerspProjection();
  /// @brief Calculate new rotation vectors for the camera after a roll, pitch
  /// or yaw
  /// @param [in,out] io_a the first vector to be rotated
  /// @param [in,out] io_b the second vector to be rotated
  /// @param [in] _angle the angle to rotate
  void rotAxes(openvdb::Vec4f io_a, openvdb::Vec4f io_b, float _angle);

private:
  /// @brief Vector U for local Camera coordinates
  openvdb::Vec4f m_u;
  /// @brief Vector V for local Camera coordinates
  openvdb::Vec4f m_v;
  /// @brief Vector N for local Camera coordinates
  openvdb::Vec4f m_n;
  /// @brief Eye position vector
  openvdb::Vec4f m_eye;
  /// @brief Look position vector
  openvdb::Vec4f m_look;
  /// @brief Up vector
  openvdb::Vec4f m_up;

  /// @brief Width of display images used for perspective projection
  float m_width;
  /// @brief Height of display images used for perspective projection
  float m_height;
  /// @brief Aspect ratio of camera
  float m_aspect;
  /// @brief Near clipping plane distance
  float m_nearPlane;
  /// @brief Far clipping plane distance
  float m_farPlane;
  /// @brief Field of View of camera
  float m_FOV;

  /// @brief Array of 6 plasnes making up the camera frustrum
  Plane m_planes[6];

  // taken from Jon Maceys NGL
  // http://nccastaff.bmth.ac.uk/jmacey/GraphicsLib/
  /// @enum PROJPLANE
  enum PROJPLANE { TOP = 0, BOTTOM, LEFT, RIGHT, NEARP, FARP };
  /// @brief index values for the planes array
  openvdb::Vec3f m_ntl, m_ntr, m_nbl, m_nbr, m_ftl, m_ftr, m_fbl, m_fbr;

  /// @brief Projection matrix of the camera
  openvdb::Mat4s m_projectionMatrix;
  /// @brief View matrix of the camera
  openvdb::Mat4s m_viewMatrix;

  /// @brief Method to set the view matrix
  void setViewMatrix();
  /// @brief Method to set the projection matrix
  void setProjectionMatrix();
};

#endif /* __CAMERA_H__ */
