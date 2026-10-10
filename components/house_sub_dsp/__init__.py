"""Runtime mono/crossover controls for the pinned ESPHome Snapclient PR #14389."""
import esphome.codegen as cg
import esphome.config_validation as cv
import esphome.final_validate as fv
from esphome.const import CONF_ID

DEPENDENCIES = ["esp32", "media_player"]
CONF_LOWPASS_HZ = "lowpass_hz"
CONF_LOWCUT_HZ = "lowcut_hz"
CONF_PHASE_INVERTED = "phase_inverted"
CONF_CROSSOVER_BYPASS = "crossover_bypass"
EXPECTED_CORE = "774268009d1a6c3d1664fad533409a22e5a41e83"
EXPECTED_REPO = "https://github.com/luar123/snapclient.git"

ns = cg.esphome_ns.namespace("house_sub_dsp")
HouseSubDSP = ns.class_("HouseSubDSP", cg.Component)


def validate_initial_settings(config):
    if config[CONF_LOWCUT_HZ] > 0 and config[CONF_LOWCUT_HZ] >= config[CONF_LOWPASS_HZ]:
        raise cv.Invalid("lowcut_hz must be zero/off or lower than lowpass_hz")
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(HouseSubDSP),
            cv.Optional(CONF_LOWPASS_HZ, default=90.0): cv.float_range(min=20.0, max=200.0),
            cv.Optional(CONF_LOWCUT_HZ, default=0.0): cv.float_range(min=0.0, max=80.0),
            cv.Optional(CONF_PHASE_INVERTED, default=False): cv.boolean,
            cv.Optional(CONF_CROSSOVER_BYPASS, default=False): cv.boolean,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    validate_initial_settings,
    cv.require_framework_version(esp_idf=cv.Version(5, 1, 1)),
)


def validate_baseline(config):
    full = fv.full_config.get()
    players = [p for p in full.get("media_player", []) if p.get("platform") == "snapclient"]
    if len(players) != 1 or "audio_dac" in players[0]:
        raise cv.Invalid(
            "house_sub_dsp needs exactly one PR #14389 Snapclient media_player with software volume (no audio_dac)"
        )
    try:
        from esphome.components.snapclient.media_player import (
            SNAPCLIENT_GIT_REPO,
            SNAPCLIENT_GIT_VERSION,
        )
    except ImportError as err:
        raise cv.Invalid(
            "house_sub_dsp requires Snapclient PR #14389, not the old c-MM component"
        ) from err
    if SNAPCLIENT_GIT_VERSION != EXPECTED_CORE or SNAPCLIENT_GIT_REPO != EXPECTED_REPO:
        raise cv.Invalid(
            "Unreviewed Snapclient core: use luar123/esphome at 4d3280bd35fdd970e628a22197f19ab0fded1a39"
        )
    return config


FINAL_VALIDATE_SCHEMA = validate_baseline


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    cg.add(var.set_lowpass_hz(config[CONF_LOWPASS_HZ]))
    cg.add(var.set_lowcut_hz(config[CONF_LOWCUT_HZ]))
    cg.add(var.set_phase_inverted(config[CONF_PHASE_INVERTED]))
    cg.add(var.set_crossover_bypass(config[CONF_CROSSOVER_BYPASS]))
    # Wrap PR #14389's two-argument PCM-chunk/settings ABI. The original worker
    # still runs exactly once per fragment for Snapcast software volume.
    cg.add_build_flag("-fno-lto")
    cg.add_build_flag("-Wl,--wrap=dsp_processor_worker")
