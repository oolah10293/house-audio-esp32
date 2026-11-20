#include "house_bose.h"
#include "esphome/core/log.h"
#include "esp_http_client.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include <cstdio>
#include <memory>

namespace esphome { namespace house_bose {
static const char *const TAG = "house_bose";
using Json = std::unique_ptr<cJSON, decltype(&cJSON_Delete)>;

void HouseBose::setup() {
  tx_pin_->setup();
  tx_pin_->digital_write(false);  // inverted open-drain: false releases bus
  uint8_t mac[6];
  if (esp_read_mac(mac, ESP_MAC_WIFI_STA) != ESP_OK) { mark_failed(); return; }
  char id[18];
  std::snprintf(id, sizeof(id), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  renderer_id_ = id;  // Exact pinned Snapclient hello.id format, not a name/IP guess.
  demands_ = xQueueCreate(1, sizeof(Demand));
  reports_ = xQueueCreate(1, sizeof(Report));
  if (!demands_ || !reports_ || xTaskCreate(worker_entry_, "bose_pi", 8192, this, 1, &worker_handle_) != pdPASS) {
    ESP_LOGE(TAG, "Unable to start Pi feedback worker");
    if (demands_) vQueueDelete(demands_);
    if (reports_) vQueueDelete(reports_);
    demands_ = nullptr; reports_ = nullptr;
    mark_failed(); return;
  }
  controller_.begin(millis());
}

void HouseBose::dump_config() {
  ESP_LOGCONFIG(TAG, "Bose auto standby v0.1.0; Pi %s; renderer %s", server_url_.c_str(), renderer_id_.c_str());
  ESP_LOGCONFIG(TAG, "30s grace; command echo + 00 ACK; 3 attempts; networking in separate task");
}

uint32_t HouseBose::transmit(const uint8_t *frame, size_t size) {
  uint8_t old;
  while (available()) if (!read_byte(&old)) break;  // Discard unrelated/stale replies.
  tx_pin_->digital_write(false);
  delayMicroseconds(208);
  for (size_t i = 0; i < size; ++i) {
    tx_pin_->digital_write(true); delayMicroseconds(208);
    for (unsigned bit = 0; bit < 8; ++bit) {
      tx_pin_->digital_write((frame[i] & (1U << bit)) == 0);
      delayMicroseconds(208);
    }
    tx_pin_->digital_write(false); delayMicroseconds(208);
  }
  const uint32_t finished = millis();
  ESP_LOGD(TAG, "TX %02X %02X %02X %02X (%u bytes)", frame[0], frame[1], frame[2], frame[3], unsigned(size));
  return finished;
}

void HouseBose::loop() {
  const uint32_t now = millis();
  Demand demand;
  if (xQueueReceive(demands_, &demand, 0) == pdTRUE)
    controller_.demand(demand.known, demand.needed, demand.received_ms, demand.valid_ms);
  uint8_t byte;
  unsigned count = 0;
  while (count++ < 64 && available() && read_byte(&byte)) controller_.receive(byte, now);
  controller_.tick(now);
  const Report report = controller_.report(now);
  std::string display;
  if (report.state == State::STANDBY_PENDING) display = "Standby in " + std::to_string(report.remaining_seconds) + " seconds";
  else if (report.state == State::STANDBY && report.acknowledged) display = "Standby - acknowledged";
  else if (report.state == State::AWAKE && report.acknowledged) display = "Awake - acknowledged";
  else if (report.state == State::WAKING) display = "Waking - awaiting acknowledgment";
  else display = report.detail;
  const bool changed = display != last_status_;
  if (changed) {
    status_sensor_->publish_state(display);
    ESP_LOGI(TAG, "%s", display.c_str());
    last_status_ = display;
  }
  // Latest-value queue also lets the worker refuse to replay an old status if
  // the main loop stops. No mutable C++ string or ESPHome entity crosses tasks.
  if (changed || elapsed(now, reported_at_, 250)) {
    xQueueOverwrite(reports_, &report); reported_at_ = now;
  }
}

namespace {
struct ResponseBuffer { std::string *body; bool too_large{false}; };
esp_err_t on_http(esp_http_client_event_t *event) {
  auto *buffer = static_cast<ResponseBuffer *>(event->user_data);
  if (event->event_id == HTTP_EVENT_ON_DATA && event->data_len > 0) {
    if (buffer->body->size() + size_t(event->data_len) > 65536) { buffer->too_large = true; return ESP_FAIL; }
    buffer->body->append(static_cast<const char *>(event->data), event->data_len);
  }
  return ESP_OK;
}
}

int HouseBose::request_(const std::string &path, const std::string *body, std::string &response) {
  response.clear(); ResponseBuffer buffer{&response};
  const std::string url = server_url_ + path;
  esp_http_client_config_t config{};
  config.url = url.c_str(); config.timeout_ms = 900;
  config.event_handler = on_http; config.user_data = &buffer;
  config.disable_auto_redirect = true;
  auto client = esp_http_client_init(&config);
  if (!client) return -1;
  if (body) {
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, body->data(), body->size());
  }
  const auto error = esp_http_client_perform(client);
  const int status = error == ESP_OK && !buffer.too_large ? esp_http_client_get_status_code(client) : -1;
  esp_http_client_cleanup(client);
  return status;
}

void HouseBose::worker_() {
  std::string sub_id, response;
  Report latest{}, sent{};
  bool have_report = false, have_sent = false;
  uint32_t polled_at = millis() - 2000, posted_at = millis() - 5000;
  while (true) {
    uint32_t now = millis();
    if (xQueueReceive(reports_, &latest, 0) == pdTRUE) have_report = true;
    wifi_ap_record_t ap{};
    const bool connected = esp_wifi_sta_get_ap_info(&ap) == ESP_OK;
    if (elapsed(now, polled_at, 2000)) {
      polled_at = now;
      Demand demand; demand.received_ms = now;
      if (connected && sub_id.empty() && request_("/api/subwoofers", nullptr, response) == 200) {
        Json root(cJSON_Parse(response.c_str()), cJSON_Delete);
        sub_id = discover_sub(root.get(), renderer_id_);
      }
      if (connected && !sub_id.empty()) {
        const int status = request_("/api/subwoofers/" + sub_id + "/demand", nullptr, response);
        if (status == 200) {
          Json root(cJSON_Parse(response.c_str()), cJSON_Delete);
          demand = parse_demand(root.get(), sub_id, renderer_id_, millis());
        } else if (status == 404) sub_id.clear();
      }
      xQueueOverwrite(demands_, &demand);  // Any failure is UNKNOWN, not absent.
    }
    // Refresh after HTTP wait so old acknowledgments/countdowns aren't posted.
    if (xQueueReceive(reports_, &latest, 0) == pdTRUE) have_report = true;
    now = millis();
    const bool changed = !have_sent || latest.mode != sent.mode || latest.state != sent.state ||
                         latest.acknowledged != sent.acknowledged || std::strcmp(latest.detail, sent.detail) != 0;
    const uint32_t interval = latest.state == State::STANDBY_PENDING ? 1000 : 4000;
    if (connected && !sub_id.empty() && have_report && !elapsed(now, latest.sampled_ms, 1500) &&
        ((changed && elapsed(now, posted_at, 500)) || elapsed(now, posted_at, interval))) {
      Json body(cJSON_CreateObject(), cJSON_Delete);
      if (body) {
        cJSON_AddStringToObject(body.get(), "rendererId", renderer_id_.c_str());
        cJSON_AddStringToObject(body.get(), "mode", mode_name(latest.mode));
        cJSON_AddStringToObject(body.get(), "state", state_name(latest.state));
        cJSON_AddBoolToObject(body.get(), "acknowledged", latest.acknowledged);
        cJSON_AddStringToObject(body.get(), "detail", latest.detail);
        if (latest.state == State::STANDBY_PENDING)
          cJSON_AddNumberToObject(body.get(), "standbyRemainingSeconds", latest.remaining_seconds);
        char *encoded = cJSON_PrintUnformatted(body.get());
        if (encoded) {
          const std::string text(encoded); cJSON_free(encoded);
          const int code = request_("/api/subwoofers/" + sub_id + "/status", &text, response);
          if (code == 200) { sent = latest; have_sent = true; }
          else if (code == 404) sub_id.clear();
        }
      }
      posted_at = millis();
      // Avoid a hot retry loop when a transition could not be delivered.
      if (!have_sent || changed) vTaskDelay(pdMS_TO_TICKS(250));
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
}}  // namespace
