#pragma once
#include "esphome/core/component.h"
#include "esphome/components/house_sub_dsp/house_sub_dsp.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <atomic>
#include <string>

namespace esphome { namespace house_rio {

// Pure software-audio following for an amplifier with NO wired standby input.
// Snapcast still runs; the external TPA3118 remains powered at all times.
class HouseRio : public Component {
 public:
  void set_dsp(house_sub_dsp::HouseSubDSP *dsp) { dsp_ = dsp; }
  void set_server_url(const std::string &url) { server_url_ = url; }
  void set_status_sensor(text_sensor::TextSensor *sensor) { status_sensor_ = sensor; }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }

 protected:
  static void worker_entry_(void *arg) { static_cast<HouseRio *>(arg)->worker_(); }
  void worker_();
  int request_(const std::string &path, const std::string *body, std::string &response);
  house_sub_dsp::HouseSubDSP *dsp_{nullptr};
  text_sensor::TextSensor *status_sensor_{nullptr};
  TaskHandle_t worker_handle_{nullptr};
  std::string server_url_, renderer_id_, last_display_;
  std::atomic<bool> audio_muted_{true};
  std::atomic<bool> have_demand_{false};
  std::atomic<bool> pi_available_{false};
  bool last_applied_muted_{true};
};

}}  // namespace esphome::house_rio
