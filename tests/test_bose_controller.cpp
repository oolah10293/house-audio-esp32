#include "../components/house_bose/bose_controller.h"
#include <cassert>
#include <functional>
#include <iostream>
#include <vector>
using namespace esphome::house_bose;

struct Rig : BoseTransport {
  uint32_t now{0};
  std::vector<std::vector<uint8_t>> frames;
  std::vector<uint32_t> times;
  BoseController controller{*this};
  std::function<bool(size_t)> accept = [](size_t) { return true; };
  explicit Rig(uint32_t start = 0) : now(start) { controller.begin(now); }
  uint32_t transmit(const uint8_t *data, size_t size) override {
    frames.emplace_back(data, data + size); times.push_back(now); return now;
  }
  void step() {
    const auto before = frames.size();
    controller.tick(now);
    if (frames.size() != before && accept(frames.size())) {
      for (auto b : frames.back()) controller.receive(b, now);
      controller.receive(0, now);
    }
    now += 10;
  }
  void run(uint32_t duration) { for (uint32_t i = 0; i < duration; i += 10) step(); }
  void demand(bool needed) { controller.demand(true, needed, now, 10000); }
  void unknown() { controller.demand(false, false, now, 10000); }
  void sustained(bool needed, unsigned seconds) { for (unsigned i = 0; i < seconds; ++i) { demand(needed); run(1000); } }
  Report report() { return controller.report(now); }
};

int main() {
  {
    Rig r; r.run(4000);
    assert(r.frames.size() == 1 && r.frames[0] == (std::vector<uint8_t>{1,0,0x80,0x81}));
    assert(r.report().state == State::STANDBY && r.report().acknowledged);
    r.run(60000); assert(r.frames.size() == 1); // Boot unknown never wakes or spams.
    r.demand(true); r.run(1000);
    assert(r.report().state == State::AWAKE && r.report().acknowledged);
    assert(r.frames.size() == 6);
    assert(r.frames[1] == r.frames[2]);
    assert(r.times[2] - r.times[1] >= 180);
    for (unsigned i=3; i<6; ++i) assert(r.times[i]-r.times[i-1] >= 30);
    assert(r.frames[3] == (std::vector<uint8_t>{0x0D,0,2,0x0F}));
    assert(r.frames[4] == (std::vector<uint8_t>{0x0E,0,0,0,0x0E}));
    r.sustained(false, 29); assert(r.frames.size() == 6);
    assert(r.report().state == State::STANDBY_PENDING);
    r.sustained(true, 1); assert(r.report().state == State::AWAKE); // cancellation
    r.sustained(false, 31); assert(r.frames.size() == 7 && r.report().state == State::STANDBY);
    r.sustained(false, 20); assert(r.frames.size() == 7);
  }
  {
    Rig r; r.sustained(true, 5); r.sustained(false, 25);
    r.unknown(); r.run(10000); assert(r.report().state == State::AWAKE);
    r.sustained(false, 29); assert(r.report().state == State::STANDBY_PENDING);
    r.sustained(false, 2); assert(r.report().state == State::STANDBY);
  }
  {
    Rig r; r.sustained(true, 5); r.demand(false); r.run(31000);
    assert(r.report().state == State::AWAKE); // TTL expiration cancels grace.
    r.controller.set_mode(Mode::FORCE_STANDBY); r.sustained(true, 2);
    assert(r.report().state == State::STANDBY);
    r.controller.set_mode(Mode::FORCE_AWAKE); r.unknown(); r.run(2000);
    assert(r.report().state == State::AWAKE);
    r.controller.set_mode(Mode::AUTO); r.sustained(false, 31);
    assert(r.report().state == State::STANDBY);
  }
  {
    Rig r; r.demand(true); r.accept = [](size_t n) { return n > 1; }; r.run(6000);
    assert(r.report().state == State::AWAKE && r.frames.size() == 6); // Retry first wake without toggle.
  }
  {
    Rig r; r.controller.set_mode(Mode::FORCE_AWAKE); r.accept = [](size_t) { return false; };
    r.run(10000); assert(r.frames.size() == 3 && r.report().state == State::ERROR && !r.report().acknowledged);
    r.run(60000); assert(r.frames.size() == 3); // bounded retries
    r.accept = [](size_t) { return true; }; r.controller.set_mode(Mode::FORCE_AWAKE); r.run(2000);
    assert(r.report().state == State::AWAKE); // explicit retry of same mode
  }
  {
    Rig r; r.controller.set_mode(Mode::FORCE_AWAKE); r.run(3090);
    assert(r.frames.size() == 1);
    r.controller.set_mode(Mode::FORCE_STANDBY); r.run(1000);
    assert(r.report().state == State::STANDBY && r.frames.size() == 2);
    assert(r.frames.back()[2] == 0x80); // canceled wake doesn't finish configuring/unmute
    r.controller.set_mode(Mode::FORCE_AWAKE); r.run(2000);
    assert(r.report().state == State::AWAKE);
    r.controller.set_attenuation(23); r.run(1000);
    assert(r.frames.back() == (std::vector<uint8_t>{2,0,23,21}));
    const auto count = r.frames.size(); r.run(1000); assert(r.frames.size() == count);
  }
  {
    Rig r; r.accept = [](size_t) { return false; }; r.run(3060);
    r.controller.receive(0, r.now); r.run(10000);
    assert(r.report().state == State::ERROR); // lone ACK never validates echo
    Rig echo; echo.accept = [](size_t) { return false; }; echo.run(3060);
    for (auto b : echo.frames.back()) echo.controller.receive(b, echo.now);
    echo.run(10000); assert(echo.report().state == State::ERROR); // full echo without ACK
  }
  {
    Rig r(0xFFFFF000U); r.sustained(true, 6); r.sustained(false, 31);
    assert(r.report().state == State::STANDBY); // rollover
  }
  std::cout << "PASS: boot, presence, grace, cancellation, outages, TTL, overrides, retries, ACK validation, attenuation and rollover\n";
}
