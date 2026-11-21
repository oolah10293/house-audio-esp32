#include "../components/house_bose/pi_contract.h"
#include <cassert>
#include <iostream>
using namespace esphome::house_bose;
int main() {
  const std::string renderer = "AA:BB:CC:DD:EE:FF", sub = "sub-test";
  auto *root = cJSON_Parse(R"({"service":"house-audio-server","subwoofers":[{"id":"sub-other","rendererId":"other"},{"id":"sub-test","rendererId":"AA:BB:CC:DD:EE:FF"}]})");
  assert(discover_sub(root, renderer) == sub);
  assert(discover_sub(root, "wrong").empty()); cJSON_Delete(root);
  root = cJSON_Parse(R"({"service":"house-audio-server","id":"sub-test","rendererId":"AA:BB:CC:DD:EE:FF","known":true,"needed":false,"validForSeconds":10,"standbyDelaySeconds":30})");
  auto d = parse_demand(root, sub, renderer, 42);
  assert(d.known && !d.needed && d.received_ms == 42 && d.valid_ms == 10000);
  assert(!parse_demand(root, "wrong", renderer, 0).known);
  assert(!parse_demand(root, sub, "wrong", 0).known);
  cJSON_ReplaceItemInObject(root, "needed", cJSON_CreateNull()); assert(!parse_demand(root, sub, renderer, 0).known);
  cJSON_ReplaceItemInObject(root, "needed", cJSON_CreateTrue()); assert(parse_demand(root, sub, renderer, 0).needed);
  cJSON_ReplaceItemInObject(root, "known", cJSON_CreateFalse()); assert(!parse_demand(root, sub, renderer, 0).known);
  cJSON_ReplaceItemInObject(root, "known", cJSON_CreateTrue());
  cJSON_ReplaceItemInObject(root, "validForSeconds", cJSON_CreateNumber(11)); assert(!parse_demand(root, sub, renderer, 0).known);
  cJSON_ReplaceItemInObject(root, "validForSeconds", cJSON_CreateNumber(-1)); assert(!parse_demand(root, sub, renderer, 0).known);
  cJSON_Delete(root);
  assert(!parse_demand(nullptr, sub, renderer, 0).known);
  assert(!safe_id("../bad") && !safe_id("sub?bad") && safe_id("sub-abc123"));
  std::cout << "PASS: stable-ID discovery and strict demand schema; failures remain unknown\n";
}
