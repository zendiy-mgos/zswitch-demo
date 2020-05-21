#include "mgos.h"
#include "mgos_gpio.h"
#include "mgos_zswitch.h"


static void relay_timer_cb(void *arg) {
  struct mgos_zswitch *handle = (struct mgos_zswitch *)arg;
  int state = mgos_zswitch_state_toggle(handle);
  if (state == MGOS_ZTHING_RESULT_ERROR) {
    LOG(LL_ERROR, ("Error toggling status of switch '%s'", handle->id));  
  } else {
    LOG(LL_INFO, ("Switch '%s', switched %s", handle->id,
      (state == true ? "ON" : "OFF")));
  }
}

 bool zswitch_state_handler(enum mgos_zthing_state_act act,
                            struct mgos_zswitch_state *state,
                            void *user_data) {
  bool gpio_val;
  bool active_high = mgos_sys_config_get_app_relays_active_high();  
  
  int pin;
  if (strcmp(state->handle->id, mgos_sys_config_get_app_relay1_id()) == 0) {
    pin = mgos_sys_config_get_app_relay1_pin();
  } else if (strcmp(state->handle->id, mgos_sys_config_get_app_relay2_id()) == 0) {
    pin = mgos_sys_config_get_app_relay2_pin();
  } else {
    return false;
  }

  if (act == MGOS_ZTHING_STATE_SET) {
    if (state->value) {
      gpio_val = (active_high ? true : false);
    } else {
      gpio_val = (active_high ? false : true);
    }
    mgos_gpio_write(pin, gpio_val);
    mgos_msleep(10);
    if (mgos_gpio_read(pin) == gpio_val) return true;
    LOG(LL_ERROR, ("Unexpected GPIO value reading '%s' state.",
      state->handle->id));
  } else if (act == MGOS_ZTHING_STATE_GET) {
    gpio_val = mgos_gpio_read(pin);
    if (active_high) {
      state->value = gpio_val ? true : false;
    } else {
      state->value = gpio_val ? false : true;
    } 
    return true;
  }
  return false;

  (void) user_data;
}

enum mgos_app_init_result mgos_app_init(void) {
  /* Create switches having the same group ID,
   * so when one will be turned on, all others
   * will be automatically torned off.
   */
  bool active_high = mgos_sys_config_get_app_relays_active_high();
  struct mgos_zswitch_cfg cfg = MGOS_ZSWITCH_CFG;
  cfg.group_id = 1;
  
  struct mgos_zswitch *sw1 = NULL;
  sw1 = mgos_zswitch_create(mgos_sys_config_get_app_relay1_id(), &cfg);  
  if (sw1 == NULL) return MGOS_APP_INIT_ERROR;

  if (!mgos_gpio_setup_output(mgos_sys_config_get_app_relay1_pin(),
      !active_high)) return MGOS_APP_INIT_ERROR;

  if (!mgos_zswitch_state_handler_set(sw1, zswitch_state_handler, NULL)) {
    return MGOS_APP_INIT_ERROR;
  }


  struct mgos_zswitch *sw2 = NULL;
  sw2 = mgos_zswitch_create(mgos_sys_config_get_app_relay2_id(), &cfg);
  if (sw2 == NULL) return MGOS_APP_INIT_ERROR;

  if (!mgos_gpio_setup_output(mgos_sys_config_get_app_relay2_pin(),
      !active_high)) return MGOS_APP_INIT_ERROR;

  if (!mgos_zswitch_state_handler_set(sw2, zswitch_state_handler, NULL)) {
    return MGOS_APP_INIT_ERROR;
  }

  mgos_set_timer(5000, MGOS_TIMER_REPEAT,
    relay_timer_cb, sw1);
  
  mgos_set_timer(10000, MGOS_TIMER_REPEAT,
    relay_timer_cb, sw2);

  return MGOS_APP_INIT_SUCCESS;
}