"""Mono/LR4 add-on for the pinned c-MM ESPHome Snapclient PCM path."""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "snapclient"]
CONF_LOWPASS_HZ = "lowpass_hz"

ns = cg.esphome_ns.namespace("house_sub_dsp")
HouseSubDSP = ns.class_("HouseSubDSP", cg.Component)

CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HouseSubDSP),
        cv.Optional(CONF_LOWPASS_HZ, default=90.0): cv.float_range(min=20.0, max=200.0),
    }).extend(cv.COMPONENT_SCHEMA),
    cv.require_framework_version(esp_idf=cv.Version(5, 1, 1)),
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_lowpass_hz(config[CONF_LOWPASS_HZ]))
    # Keep upstream decoding, software volume, queueing and timestamps intact.
    # Only its external PCM-processing call is intercepted; __real calls through.
    # Cross-TU LTO can inline the upstream call before the linker can wrap it.
    cg.add_build_flag("-fno-lto")
    cg.add_build_flag("-Wl,--wrap=dsp_processor_worker")
