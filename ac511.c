#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/usb.h>
#include <linux/usb/input.h>
#include <linux/hid.h>

// http://blog.csdn.net/tony_shen/article/details/52125882

#define IDVENDOR 0x413C
#define IDPRODUCT 0xA503
#define IDVERSION 0x1
#define DRVNAME "Dell AC511 USB SoundBar"
//[    2.822489] input: Dell Dell AC511 USB SoundBar as /devices/platform/soc/1c1d400.usb/usb8/8-1/8-1:1.3/0003:413C:A503.0001/input/input1
//[    2.903118] hid-generic 0003:413C:A503.0001: input: USB HID v1.00 Device [Dell Dell AC511 USB SoundBar] on usb-1c1d400.usb-1/input3

#define DRIVER_VERSION "v1.0"
#define DRIVER_LICENSE "GPL"
#define DRIVER_AUTHOR "github.com/llwslc <llwslc@gmail.com>"
#define DRIVER_DESC "Dell AC511 USB SoundBar Driver"

struct stAC511
{
  char name[128];
  struct usb_device *usbdev;
  struct input_dev *inputdev;
  struct urb *irq;
  char *data;
};

void log(const char *text)
{
  printk(KERN_ALERT "AC511: %s\n", text);
}

static void ac511_irq(struct urb *urb)
{
  log("irq");
  struct stAC511 *ac511 = urb->context;
  char *data = ac511->data;
  struct input_dev *inputdev = ac511->inputdev;
  int status = 0;

  switch (urb->status)
  {
  case 0:
    break;
  case -ECONNRESET:
  case -ENOENT:
  case -ESHUTDOWN:
    return;
  default:
    status = usb_submit_urb(urb, GFP_ATOMIC);
  }

  if (status)
  {
    printk(KERN_ALERT "AC511: can't resubmit intr, status %d\n", status);
    return;
  }

  int i;
  for (i = 0; i < 16; i++)
  {
    printk("%02x  ", data[i]);
  }
  printk("\n");

  // input_report_key(inputdev, KEY_VOLUMEDOWN, data[0]);
  // input_report_key(inputdev, KEY_VOLUMEUP,   data[1]);

  // input_sync(inputdev);
}

static int ac511_open(struct input_dev *inputdev)
{
  log("open");
  struct stAC511 *ac511 = input_get_drvdata(inputdev);

  if (usb_submit_urb(ac511->irq, GFP_KERNEL))
    return -EIO;

  return 0;
}

static void ac511_close(struct input_dev *inputdev)
{
  log("close");
  struct stAC511 *ac511 = input_get_drvdata(inputdev);

  usb_kill_urb(ac511->irq);
}

static int ac511_probe(struct usb_interface *intf, const struct usb_device_id *id)
{
  log("probe");
  struct usb_device *usbdev = interface_to_usbdev(intf);
  struct usb_host_interface *interface;
  struct usb_endpoint_descriptor *endpoint;
  struct stAC511 *ac511;
  struct input_dev *inputdev;
  int pipe, maxp;
  int error = -ENOMEM;

  interface = intf->cur_altsetting;

  if (interface->desc.bNumEndpoints != 1)
    return -ENODEV;

  endpoint = &interface->endpoint[0].desc;
  if (!usb_endpoint_is_int_in(endpoint))
    return -ENODEV;

  pipe = usb_rcvintpipe(usbdev, endpoint->bEndpointAddress);
  maxp = usb_maxpacket(usbdev, pipe, usb_pipeout(pipe));

  ac511 = kzalloc(sizeof(struct stAC511), GFP_KERNEL);
  inputdev = input_allocate_device();
  if (!ac511 || !inputdev)
    goto fail1;

  ac511->data = kmalloc(8, GFP_ATOMIC);
  if (!ac511->data)
    goto fail1;

  ac511->irq = usb_alloc_urb(0, GFP_KERNEL);
  if (!ac511->irq)
    goto fail2;

  ac511->usbdev = usbdev;
  ac511->inputdev = inputdev;

  strcpy(ac511->name, DRVNAME);
  inputdev->name = ac511->name;
  inputdev->phys = ac511->name;

  inputdev->id.bustype = BUS_USB;
  inputdev->id.vendor = IDVENDOR;
  inputdev->id.product = IDPRODUCT;
  inputdev->id.version = IDVERSION;

  inputdev->dev.parent = &intf->dev;

  set_bit(EV_KEY, inputdev->evbit);

  set_bit(KEY_VOLUMEDOWN, inputdev->keybit);
  set_bit(KEY_VOLUMEUP, inputdev->keybit);

  input_set_drvdata(inputdev, ac511);

  inputdev->open = ac511_open;
  inputdev->close = ac511_close;

  usb_fill_int_urb(ac511->irq, usbdev, pipe, ac511->data, maxp,
                   ac511_irq, ac511, endpoint->bInterval);

  error = input_register_device(ac511->inputdev);
  if (error)
    goto fail3;

  usb_set_intfdata(intf, ac511);
  return 0;
fail3:
  usb_free_urb(ac511->irq);
fail2:
  kfree(ac511->data);
fail1:
  input_free_device(inputdev);
  kfree(ac511);
  return error;
}

static void ac511_disconnect(struct usb_interface *intf)
{
  log("disconnect");
  struct stAC511 *ac511 = usb_get_intfdata(intf);

  usb_set_intfdata(intf, NULL);

  if (ac511)
  {
    usb_kill_urb(ac511->irq);
    input_unregister_device(ac511->inputdev);
    usb_free_urb(ac511->irq);
    kfree(ac511->data);
    kfree(ac511);
  }
}

static struct usb_device_id ac511_id_table[] =
    {
        {USB_DEVICE(IDVENDOR, IDPRODUCT)},
        {}};

MODULE_DEVICE_TABLE(usb, ac511_id_table);

static struct usb_driver ac511_drv = {
    .name = DRVNAME,
    .probe = ac511_probe,
    .disconnect = ac511_disconnect,
    .id_table = ac511_id_table,
};

static int __init driver_enter(void)
{
  return usb_register(&ac511_drv);
}

static void __exit driver_exit(void)
{
  usb_deregister(&ac511_drv);
}

module_init(driver_enter);
module_exit(driver_exit);

MODULE_LICENSE(DRIVER_LICENSE);
MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
