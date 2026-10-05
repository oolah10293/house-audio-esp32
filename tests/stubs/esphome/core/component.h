#pragma once
namespace esphome {
class Component { public: virtual ~Component()=default; virtual void setup() {} virtual void dump_config() {} virtual float get_setup_priority() const {return 600.0f;} };
}
