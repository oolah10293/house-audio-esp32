"""Bose amplifier policy and feedback, independent of the Snapcast audio task."""
from esphome import pins
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import esp32, uart, text_sensor
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "wifi", "uart"]
AUTO_LOAD = ["text_sensor"]
ns = cg.esphome_ns.namespace("house_bose")
HouseBose = ns.class_("HouseBose", cg.Component, uart.UARTDevice)

def validate_server_url(value):
    value = cv.url(value)
    if not value.startswith("http://") or any(c in value[7:].rstrip("/") for c in "/?#"):
        raise cv.Invalid("server_url must be an HTTP server origin, e.g. http://192.168.1.12:8787")
    return value

CONFIG_SCHEMA = cv.All(cv.Schema({
    cv.GenerateID(): cv.declare_id(HouseBose),
    cv.Required("tx_pin"): pins.internal_gpio_output_pin_schema,
    cv.Optional("server_url", default="http://192.168.1.12:8787"): validate_server_url,
    cv.Required("status"): text_sensor.text_sensor_schema(),
}).extend(cv.COMPONENT_SCHEMA).extend(uart.UART_DEVICE_SCHEMA), cv.only_with_esp_idf)

FINAL_VALIDATE_SCHEMA = uart.final_validate_device_schema(
    "house_bose", baud_rate=4800, require_rx=True, data_bits=8, parity="NONE", stop_bits=1)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)
    pin = await cg.gpio_pin_expression(config["tx_pin"])
    cg.add(var.set_tx_pin(pin))
    cg.add(var.set_server_url(config["server_url"].rstrip("/")))
    sensor = await text_sensor.new_text_sensor(config["status"])
    cg.add(var.set_status_sensor(sensor))
    esp32.include_builtin_idf_component("esp_http_client")
    esp32.include_builtin_idf_component("json")
