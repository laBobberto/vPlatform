#include "qemu/osdep.h"
#include "hw/i2c/i2c.h"
#include "qapi/error.h"
#include "qapi/visitor.h"
#include "qom/object.h"

#define TYPE_VPLATFORM_I2C "vplatform-i2c"
OBJECT_DECLARE_SIMPLE_TYPE(VPlatformI2CState, VPLATFORM_I2C)

struct VPlatformI2CState {
    I2CSlave parent_obj;
    uint8_t reg; // Единственный регистр устройства
};

static int vplatform_i2c_event(I2CSlave *s, enum i2c_event event)
{
    return 0;
}

static uint8_t vplatform_i2c_recv(I2CSlave *s)
{
    VPlatformI2CState *state = VPLATFORM_I2C(s);
    return state->reg; // Возвращаем значение регистра при чтении по I2C
}

static int vplatform_i2c_send(I2CSlave *s, uint8_t data)
{
    VPlatformI2CState *state = VPLATFORM_I2C(s);
    state->reg = data; // Записываем данные в регистр при записи по I2C
    return 0;
}

// Геттер для свойства "reg" (используется QMP/QOM)
static void vplatform_i2c_get_reg(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    VPlatformI2CState *state = VPLATFORM_I2C(obj);
    uint8_t value = state->reg;
    visit_type_uint8(v, name, &value, errp);
}

// Сеттер для свойства "reg" (используется QMP/QOM)
static void vplatform_i2c_set_reg(Object *obj, Visitor *v, const char *name,
                                  void *opaque, Error **errp)
{
    VPlatformI2CState *state = VPLATFORM_I2C(obj);
    uint8_t value;
    if (!visit_type_uint8(v, name, &value, errp)) {
        return;
    }
    state->reg = value;
}

static void vplatform_i2c_init(Object *obj)
{
    // Добавляем свойство "reg", чтобы его можно было читать/писать через QMP (qom-get/qom-set)
    object_property_add(obj, "reg", "uint8",
                        vplatform_i2c_get_reg,
                        vplatform_i2c_set_reg,
                        NULL, NULL);
}

static void vplatform_i2c_class_init(ObjectClass *oc, const void *data)
{
    I2CSlaveClass *sc = I2C_SLAVE_CLASS(oc);
    sc->event = vplatform_i2c_event;
    sc->recv = vplatform_i2c_recv;
    sc->send = vplatform_i2c_send;
}

static const TypeInfo vplatform_i2c_info = {
    .name = TYPE_VPLATFORM_I2C,
    .parent = TYPE_I2C_SLAVE,
    .instance_size = sizeof(VPlatformI2CState),
    .instance_init = vplatform_i2c_init,
    .class_init = vplatform_i2c_class_init,
};

static void vplatform_i2c_register_types(void)
{
    type_register_static(&vplatform_i2c_info);
}

type_init(vplatform_i2c_register_types)
