#ifndef __VOLUME_RESTIR_UTILS_LOGGING_HPP__
#define __VOLUME_RESTIR_UTILS_LOGGING_HPP__

/**
 * @file logging.hpp
 * @author Zhihao Ruan (ruanzh@seas.upenn.edu)
 *
 * @brief Some hacks for spdlog to print custom type objects.
 *
 *  See reference:
 *   - https://fmt.dev/latest/api.html#format-api
 *
 * @date 2021-11-23
 */

#include "glm/glm.hpp"
#include "spdlog/fmt/bundled/format.h"
#include "spdlog/spdlog.h"

template <>
struct fmt::formatter<glm::vec4> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(const glm::vec4& p, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(), "[{:7.3f}, {:7.3f}, {:7.3f}, {:7.3f}]", p[0],
                     p[1], p[2], p[3]);
  }
};

template <>
struct fmt::formatter<glm::vec3> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(const glm::vec3& p, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(), "[{:7.3f}, {:7.3f}, {:7.3f}]", p[0], p[1],
                     p[2]);
  }
};

template <>
struct fmt::formatter<glm::mat4> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(const glm::mat4& p, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(ctx.out(),
                     "\n\t[{:7.3f}, {:7.3f}, {:7.3f}, {:7.3f}\n\t {:7.3f}, "
                     "{:7.3f}, {:7.3f}, {:7.3f}\n\t {:7.3f}, {:7.3f}, {:7.3f}, "
                     "{:7.3f}\n\t {:7.3f}, {:7.3f}, {:7.3f}, {:7.3f}]",
                     p[0][0], p[0][1], p[0][2], p[0][3], p[1][0], p[1][1],
                     p[1][2], p[1][3], p[2][0], p[2][1], p[2][2], p[2][3],
                     p[3][0], p[3][1], p[3][2], p[3][3]);
  }
};

template <>
struct fmt::formatter<glm::mat3> {
  constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
    return ctx.end();
  }

  template <typename FormatContext>
  auto format(const glm::mat3& p, FormatContext& ctx) -> decltype(ctx.out()) {
    // ctx.out() is an output iterator to write to.
    return format_to(
        ctx.out(),
        "\n\t[{:7.3f}, {:7.3f}, {:7.3f}\n\t {:7.3f}, {:7.3f}, {:7.3f}\n\t "
        "{:7.3f}, {:7.3f}, {:7.3f}\n\t {:7.3f}, {:7.3f}, {:7.3f}]",
        p[0][0], p[0][1], p[0][2], p[1][0], p[1][1], p[1][2], p[2][0], p[2][1],
        p[2][2]);
  }
};

#endif /* __VOLUME_RESTIR_UTILS_LOGGING_HPP__ */
