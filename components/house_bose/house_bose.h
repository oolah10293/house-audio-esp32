#pragma once
#include "bose_controller.h"
#include "pi_contract.h"
#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string>

namespace esphome { namespace house_bose {
class HouseBose : public Component, public uart::UARTDevice, public BoseTransport {
 public:
  HouseBose() : controller_(*this) {}
  void set_tx_pin(InternalGPIOPin *pin) { tx_pin_ = pin; }
  void set_server_url(const std::string &url) { server_url_ = url; }
  void set_status_sensor(text_sensor::TextSensor *sensor) { status_sensor_ = sensor; }
  void set_mode(const std::string &mode) {
    controller_.set_mode(mode == "Force awake" ? Mode::FORCE_AWAKE : mode == "Force standby" ? Mode::FORCE_STANDBY : Mode::AUTO);
  }
  void set_attenuation(int attenuation) { controller_.set_attenuation(attenuation); }
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::AFTER_WIFI; }
  uint32_t transmit(const uint8_t *frame, size_t size) override;
 protected:
  static void worker_entry_(void *arg) { static_cast<HouseBose *>(arg)->worker_(); }
  void worker_();
  int request_(const std::string &path, const std::string *body, std::string &response);
  InternalGPIOPin *tx_pin_{nullptr};
  text_sensor::TextSensor *status_sensor_{nullptr};
  std::string server_url_, renderer_id_, last_status_;
  QueueHandle_t demands_{nullptr}, reports_{nullptr};
  TaskHandle_t worker_handle_{nullptr};
  BoseController controller_;
  uint32_t reported_at_{0};
};
}}
