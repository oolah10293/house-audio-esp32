#pragma once
#include "cJSON.h"
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

namespace esphome { namespace house_bose {
struct Demand { bool known{false}, needed{false}; uint32_t received_ms{0}, valid_ms{10000}; };
inline const cJSON *field(const cJSON *root, const char *key) { return cJSON_GetObjectItemCaseSensitive(root, key); }
inline bool equals(const cJSON *root, const char *key, const std::string &value) {
  auto *v = field(root, key); return cJSON_IsString(v) && value == v->valuestring;
}
inline bool safe_id(const char *s) {
  if (!s || !*s || std::strlen(s) > 96) return false;
  for (; *s; ++s) if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') || (*s >= '0' && *s <= '9') || *s == '-' || *s == '_')) return false;
  return true;
}
inline std::string discover_sub(const cJSON *root, const std::string &renderer) {
  if (!cJSON_IsObject(root) || !equals(root, "service", "house-audio-server")) return {};
  const auto *subs = field(root, "subwoofers");
  if (!cJSON_IsArray(subs)) return {};
  std::string found;
  for (auto *sub = subs->child; sub; sub = sub->next) {
    if (!equals(sub, "rendererId", renderer)) continue;
    const auto *id = field(sub, "id");
    if (!found.empty() || !cJSON_IsString(id) || !safe_id(id->valuestring)) return {};
    found = id->valuestring;
  }
  return found;
}
inline Demand parse_demand(const cJSON *root, const std::string &sub, const std::string &renderer, uint32_t now) {
  Demand result; result.received_ms = now;
  if (!cJSON_IsObject(root) || !equals(root, "service", "house-audio-server") ||
      !equals(root, "id", sub) || !equals(root, "rendererId", renderer)) return result;
  const auto *known = field(root, "known"), *needed = field(root, "needed");
  const auto *ttl = field(root, "validForSeconds"), *delay = field(root, "standbyDelaySeconds");
  if (!cJSON_IsTrue(known) || !cJSON_IsBool(needed) || !cJSON_IsNumber(ttl) ||
      !std::isfinite(ttl->valuedouble) || ttl->valuedouble <= 0 || ttl->valuedouble > 10 ||
      !cJSON_IsNumber(delay) || delay->valuedouble != 30) return result;
  result.known = true; result.needed = cJSON_IsTrue(needed);
  result.valid_ms = uint32_t(ttl->valuedouble * 1000);
  if (!result.valid_ms) result.known = false;
  return result;
}
}}  // namespace
