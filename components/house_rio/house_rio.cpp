#include "house_rio.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include <cmath>
#include <cstdio>
#include <cstring>
#include <memory>

namespace esphome { namespace house_rio {
static const char *const TAG = "house_rio";
using Json = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;

namespace {
struct ResponseBuffer { std::string *body; bool too_large{false}; };
esp_err_t on_http(esp_http_client_event_t *event) {
  auto *buffer = static_cast<ResponseBuffer *>(event->user_data);
  if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
    if (buffer->body->size() + size_t(event->data_len) > 65536) {
      buffer->too_large = true;
      return ESP_FAIL;
    }
    buffer->body->append(static_cast<const char *>(event->data), event->data_len);
  }
  return ESP_OK;
}
const cJSON *field(const cJSON *root, const char *key) {
  return cJSON_GetObjectItemCaseSensitive(root, key);
}
bool is_string(const cJSON *root, const char *key, const std::string &expected) {
  const auto *item = field(root, key);
  return cJSON_IsString(item) && expected == item->valuestring;
}
bool safe_id(const char *id) {
  if (!id || !*id || std::strlen(id) > 96) return false;
  for (const char *s = id; *s; ++s)
    if (!((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z') ||
          (*s >= '0' && *s <= '9') || *s == '_' || *s == '-'))
      return false;
  return true;
}
std::string discover_sub(const cJSON *root, const std::string &renderer_id) {
  if (!cJSON_IsObject(root) || !is_string(root, "service", "house-audio-server")) return {};
  const auto *subs = field(root, "subwoofers");
  if (!cJSON_IsArray(subs)) return {};
  std::string found;
  for (const auto *sub = subs->child; sub; sub = sub->next) {
    if (!is_string(sub, "rendererId", renderer_id)) continue;
    const auto *id = field(sub, "id");
    if (!found.empty() || !cJSON_IsString(id) || !safe_id(id->valuestring)) return {};
    found = id->valuestring;
  }
  return found;
}
bool parse_demand(const cJSON *root, const std::string &id, const std::string &renderer_id,
                  bool &needed) {
  if (!cJSON_IsObject(root) || !is_string(root, "service", "house-audio-server") ||
      !is_string(root, "id", id) || !is_string(root, "rendererId", renderer_id))
    return false;
  const auto *known = field(root, "known"), *demand = field(root, "needed");
  const auto *ttl = field(root, "validForSeconds"), *delay = field(root, "standbyDelaySeconds");
  if (!cJSON_IsTrue(known) || !cJSON_IsBool(demand) || !cJSON_IsNumber(ttl) ||
      !std::isfinite(ttl->valuedouble) || ttl->valuedouble <= 0 || ttl->valuedouble > 10 ||
      !cJSON_IsNumber(delay) || delay->valuedouble != 30)
    return false;
  needed = cJSON_IsTrue(demand);
  return true;
}
}  // namespace

void HouseRio::setup() {
  if (dsp_ == nullptr) { mark_failed(); return; }
  // Boot silently until the Pi returns authoritative demand. The amplifier
  // remains powered; only the post-crossover PCM samples are muted.
  dsp_->set_output_muted(true);
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) {
    ESP_LOGE(TAG, "Unable to read Snapcast Wi-Fi identity");
    mark_failed(); return;
  }
  char id[18];
  std::snprintf(id, sizeof(id), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  renderer_id_ = id;
  if (xTaskCreate(worker_entry_, "rio_pi", 8192, this, 1, &worker_handle_) != pdPASS) {
    ESP_LOGE(TAG, "Unable to start Rio Pi demand worker");
    mark_failed();
  }
}

void HouseRio::dump_config() {
  ESP_LOGCONFIG(TAG, "Rio audio-follow (DSP mute only), Pi %s, renderer %s",
                server_url_.c_str(), renderer_id_.c_str());
  ESP_LOGCONFIG(TAG, "External amplifier remains powered; no physical standby control");
}

void HouseRio::loop() {
  const bool muted = audio_muted_.load(std::memory_order_acquire);
  if (muted != last_applied_muted_) {
    dsp_->set_output_muted(muted);
    last_applied_muted_ = muted;
    ESP_LOGI(TAG, "%s (TPA3118 remains powered)", muted ? "DSP audio muted" : "DSP audio enabled");
  }
  if (!status_sensor_) return;
  const bool have_demand = have_demand_.load(std::memory_order_acquire);
  const bool pi_available = pi_available_.load(std::memory_order_acquire);
  const std::string display = !have_demand ? "Waiting for Pi demand - audio muted" :
      !pi_available ? (muted ? "Pi unavailable - holding audio mute" : "Pi unavailable - holding audio") :
      muted ? "Audio muted - amplifier powered" : "Audio active - amplifier powered";
  if (display != last_display_) {
    status_sensor_->publish_state(display);
    last_display_ = display;
  }
}

int HouseRio::request_(const std::string &path, const std::string *body, std::string &response) {
  response.clear();
  ResponseBuffer buffer{&response};
  const std::string url = server_url_ + path;
  esp_http_client_config_t config{};
  config.url = url.c_str();
  config.timeout_ms = 900;
  config.event_handler = on_http;
  config.user_data = &buffer;
  config.disable_auto_redirect = true;
  auto *client = esp_http_client_init(&config);
  if (!client) return -1;
  if (body) {
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body->data(), body->size());
  }
  const esp_err_t error = esp_http_client_perform(client);
  const int status = error == ESP_OK && !buffer.too_large ?
                     esp_http_client_get_status_code(client) : -1;
  esp_http_client_cleanup(client);
  return status;
}

void HouseRio::worker_() {
  std::string sub_id, response, last_report;
  uint32_t last_poll = millis() - 2000, last_post = millis() - 4000;
  while (true) {
    const uint32_t now = millis();
    wifi_ap_record_t ap{};
    const bool connected = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;

    if (static_cast<uint32_t>(now - last_poll) >= 2000) {
      last_poll = now;
      bool known = false, needed = false;
      if (connected && sub_id.empty() &&
          request_("/api/subwoofers", nullptr, response) == 200) {
        Json root(cJSON_Parse(response.c_str()), cJSON_Delete);
        sub_id = discover_sub(root.get(), renderer_id_);
      }
      if (connected && !sub_id.empty()) {
        const int result = request_("/api/subwoofers/" + sub_id + "/demand", nullptr, response);
        if (result == 200) {
          Json root(cJSON_Parse(response.c_str()), cJSON_Delete);
          known = parse_demand(root.get(), sub_id, renderer_id_, needed);
        } else if (result == 404) sub_id.clear();
      }
      pi_available_.store(known, std::memory_order_release);
      if (known) {
        have_demand_.store(true, std::memory_order_release);
        audio_muted_.store(!needed, std::memory_order_release);
      }
      // Unknown/outage means HOLD audio state, not assume all parents departed.
    }

    const bool have_demand = have_demand_.load(std::memory_order_acquire);
    const bool pi_available = pi_available_.load(std::memory_order_acquire);
    const bool muted = audio_muted_.load(std::memory_order_acquire);
    const char *state = !have_demand ? "unknown" : muted ? "audio_muted" : "audio_active";
    const char *detail = !have_demand ? "Awaiting Pi demand; DSP muted; amplifier powered" :
        !pi_available ? "Pi unavailable; holding DSP audio state; amplifier powered" :
        muted ? "DSP audio muted; amplifier remains powered" :
                "DSP audio active; amplifier remains powered";
    const std::string signature = std::string(state) + "|" + detail;
    if (connected && !sub_id.empty() &&
        (signature != last_report || static_cast<uint32_t>(millis() - last_post) >= 4000)) {
      Json json(cJSON_CreateObject(), cJSON_Delete);
      if (json) {
        cJSON_AddStringToObject(json.get(), "rendererId", renderer_id_.c_str());
        cJSON_AddStringToObject(json.get(), "mode", "auto");
        cJSON_AddStringToObject(json.get(), "state", state);
        cJSON_AddBoolToObject(json.get(), "acknowledged", false);
        cJSON_AddStringToObject(json.get(), "detail", detail);
        char *encoded = cJSON_PrintUnformatted(json.get());
        if (encoded) {
          const std::string body(encoded);
          cJSON_free(encoded);
          const int result = request_("/api/subwoofers/" + sub_id + "/status", &body, response);
          if (result == 200) last_report = signature;
          else if (result == 404) sub_id.clear();
        }
      }
      last_post = millis();  // Back off on failures; no hot retry loop.
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

}}  // namespace esphome::house_rio
