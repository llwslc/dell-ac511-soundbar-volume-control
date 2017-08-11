#!/usr/bin/python
# -*- coding: UTF-8 -*-

import os
import evdev

devName = "Dell AC511 USB SoundBar";

# 查找 Dell AC511
def findAC511():
  # print "find";

  ac511Event = None;

  devices = [evdev.InputDevice(fn) for fn in evdev.list_devices()]
  for device in devices:
    if device.name.find(devName) != -1:
      ac511Event = device.fn;

  if ac511Event is not None:
    openAC511(ac511Event);

  return;

# 打开 Dell AC511
def openAC511(ac511Event):
  # print "open";

  device = evdev.InputDevice(ac511Event);

  cardList = os.popen("aplay -l").readlines();
  cardNum = None;
  for card in cardList:
    if card.find(devName) != -1:
      cardNum = card.split(":")[0].split(" ")[1];

  if cardNum is None:
    return;

  try:
    for event in device.read_loop():
      if event.type == evdev.ecodes.EV_KEY:
        if event.code == evdev.ecodes.KEY_VOLUMEDOWN:
          # print "down"
          os.popen("amixer -c " + str(cardNum) + " sset PCM 1db-");
        if event.code == evdev.ecodes.KEY_VOLUMEUP:
          # print "up"
          os.popen("amixer -c " + str(cardNum) + " sset PCM 1db+");
  except:
    # print "close"
    None;

  return;


while True:
  findAC511();
