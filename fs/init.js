load("api_timer.js")
load("api_config.js")
load("api_gpio.js")
load("api_zswitch_gpio.js")

function onTimer(sw) {
  let state = sw.toggleState();
  if (state === ZenThing.RESULT_ERROR) {
    print('Error toggling status of switch', sw.id);
  } else {
    print('Switch', sw.id, 'switched', (state === true  ? 'ON' : 'OFF'));
  }
}

function stateHandler(act, state, pinCfg) {
  let pin = Cfg.get(pinCfg);
  let isah = Cfg.get('app.relays_active_high');
  if (act === ZenThing.ACT_STATE_SET) {
    print('Reading state of switch ', state.thing.id);
    if (state.value) {
      /* switch ON */
      GPIO.write(pin, (isah ? 1 : 0));
    } else {
      /* switch OFF */
      GPIO.write(pin, (isah ? 0 : 1));
    }
  } else if (act === ZenThing.ACT_STATE_GET) {
    print('Getting state of switch ', state.thing.id);
    let r = GPIO.read(pin);
    state.value = (isah ? r : !r);
  }
  return true;
}

/* Create switches having the same group ID,
 * so when one will be turned on, all others
 * will be automatically torned off.
 */   
let cfg = {groupId: 1};
let sw1 = ZenSwitch.create(Cfg.get('app.relay1.id'), cfg);
let sw2 = ZenSwitch.create(Cfg.get('app.relay2.id'), cfg);

let success = false;
let gpioCfg = {activeHigh: Cfg.get('app.relays_active_high')};
if (sw1 && sw2) {

  GPIO.setup_output(Cfg.get('app.relay1.pin'),
    (Cfg.get('app.relays_active_high') ? false : true));
  let okh1 = sw1.setStateHandler(stateHandler, 'app.relay1.pin');

  GPIO.setup_output(Cfg.get('app.relay2.pin'),
    (Cfg.get('app.relays_active_high') ? false : true));
  let okh2 = sw2.setStateHandler(stateHandler, 'app.relay2.pin');

  if (okh1 && okh2) {
    Timer.set(5000, Timer.REPEAT, onTimer, sw1);
    Timer.set(10000, Timer.REPEAT, onTimer, sw2);
    success = true;
  }
}

if (!success) {
  if (sw1) {
    sw1.resetStateHandler();
    sw1.close();
  }
  if (sw2) {
    sw2.resetStateHandler();
    sw2.close();
  }
  print('Error initializing the firmware');
}
