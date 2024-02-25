import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir
from esphome.const import CONF_ID

AUTO_LOAD = ['climate_ir']

mbishi_ns = cg.esphome_ns.namespace('mbishi')
MbishiClimate = mbishi_ns.class_('MbishiClimate', climate_ir.ClimateIR)

#CONFIG_SCHEMA = climate_ir.CLIMATE_IR_SCHEMA.extend({
CONFIG_SCHEMA = climate_ir.CLIMATE_IR_WITH_RECEIVER_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(MbishiClimate),
})


def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield climate_ir.register_climate_ir(var, config)
