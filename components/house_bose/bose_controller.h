#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace esphome { namespace house_bose {

enum class Mode { AUTO, FORCE_STANDBY, FORCE_AWAKE };
enum class State { UNKNOWN, STANDBY, WAKING, AWAKE, STANDBY_PENDING, ERROR };
inline const char *mode_name(Mode m) {
  return m == Mode::AUTO ? "auto" : m == Mode::FORCE_STANDBY ? "force_standby" : "force_awake";
}
inline const char *state_name(State s) {
  switch (s) {
    case State::STANDBY: return "standby";
    case State::WAKING: return "waking";
    case State::AWAKE: return "awake";
    case State::STANDBY_PENDING: return "standby_pending";
    case State::ERROR: return "error";
    default: return "unknown";
  }
}
inline bool elapsed(uint32_t now, uint32_t then, uint32_t interval) { return uint32_t(now - then) >= interval; }

struct Report {
  Mode mode{Mode::AUTO};
  State state{State::UNKNOWN};
  bool acknowledged{false};
  uint32_t remaining_seconds{0};
  uint32_t sampled_ms{0};
  char detail[128]{};
};

class BoseTransport {
 public:
  virtual ~BoseTransport() = default;
  // Sends one <= 5-byte frame only; returns monotonic time AFTER transmission.
  virtual uint32_t transmit(const uint8_t *frame, size_t size) = 0;
};

// Single-threaded policy/command engine. Main loop owns it; HTTP uses queues.
// Timers use unsigned subtraction and remain correct across millis() rollover.
class BoseController {
 public:
  explicit BoseController(BoseTransport &transport) : transport_(transport) {}
  void begin(uint32_t now) { boot_at_ = now; begun_ = true; }
  void set_mode(Mode mode) {
    if (mode != mode_ || failed_) { failed_ = false; attempts_ = 0; }
    mode_ = mode;
    pending_ = false;
  }
  void set_attenuation(int attenuation) { attenuation_ = std::clamp(attenuation, 0, 40); }
  void demand(bool known, bool needed, uint32_t received, uint32_t valid_ms) {
    known_ = known && valid_ms > 0 && valid_ms <= 10000;
    needed_ = needed;
    received_at_ = received;
    valid_ms_ = valid_ms;
    // A brief outage must cancel the old countdown even if a later good sample
    // arrives before tick(). It must never count outage time as confirmed absence.
    if (!known_) pending_ = false;
  }
  void receive(uint8_t byte, uint32_t now) {
    if (!waiting_ || rx_bad_ || ack_ || elapsed(now, sent_at_, 120)) return;
    if (echo_pos_ < frame_size_) {
      if (byte != frame_[echo_pos_++]) rx_bad_ = true;
    } else {
      // The proven bus reply is the exact local TX echo followed by 00.
      // Echo's own zero bytes, lone 00 and nonzero replies cannot qualify.
      ack_ = byte == 0;
      rx_bad_ = !ack_;
    }
  }

  void tick(uint32_t now) {
    if (!begun_) return;
    const bool fresh = known_ && !elapsed(now, received_at_, valid_ms_);
    bool wanted = desired_awake_;
    if (mode_ != Mode::AUTO) {
      pending_ = false;
      wanted = mode_ == Mode::FORCE_AWAKE;
    } else if (!fresh) {
      pending_ = false;  // Hold state/complete current request. No false absence.
    } else if (needed_) {
      pending_ = false;
      wanted = true;
    } else if (desired_awake_) {
      if (!pending_) { pending_ = true; absent_since_ = now; }
      if (elapsed(now, absent_since_, 30000)) { wanted = false; pending_ = false; }
    }
    if (wanted != desired_awake_) {
      desired_awake_ = wanted;
      failed_ = false;
      attempts_ = 0;
    }
    if (!elapsed(now, boot_at_, 3000)) return;

    if (waiting_) {
      if (!ack_ && !rx_bad_ && !elapsed(now, sent_at_, 120)) return;
      if ((rx_bad_ || !ack_) && elapsed(now, sent_at_, 30)) {
        waiting_ = false;
        active_ = false;
        state_ = State::ERROR;
        last_ack_ = false;
        retry_at_ = now;
        retry_pending_ = true;
        failed_ = ++attempts_ >= 3;
        return;
      }
      if (!ack_ || !elapsed(now, sent_at_, command_gap_)) return;
      waiting_ = false;
      ++step_;
      if (step_ >= (target_awake_ && !volume_only_ ? 5U : 1U)) {
        active_ = false;
        retry_pending_ = false;
        attempts_ = 0;
        applied_awake_ = target_awake_;
        have_applied_ = true;
        state_ = target_awake_ ? State::AWAKE : State::STANDBY;
        last_ack_ = true;
        if (target_awake_) applied_attenuation_ = sent_attenuation_;
      }
    }

    // Honor a new mode only at a complete frame boundary. A partially configured
    // wake is never reported as awake. Even after canceling it, send standby.
    if (active_ && target_awake_ != desired_awake_) { active_ = false; have_applied_ = false; }
    if (failed_) return;
    if (retry_pending_ && !elapsed(now, retry_at_, 1000)) return;
    if (!active_) {
      if (!have_applied_ || applied_awake_ != desired_awake_ || state_ == State::ERROR) {
        start_(now, desired_awake_, false);
      } else if (desired_awake_ && applied_attenuation_ != attenuation_) {
        start_(now, true, true);
      } else return;
    }
    if (!elapsed(now, sequence_at_, 50)) return;
    send_();
  }

  Report report(uint32_t now) const {
    Report r;
    r.mode = mode_; r.state = state_; r.sampled_ms = now;
    r.acknowledged = last_ack_ && !active_ && (state_ == State::AWAKE || state_ == State::STANDBY);
    const char *detail = "Waiting for initial standby command";
    if (state_ == State::ERROR) detail = failed_ ? "Bose did not acknowledge after 3 attempts; reselect mode to retry" : "Bose reply missing or invalid; retrying";
    else if (active_) detail = target_awake_ ? "Wake/configuration in progress" : "Standby command requested; awaiting acknowledgment";
    else if (r.acknowledged) detail = "Bose command acknowledged; physical amplifier power is not measured";
    if (pending_ && !active_ && state_ == State::AWAKE) {
      r.state = State::STANDBY_PENDING;
      r.acknowledged = false;
      const uint32_t spent = uint32_t(now - absent_since_);
      r.remaining_seconds = spent < 30000 ? (30000 - spent + 999) / 1000 : 0;
      detail = "Waiting after last assigned speaker left";
    }
    std::strncpy(r.detail, detail, sizeof(r.detail) - 1);
    return r;
  }

 private:
  void start_(uint32_t now, bool awake, bool volume_only) {
    target_awake_ = awake; volume_only_ = volume_only; active_ = true;
    step_ = 0; sequence_at_ = now; retry_pending_ = false; last_ack_ = false;
    state_ = awake ? State::WAKING : State::UNKNOWN;
  }
  void send_() {
    const uint8_t wake[] = {1, 0, 0, 1}, standby[] = {1, 0, 0x80, 0x81};
    const uint8_t analog[] = {0x0D, 0, 2, 0x0F}, stream[] = {0x0E, 0, 0, 0, 0x0E};
    sent_attenuation_ = attenuation_;
    const uint8_t volume[] = {2, 0, uint8_t(attenuation_), uint8_t(2 ^ attenuation_)};
    const uint8_t *frame = !target_awake_ ? standby : (volume_only_ || step_ == 4) ? volume : step_ < 2 ? wake : step_ == 2 ? analog : stream;
    frame_size_ = target_awake_ && !volume_only_ && step_ == 3 ? 5 : 4;
    std::memcpy(frame_, frame, frame_size_);
    command_gap_ = target_awake_ && !volume_only_ && step_ == 0 ? 180 : 30;
    echo_pos_ = 0; ack_ = false; rx_bad_ = false; waiting_ = true;
    sent_at_ = transport_.transmit(frame_, frame_size_);
  }
  BoseTransport &transport_;
  Mode mode_{Mode::AUTO}; State state_{State::UNKNOWN};
  bool begun_{false}, known_{false}, needed_{false}, pending_{false};
  bool desired_awake_{false}, target_awake_{false}, applied_awake_{false}, have_applied_{false};
  bool active_{false}, volume_only_{false}, waiting_{false}, ack_{false}, rx_bad_{false};
  bool failed_{false}, retry_pending_{false}, last_ack_{false};
  int attenuation_{12}, sent_attenuation_{12}, applied_attenuation_{-1};
  unsigned attempts_{0}, step_{0}; size_t echo_pos_{0}, frame_size_{0}; uint8_t frame_[5]{};
  uint32_t boot_at_{0}, received_at_{0}, valid_ms_{10000}, absent_since_{0};
  uint32_t sequence_at_{0}, sent_at_{0}, command_gap_{30}, retry_at_{0};
};
}}  // namespace esphome::house_bose
