"""Rio software-audio follow control using the Pi subwoofer demand contract.

No hardware standby or amplifier power switching is claimed.
"""
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, text_sensor
from esphome.components.house_sub_dsp import HouseSubDSP

DEPENDENCIES = ["esp32", "wifi", "house_sub_dsp"]
AUTO_LOAD = ["text_sensor"]
CONF_DSP_ID = "dsp_id"

ns = cg.esphome_ns.namespace("house_rio")
HouseRio = ns.class_("HouseRio", cg.Component)


def validate_server_url(value):
    value = cv.url(value)
    if not value.startswith("http://") or any(c in value[7:].rstrip("/") for c in "/?#"):
        raise cv.Invalid("server_url must be an HTTP server origin")
    return value


CONFIG_SCHEMA = cv.All(
    cv.Schema({
        cv.GenerateID(): cv.declare_id(HouseRio),
        cv.Required(CONF_DSP_ID): cv.use_id(HouseSubDSP),
        cv.Optional("server_url", default="http://192.168.1.12:8787"): validate_server_url,
        cv.Required("status"): text_sensor.text_sensor_schema(),
    }).extend(cv.COMPONENT_SCHEMA),
    cv.only_with_esp_idf,
)


async def to_code(config):
    var = cg.new_Pvariable(config["id"])
    await cg.register_component(var, config)
    dsp = await cg.get_variable(config[CONF_DSP_ID])
    cg.add(var.set_dsp(dsp))
    cg.add(var.set_server_url(config["server_url"].rstrip("/")))
    sensor = await text_sensor.new_text_sensor(config["status"])
    cg.add(var.set_status_sensor(sensor))
    esp32.include_builtin_idf_component("esp_http_client")
    esp32.include_builtin_idf_component("json")
